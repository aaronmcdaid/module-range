/*
 * orange  - yet another range library
 *
 * By Aaron McDaid - aaron.mcdaid@gmail.com
 *
 *
 * In the code below, I'm trying to organize it so that it can be
 * read from top to bottom and is understandable.
 *
 * Where possible, I put some tests into a 'testing_namespace'. You might
 * find it useful to just search for that string in this file and read
 * the tests.
 *
 *
 * Brief description, and overview of this code
 * ============================================
 *
 * ( This documentation includes some stuff that isn't implemented. We
 *   should implement more! )
 *
 *      vector<int>  v {2,3,5,7};
 *
 *      // print every value
 *      v |foreach| [](auto x) { std::cout << x << '\n'; };
 *
 *      // print the square of each value
 *      v   |mapr|      [](auto x) { return x*x; }
 *          |foreach|   [](auto y) { std:: cout "x^2=" << y << '\n';};
 *
 *      // filter to only the odd ones, then print them:
 *      v   |filter|  [](auto x) { return x%2 == 1; }
 *          |foreach| [](auto x) { std::cout << x << '\n'; };
 *
 * ( say 'mapr' instead of 'map' simply to avoid clashing with 'std::map')
 *
 * Many different types can be considered as 'range types'. The obvious
 * example is a pair of iterators, but there are many others too.
 * A 'vector' is not itself a range; but it is trivially convertable
 * to a range.
 *
 * A 'range' is typically a very lightweight object, like a pointer, that
 * can be copied easily. It doesn't usually "own" the underlying data,
 * but this library supports ownership where appropriate.
 *
 * A range will support some subset of these actions:
 *  -   empty       ::: No more input is available to read
 *  -   front_val   ::: Read the current value - repeated calls will return
 *                      the same value (unless the underlying container has
 *                      been modified by some other part of the system.
 *  -   advance     ::: skip the current item and move to the next
 *
 *  -   full        ::: if an output range can no longer be written to
 *  -   front_ref   ::: return a reference to the current item. Repeated
 *                      calls will return a reference to the same object.
 *  -   push        ::: write a value to an output range and advance. This
 *                      is useful when treating the standard output as an
 *                      output range. It's not possible to define 'front_ref'
 *                      on such a range as we can't meaningful write to the
 *                      "same place" in the output repeatedly. Once we write
 *                      to the stream, our next write must be to the following
 *                      'position' in the output range.
 *  -   pull        ::: return the current value and also advance. As if
 *                      running front_val and then advance. Useful when a
 *                      range doesn't allow repeating read
 *
 * Via traits (see below), you can specify, for your own types, how these
 * actions are to be performed on your objects.
 * These names are all available in the orange:: namespace. They will use
 * the underlying traits actions where they are provided and, in some cases,
 * this library can synethesize extra functions where they are not explicit
 * in your trait; for example, we can synthesize 'orange::pull' from 'front_val'
 * and 'advance' if your trait does not contain 'advance'
 *
 * This becomes useful when you want to combine a range and a function,
 * and create a new range which exposes range where the function has been
 * applied to each element of the underlying range.
 *
 * ==
 *
 * A 'range type', R, here is a type for which the type traits<R> exists.
 * More precisely, traits<R> can also be default constructed. The traits
 * object has no state, its purpose is simply to record the type-specific
 * details. For example, an input range must be able to support the 'orange::empty'
 * function which tells us if more input is available. For a pair of iterators,
 * this means testing the two iterators to see if they are equal. For a file
 * input stream, we test the stream for end-of-file.
 *
 * (I'll try to document the functions in the order they appear below in
 * the code)
 *
 *  is_range_v      ::: constexpr-bool function to test if a given
 *                      type R has a suitable traits<R>
 *
 * The code then has the traits definition for a std::pair of iterators.
 * Traits for other types are specified later in this code, but I brought
 * std::pair to the top as it's simple and helps me to explain this system
 *
 *  template<typename I>
 *  struct traits<std:: pair<I,I>>
 *
 * For now, this just means providing 'empty', 'advance' and 'front_val'
 * In future, some functions for trait a pair as an output range should
 * be added, such as 'full' and 'front_ref'.
 *
 * Next, the functions in 'orange::' are defined, relying on the operations
 * provided in the traits object. For example, this defines 'orange::front_val':
 *
 *  template<typename R>
 *  auto front_val  (R const &r)
 *  ->decltype(traits<R>::front_val(r)) {
 *      return traits<R>::front_val(r); }
 *
 * Another overload of 'orange::front_val' could be provided to synthesize
 * front_val where the traits has 'front_ref', but not 'front_val'.
 *
 */

#include<utility>
#include<functional>
#include<algorithm> // for std::min
#include<tuple>
#include<vector>
#include<limits>
#include<memory>

 /* SFINAE_ENABLE_IF_CHECK
  * ======================
  *     Sorry for the macro, but this macro is just too useful! It has two
  * extra 'features' over and above a straightforward enable_if.
  *
  * Here is a simple conventional use of enable_if in a function template:
  *
  *     template<typename T
  *             , std::enable_if_t< std::is_lvalue_reference<T>{} >* =nullptr
  *             >
  *     bool i_am_an_lvalue(T&&)
  *     { return true; }
  *
  * and I suggest this instead:
  *
  *     template<typename T
  *             , SFINAE_ENABLE_IF_CHECK( std::is_lvalue_reference<T>{} )
  *             >
  *     bool i_am_an_lvalue(T&&)
  *     { return true; }
  *
  * for these two reasons:
  *  1) two overloads with very similar signatures can clash with each other,
  *     even when we know only one will satisfy the condition, causing an error.
  *     This macro solves this by building the line number (__LINE__) into the
  *     pointer type.
  *  2) If the condition is not deduced, then the conventional method will give
  *     an error. For example, if it's a template parameter in an outer struct.
  *     This macro solves this by separating the condition into two template
  *     parameters (hence the comma in the macro, to separate the args).
  *     Hence, the previous code expands to the following, which works because
  *     the boolean is merely given a default value, which is then tested:
  *
  *     template<typename T
  *           bool hidden_test_expression_for_enable_if = ( std::is_lvalue_reference<T>{} )
  *         , std::enable_if_t<hidden_test_expression_for_enable_if, std:: integral_constant<size_t, __LINE__> >* =nullptr
  *             >
  *     bool i_am_an_lvalue(T&&)
  *     { return true; }
  */
#define SFINAE_ENABLE_IF_CHECK(...)                                       \
              typename ...                                                              \
            , bool hidden_test_expression_for_enable_if = (__VA_ARGS__)           \
            , std::enable_if_t<hidden_test_expression_for_enable_if, std:: integral_constant<size_t, __LINE__> >* =nullptr

/*
 * orange_utils
 *
 * I define 'is_invokable_v' in this namespace as it's range specific and might be useful elsewhere.
 */
namespace orange_utils {



    /*  priority_tag
     *  ============
     *      'priority_tag' is very useful to specify priority
     *  among overloads that would otherwise be ambiguous.
     *  https://stackoverflow.com/questions/43470741/how-does-eric-nieblers-implementation-of-stdis-function-work
     */
    template<int i>
    struct priority_tag;
    template<int i>
    struct priority_tag : public priority_tag<i-1> {};
    template<>
    struct priority_tag<0> {};


    /*  void_t
     *  ======
     * https://stackoverflow.com/questions/27687389/how-does-void-t-work
     */
    template< typename ... >
    struct voider_t { using type = void; };
    template< typename ... Ts> using void_t = typename voider_t<Ts...> :: type;


    /*  is_invokable_v
     *  ==============
     *  is_invokable_v<F, Args...> tells us if the function object F
     *  can be called with arguments of types Args...
     */
    namespace impl__is_invokable {
        template<typename F, typename ... Args>
        constexpr auto
        is_invokable_one_overload(orange_utils::priority_tag<2>)
        -> decltype( std::declval<F>()(std::declval<Args>()...), true )
        { return true; }

        template<typename F, typename ... Args>
        constexpr auto
        is_invokable_one_overload(orange_utils::priority_tag<1>)
        -> decltype( false )
        { return false; }

        template<typename F, typename ... Args>
        constexpr bool
        is_invokable_v =
                   is_invokable_one_overload<F, Args...>(orange_utils::priority_tag<9>{});
    }

    using impl__is_invokable:: is_invokable_v;  // to 'export' this to the orange_utils namespace

    template<typename ... Ts>
    constexpr
    void ignore(Ts && ...) {}

    /* testing_namespace
     * =================
     *  Throughout this file, I'll put tests, using static_assert, into this
     *  namespace. Reading the tests might help you to understand more of
     *  this code.
     */
    namespace testing_namespace {
        /*
         * To make a tester which checks if a give type has a '.size()' method, we define a lambda with the
         * relevant expression 'x.size()'. And also, we test if addition, (x+x), is defined.
         */
        auto checker_for__has_size_method   = [](auto&&x)->decltype(void(  x.size() )){};
        auto checker_for__has_addition      = [](auto&&x)->decltype(void(  x + x    )){};

        template<typename Arg>
        constexpr bool has_size_method  = orange_utils:: is_invokable_v<decltype(checker_for__has_size_method), Arg >;
        template<typename Arg>
        constexpr bool has_addition     = orange_utils:: is_invokable_v<decltype(checker_for__has_addition), Arg >;

        static_assert( has_size_method< std::vector<int> > ,"");
        static_assert(!has_size_method< int              > ,"");
        static_assert( has_size_method< std::vector<int> > ,"");
        static_assert(!has_size_method< int              > ,"");
    }

    template <class F, std::size_t... I>
    constexpr decltype(auto) apply_indices(F&& f, std::index_sequence<I...>)
    {
        return      std::forward<F>(f)
                    (std::integral_constant<size_t,I>{}...);
    }

    template<typename ... Ts>
    constexpr
    std::tuple<Ts...> // values and l-refs, but not r-ref
    mk_tuple(Ts && ... ts) {
        return std::tuple<Ts...>{ std::forward<Ts>(ts)... };
    }

    template<typename T>
    struct remove_RVALUE_reference
    { using type = T; };
    template<typename T>
    struct remove_RVALUE_reference<T&&>
    { using type = T; };

    template<typename T>
    auto non_rref(T&& t)
    -> typename remove_RVALUE_reference<T>::type
    { return t; }
}

namespace orange {


    /*  traits<R>
     *  =========
     *      If 'R' is a range type, then this traits class tells us
     *  how to use it; how to test if it's empty, for example.
     *  With a pair of iterators, we test for emptiness by testing
     *  if the two iterators equal to each other. With a file input
     *  stream, we would test for emptiness by testing for .eof().
     */
    template<typename R, typename = void> // second template arg is to allow 'void_t' https://stackoverflow.com/questions/27687389/how-does-void-t-work
    struct traits;


    /*  lookup_traits<R>
     *  ================
     *      We don't look up 'traits' directly. We go through 'lookup_traits'
     *  instead, as it drops 'const' and drops references.
     */
    template<typename R
            , typename R_decayed = std::decay_t<R>
            , decltype( traits< R_decayed> {} ) * = nullptr >
    struct lookup_traits : public traits<R_decayed> {};


    /*  checker_for__is_range  is_range_v
     *  =====================  ==========
     *      is_range_v<R> tests if lookup_traits<R> is defined.
     *  This is how we define is a type is a range type or not.
     */
    auto checker_for__is_range=[](auto&&x)->decltype(void(  lookup_traits< decltype(x)>{}  )){};

    template<typename T > constexpr bool
    is_range_v = orange_utils:: is_invokable_v<decltype(checker_for__is_range), T>;


    /*  has_trait_{empty,advance,front,pull}
     *  ==================================================
     *      In order to 'synthesize' the user-facing functions ( orange::front, orange::empty, and so on )
     *  for a range type R, we need a convenient way to check which functions are provided in the trait<R>.
     *  These are the 'has_trait_*' functions defined here:
     */

    auto checker_for__has_trait_empty       = [](auto&&r)->decltype(void( lookup_traits<decltype(r)>::empty    (r) )){};
    auto checker_for__has_trait_advance     = [](auto&&r)->decltype(void( lookup_traits<decltype(r)>::advance  (r) )){};
    auto checker_for__has_trait_front       = [](auto&&r)->decltype(void( lookup_traits<decltype(r)>::front    (r) )){};
    auto checker_for__has_trait_pull        = [](auto&&r)->decltype(void( lookup_traits<decltype(r)>::pull     (r) )){};

    template<typename R> constexpr bool
    has_trait_empty     = orange_utils:: is_invokable_v<decltype(checker_for__has_trait_empty), R>;
    template<typename R> constexpr bool
    has_trait_advance   = orange_utils:: is_invokable_v<decltype(checker_for__has_trait_advance), R>;
    template<typename R> constexpr bool
    has_trait_front     = orange_utils:: is_invokable_v<decltype(checker_for__has_trait_front    ), R>;
    template<typename R> constexpr bool
    has_trait_pull      = orange_utils:: is_invokable_v<decltype(checker_for__has_trait_pull), R>;


    /*
     * Users will never call the functions in the trait object directly.
     * Instead, we synthesize all the functions, where possible, such
     * as orange:empty, orange::front, orange::advance.
     *
     * This design allows us to synthesize extra functions. For example,
     * if a trait has 'front' and 'advance', but not 'pull', then we
     * are still able to synthesize 'orange::pull' using the first two.
     * This allows each trait to focus on the smallest subset of
     * necessary behaviour.
     */


    // just one overload for 'empty'
    template<typename R>
    auto constexpr
    empty  (R &r)
    ->decltype(lookup_traits<R>::empty(r))
    { return lookup_traits<R>::empty(r); }


    // one overload for 'advance'
    template<typename R>
    auto constexpr
    advance    (R       &r)
    ->decltype(lookup_traits<R>::advance(r))
    {   return lookup_traits<R>::advance(r); }


    // 'front'

    template<typename R>
    auto constexpr
    front    (R       &r)
    ->decltype(lookup_traits<R>::front(r))
    {   return lookup_traits<R>::front(r); }

    /* Next, we see 'begin' and 'end', which are useful
     * for working with range-based for.
     *
     * TODO: synthesize a suitable pair of iterators
     * for range types that don't specify a begin and
     * end of their own.
     */

    // one overload for 'begin'
    template<typename R>
    auto constexpr
    begin      (R       &r)
    ->decltype(lookup_traits<R>::begin  (r))
    {   return lookup_traits<R>::begin  (r); }

    // one overload for 'end'
    template<typename R>
    auto constexpr
    end        (R       &r)
    ->decltype(lookup_traits<R>::end    (r))
    {   return lookup_traits<R>::end    (r); }


    /* Two overloads for 'pull'.
     *  1. has 'pull' in its trait
     *  2. doesn't have 'pull' but does have 'front' and 'advance'
     */

    // pull, via trait_pull
    template<typename R
            , SFINAE_ENABLE_IF_CHECK( has_trait_pull<R&> )
            >
    auto constexpr
    pull       (R       &r)
    { return lookup_traits<R>::pull     (r); }

    // pull, via front
    template<typename R
            , SFINAE_ENABLE_IF_CHECK(( !has_trait_pull <R&> && has_trait_front<R&> && has_trait_advance<R&>
                    && !std::is_same<void, decltype( lookup_traits<R>::front(std::declval<R&>()) )>{}
                    ))
            >
    auto constexpr // TODO: proper as_value here, to convert nested refs (inside tuples) to values
    pull       (R       &r)
    {
        auto copy = lookup_traits<R>::front(r);
        lookup_traits<R>::advance(r);
        return copy;
    }

    // void version of pull, via front
    template<typename R
            , SFINAE_ENABLE_IF_CHECK( !has_trait_pull <R&> && has_trait_front<R&> && has_trait_advance<R&>
                    &&  std::is_same<void, decltype( lookup_traits<R>::front(std::declval<R&>()) )>{}
                    )
            >
    auto constexpr
    pull       (R       &r)
    -> void
    {
        lookup_traits<R>::front(r);
        lookup_traits<R>::advance(r);
    }
}



/*
 * Everything above is very general. It's relevant for all range
 * types. Next, we get more specific, by defining some traits
 * and the methods.
 *
 * The functions in the trait are static, and capture the range
 * as R&, where R is a deduced template parameter.
 */

namespace orange {


    // Let's start with the simplest example - a std::pair of iterators
    template<typename I, typename J>
    struct traits<std:: pair<I,J>> {

        template<typename R> static constexpr
        bool
        empty           (R & r)   { return r.first == r.second ;}

        template<typename R> static
        void
        advance         (R & r)   { ++ r.first  ;}

        template<typename R> static constexpr
        decltype(auto)
        front           (R & r)   { return * r.first ;}
    };


    namespace testing_namespace {
        static_assert(is_range_v< std::pair< std::vector<int>::iterator,  std::vector<int>::iterator> >, "");
        static_assert(is_range_v< std::pair<int*, int*> >, "");
        static_assert( has_trait_empty    < std::pair<int*, int*> > , "");
        static_assert( has_trait_front    < std::pair<int*, int*> > , "");
        static_assert(!has_trait_front    < std::vector<int> > , "");
    }


    /*
     *  orange_traits_are_static_here
     *  =============================
     *      Writing separate traits classes for every type might be annoying. If
     *  you have control over your own type, you may choose to place the
     *  definitions of the functions directly in your class. To do so, you need
     *  to create a typedef in your class called 'orange_traits_are_static_here'
     *  with type 'orange  ::orange_traits_are_static_here', and then provide
     *  the *static* methods you want (named 'orange_empty', 'orange_front',
     *  and so on). To see an example of this, just scroll down to the
     *  'mapping_range' template below.
     *
     *      Immediately following this comment is the traits class which tests
     *  for the appropriate typedef and knows how to forward all such calls to
     *  the appropriate function.
     *
     */
    struct orange_traits_are_static_here{};
    template<typename T>
    struct traits   < T
                    , orange_utils:: void_t< std:: enable_if_t<
                        std:: is_same<
                                typename T::orange_traits_are_static_here
                                , orange  ::orange_traits_are_static_here
                        >{}
                    > >> {


        template<typename R> static constexpr auto
        empty      (R &  r)
        ->decltype(R:: orange_empty    (r))
        {   return R:: orange_empty    (r); }

        template<typename R> static constexpr auto
        advance    (R &  r)
        ->decltype(R:: orange_advance  (r))
        {   return R:: orange_advance  (r); }

        template<typename R> static constexpr auto
        front      (R &  r)
        ->decltype(R:: orange_front    (r))
        {   return R:: orange_front    (r); }

        template<typename R> static constexpr auto
        pull       (R &  r)
        ->decltype(R:: orange_pull     (r))
        {   return R:: orange_pull     (r); }

        template<typename R> static constexpr auto
        begin      (R &  r)
        ->decltype(R:: orange_begin    (r))
        {   return R:: orange_begin    (r); }

        template<typename R> static constexpr auto
        end        (R &  r)
        ->decltype(R:: orange_end      (r))
        {   return R:: orange_end      (r); }
    };
}

namespace orange {
    /*
     * 'pair_of_values', so that we can range between a pair of numbers.  See
     * the 'ints' function below.
     *
     * This is related to 'iter_is_own_value', which is what we get if we call 'begin'
     * and 'end' on a 'pair_of_values'.
     */

    namespace impl {
        template<typename I>
        struct iter_is_own_value {
            I m_i;

            bool    operator!=  (iter_is_own_value const & other) const { return  m_i != other.m_i; }
            void    operator++  ()                                      {       ++m_i; }
            I       operator*   ()                                const { return  m_i; }
        };
    }

    template<typename T>
    struct pair_of_values
    {
        T m_begin;
        T m_end;

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;

        template<typename R> static constexpr
        bool orange_empty      (R &  r)   { return r.m_begin == r.m_end ;}

        template<typename R> static constexpr
        T    orange_front      (R &  r)   { return r.m_begin; }

        template<typename R> static constexpr
        void orange_advance    (R &  r)   {     ++ r.m_begin; }

        template<typename R> static constexpr
        auto orange_begin      (R &  r)   { return impl:: iter_is_own_value<T>{r.m_begin};}

        template<typename R> static constexpr
        auto orange_end        (R &  r)   { return impl:: iter_is_own_value<T>{r.m_end  };}
    };

    struct intsFrom0_t
    {
        constexpr
        pair_of_values<int>
        operator() (int u) const
        { return {0,u}; }

        constexpr intsFrom0_t(){}
    } constexpr intsFrom0;
    inline
    constexpr
    pair_of_values<int> ints(int u) { return {0,u}; }
    inline
    constexpr
    pair_of_values<int> ints(int l, int u) { return {l,u}; }
    inline
    constexpr
    pair_of_values<int> ints() { return {0,std::numeric_limits<int>::max()}; }


    template<typename T>
    struct replicate_t
    {
        int64_t m_n;
        T m_t;
        static_assert(!std::is_reference<T>{} ,"");

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;
        template<typename R> static constexpr auto
        orange_empty      (R &r) ->bool             { return r.m_n <= 0;}
        template<typename R> static constexpr auto
        orange_advance    (R &r) ->void             { --r.m_n; }
        template<typename R> static constexpr auto
        orange_front      (R &r) ->T                { return r.m_t; }
    };

    template<typename T>
    auto constexpr
    replicate(int64_t n, T t)
    { return replicate_t<T>{n,std::move(t)}; }

    template<typename T>
    auto constexpr
    repeat   (T t)
    { return replicate_t<T>{std::numeric_limits<int64_t>::max(),std::move(t)}; }



    /*
     * Next, a 'pair_of_iterators' type in the orange:: namespace. The main (only?)
     * reason for this (as opposed to an std::pair of iterators) is to allow
     * 'begin' and 'end' to be defined appropriately, allowing  for(auto x : r) to
     * work.  This is the class used when applying thing like '|'
     * It's in the 'orange::' namespace, so 'orange::begin' and 'orange::end'
     * can be found by ADL.
     */

    template<typename B, typename E>
    struct pair_of_iterators : public std::pair<B,E>
    {
        static_assert(!std::is_reference<B>{}, "");
        static_assert(!std::is_reference<E>{}, "");

        using std:: pair<B,E> :: pair; // to inherit the constructors
    };


    /*
     * as_range
     * Converts a non-range to a range, where appropriate. The obvious examples
     * are a container such as 'std::vector' or 'std::list'.  as_range can also
     * be called with two iterators.
     */

    // already a range? Just return as is
    template<typename T
            , SFINAE_ENABLE_IF_CHECK( is_range_v<T> )
            >
    constexpr auto
    as_range(T &&t)
    ->decltype(std::forward<T>(t))
    {   return std::forward<T>(t); }

    // a container with begin and end. The typical use of 'as_range'
    template <typename T>
    auto constexpr
    as_range(T &v)
    -> pair_of_iterators<   decltype(v.begin()) ,   decltype(v.end  ()) >
    {
        static_assert(!is_range_v<T> ,"");
        return {v.begin(),v.end()};
    }

    // an array
    template <typename T, std:: size_t N>
    auto constexpr
    as_range(T (&v)[N])
    -> pair_of_iterators< T*, T* >
    { return {std::begin(v),std::end(v)}; }

    template<typename T, size_t N>
    struct owning_range_for_ye_olde_C_array {
        T m_array[N];
        size_t m_offset;

        // don't allow this to be copied
        owning_range_for_ye_olde_C_array    (owning_range_for_ye_olde_C_array const &) = delete;
        owning_range_for_ye_olde_C_array &  operator=  (owning_range_for_ye_olde_C_array const &) = delete;

        // ... but allow moving
        constexpr
        owning_range_for_ye_olde_C_array    (owning_range_for_ye_olde_C_array      &&) = default;
        constexpr
        owning_range_for_ye_olde_C_array &  operator=  (owning_range_for_ye_olde_C_array      &&) = default;

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;
        template<typename M> static constexpr auto
        orange_empty      (M &m) ->bool             { return m.m_offset >= N;}
        template<typename M> static constexpr auto
        orange_advance    (M &m) ->void             { ++m.m_offset; }
        template<typename M> static constexpr auto
        orange_front      (M &m) ->decltype(auto)   { return m.m_array[m.m_offset]; }
    };


    // an rvalue array. We need a helper function first though:
    template <typename T, std:: size_t N, std:: size_t ... Indices>
    auto constexpr
    as_range_helper_for_rvalue_plain_C_arrays   ( T (&&a)[N]
                                                , std::index_sequence<Indices...>
                                                )
    -> owning_range_for_ye_olde_C_array<T,N>
    { return {{ std::move(a[Indices]) ... },0}; }

    // for rvalue-array, we copy the array via the helper above and then we
    // can store it in a special type, 'owning_range_for_ye_olde_C_array',
    // that knows how to place nice inside 'constexpr'.
    template <typename T, std:: size_t N>
    auto constexpr
    as_range(T (&&a)[N])
    -> owning_range_for_ye_olde_C_array<T,N>
    {
        return as_range_helper_for_rvalue_plain_C_arrays(std::move(a), std::make_index_sequence<N>());
    }

    // two iterators as arguments
    template <typename T>
    auto constexpr
    as_range(T b, T e)
    ->decltype(pair_of_iterators<   decltype(b) ,   decltype(e) >   {b,e})
    { return {b,e}; }

    template<typename B, typename E>
    struct traits<pair_of_iterators<B,E>> {
        template<typename R> static constexpr
        bool
        empty           (R & r)   { return r.first == r.second ;}

        template<typename R> static constexpr
        void
        advance         (R & r)   { ++ r.first  ;}

        template<typename R> static constexpr
        decltype(auto)
        front           (R & r)   { return * std::forward<R>(r) .first ;}

        template<typename R> static constexpr
        auto begin      (R & r)   { return r.first; }

        template<typename R> static constexpr
        auto end        (R & r)   { return r.second; }
    };

    template<typename C>
    struct owning_range { // non-copyable
        static_assert(!std::is_reference<C>{}   ,"");
        static_assert(!is_range_v<C>            ,"");

        using R = decltype( as_range(std::declval<C&>()) );
        static_assert(!std::is_reference<R>{} ,"");
        static_assert( is_range_v<R>            ,"");

        C m_c;
        R m_r;


        constexpr
        owning_range    (C && c) // takes only an rvalue reference, *not* a universal reference
        : m_c(std::move(c)) , m_r( as_range(m_c) )
        {}

        // don't allow this to be copied
        owning_range    (owning_range const &) = delete;
        owning_range &  operator=  (owning_range const &) = delete;

        // ... but allow moving
        constexpr
        owning_range    (owning_range      &&) = default;
        constexpr
        owning_range &  operator=  (owning_range      &&) = default;

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;
        template<typename M> static constexpr auto
        orange_empty      (M &m) ->bool
        { return orange:: empty(m.m_r);}
        template<typename M> static constexpr auto
        orange_advance    (M &m) ->void
        { orange::advance( m.m_r ) ; }
        template<typename M> static constexpr auto
        orange_front      (M &m) ->decltype(auto)
        { return orange::front    ( m.m_r ) ;}
    };

    // as_range, for rvalues that aren't ranges. In this case, we wrap them
    // up on owning_range. It's non-copyable, that's how it maintains the
    // semantics of ranges.
    template<typename T
            , SFINAE_ENABLE_IF_CHECK( !std::is_reference<T>{} && !is_range_v<T> )
            >
    auto constexpr
    as_range(T &&t) // enable_if is used here, to ensure it is only an rvalue
    {
        return owning_range<T>{ std::forward<T>(t) };
    }


    /*
     * Above, all the basic underlying technology for a range has
     * been defined. Now, the 'user-facing' code must be implemented,
     * allowing   |mapr|  and  |filter|  and so on.
     *
     *  v |foreach| [](auto x){ std::cout << x << '\n'; }
     *
     * The above works because we overload the '|' operator. 'foreach'
     * is an object of an empty tag type. Therefore  v|foreach  is
     * a valid expression which doesn't do much except capture
     * a copy of the range object
     * Then, via another overload of  |  , we apply the lambda.
     * So, the above can be read as
     *
     *  (v | foreach)  |  [](auto x){ std::cout << x << '\n'; }
     */

    template<typename Tag_type>
    struct tagger_t {
        constexpr tagger_t() {} // clang-3.8.0 insists on a user-provided default constructor
    };

    struct filter_tag_t         {};     constexpr   tagger_t<filter_tag_t       >   filter;

    struct map_tag_t            {};     constexpr   tagger_t<map_tag_t          >   map_range;
                                        constexpr   tagger_t<map_tag_t          >   mapr;

    struct foreach_tag_t        {};     constexpr   tagger_t<foreach_tag_t      >   foreach;

    struct collect_tag_t{constexpr collect_tag_t(){}};
                                        constexpr            collect_tag_t          collect;    // no need for 'tagger_t', this directly runs
    struct discard_collect_tag_t{constexpr discard_collect_tag_t(){}};
                                        constexpr            discard_collect_tag_t  discard_collect;    // no need for 'tagger_t', this directly runs
    struct accumulate_tag_t{constexpr accumulate_tag_t(){}};
                                        constexpr            accumulate_tag_t       accumulate;    // no need for 'tagger_t', this directly runs
    struct concat_tag_t{constexpr concat_tag_t(){}};
                                        constexpr            concat_tag_t           concat;    // no need for 'tagger_t', this directly runs
    struct memoize_tag_t{constexpr memoize_tag_t(){}};
                                        constexpr            memoize_tag_t           memoize;    // no need for 'tagger_t', this directly runs


    // the type to capture the value, i.e. for the left-hand '|'
    // of   (x|operation|func)
    template<typename R, typename Tag_type>
    struct forward_this_with_a_tag {
        R m_r;
        static_assert(!std:: is_reference<R>{}, "");
        static_assert( is_range_v< R >, "");
    };

    /*
     * The actual overloads of '|' are here.
     */
    template<typename R, typename Tag_type
            , SFINAE_ENABLE_IF_CHECK( is_range_v<R> ) // if 'r' is a range
            >
    auto constexpr
    operator| (R r, tagger_t<Tag_type>) {
        static_assert( is_range_v<R> ,"");
        return forward_this_with_a_tag<R, Tag_type>    {   std::move(r)  };
    }
    template<typename R, typename Tag_type
            , typename Rnonref = std::remove_reference_t<R>
            , SFINAE_ENABLE_IF_CHECK( !is_range_v<Rnonref> ) // if 'nr' is a not a range
        >
    auto constexpr
    operator| (R && nr, tagger_t<Tag_type> tag)
    ->decltype(as_range(std::forward<R>(nr)) | tag)
    {
        return as_range(std::forward<R>(nr)) | tag;
    }



    /*
     * all_true
     */
    constexpr bool
    all_true(void) {return true;}
    template<typename ...Ts>
    constexpr bool
    all_true(bool b, Ts && ... ts) {
        if(b)
            return all_true(std::forward<Ts>(ts)...);
        else
            return false;
    }


    /*
     * Now, to start defining the various  |operations|
     */

    // |foreach|
    // =========
    template<typename R, typename Func
            >
    constexpr auto
    operator| (forward_this_with_a_tag<R,foreach_tag_t> r, Func && func)
    -> void
    {
        while(!orange::empty(r.m_r)) {
            func(orange::front(r.m_r));
            orange::advance(r.m_r);
        }
    }


    // |mapr| or |map_range|
    template<typename R, typename F>
    struct mapping_range {
        static_assert(!std::is_reference<R>{},"");
        static_assert(!std::is_reference<F>{},"");
        static_assert( is_range_v<R>, "");
        R m_r;
        F m_f;

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;
        template<typename M> static constexpr bool
        orange_empty      (M &m) { return orange:: empty(m.m_r);}
        template<typename M> static constexpr void
        orange_advance    (M &m) { orange::advance( m.m_r ) ;}
        template<typename M> static constexpr auto
        orange_front      (M &m)
        ->decltype(m.m_f(orange::front      ( m.m_r )) )
        {   return m.m_f(orange::front      ( m.m_r )) ;}
    };

    template<typename R, typename Func>
    auto constexpr
    operator| (forward_this_with_a_tag<R,map_tag_t> f, Func && func) {
        return mapping_range<   std::remove_reference_t<R>      // so we store it by value
                            ,   std::remove_reference_t<Func>
                            > { std::move         (f.m_r)
                              , std::forward<Func>(func)
                              };
    }

    // |filter|
    template<typename R, typename F>
    struct filter_range
    {
        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;
        static_assert(!std::is_reference<R>{},"");
        static_assert(!std::is_reference<F>{},"");
        static_assert( is_range_v<R>, "");

        R m_r;
        F m_f;

        constexpr
        void
        skip_if_necessary() {
            while(!orange::empty(m_r) && !m_f(orange::front(m_r)))
            { orange::advance(m_r); }
        }

        template<typename RR, typename FF>
        constexpr
        filter_range(RR && r, FF && f)
        : m_r(std::forward<RR>(r)) , m_f(std::forward<FF>(f))
        { skip_if_necessary(); }

        template<typename M> static constexpr decltype(auto)
        orange_empty      (M &m) { return orange:: empty    (m.m_r);}

        template<typename M> static constexpr auto
        orange_front      (M &m)
        ->decltype(orange:: front    (m.m_r))
        {   return orange:: front    (m.m_r);}

        template<typename M> static constexpr void
        orange_advance    (M &m) { orange:: advance(m.m_r); m.skip_if_necessary(); }
    };

    template<typename R, typename Func>
    auto constexpr
    operator| (forward_this_with_a_tag<R,filter_tag_t> f, Func && func) {
        return filter_range <   std::remove_reference_t<R>      // so we store it by value
                            ,   std::remove_reference_t<Func>
                            > { std::move         (f.m_r)
                              , std::forward<Func>(func)
                              };
    }

    // |collect|
    template<typename R
            , typename Rnonref = std::remove_reference_t<R>
            , SFINAE_ENABLE_IF_CHECK( is_range_v<Rnonref> )
            >
    auto constexpr
    operator| (R r, collect_tag_t) {
        static_assert( is_range_v<R> ,"");
        using value_type = decltype (   orange::pull( r )  );
        static_assert(!std::is_reference<value_type>{} ,"");
        std:: vector<value_type> res;

        while(!orange::empty(r)) {
            res.push_back( orange::pull(r) );
        }

        return res;
    }


    // |discard_collect|
    template<typename R
            , typename Rnonref = std::remove_reference_t<R>
            , SFINAE_ENABLE_IF_CHECK( is_range_v<Rnonref> )
            >
    void constexpr
    operator| (R r, discard_collect_tag_t) {
        static_assert( is_range_v<R> ,"");

        while(!orange::empty(r))
        { orange::pull(r); }

    }


    // next, forward 'collect' and 'accumulate' via 'as_range()' if the lhs is not a range
    template<typename R
        , typename Tag
        , typename Rnonref = std::remove_reference_t<R>
        , SFINAE_ENABLE_IF_CHECK(   !is_range_v<Rnonref>
                                 && (   std::is_same<Tag, collect_tag_t>{}
                                     || std::is_same<Tag, discard_collect_tag_t>{}
                                     || std::is_same<Tag, accumulate_tag_t>{}
                                     || std::is_same<Tag, concat_tag_t>{}
                                    ))
        >
    auto constexpr
    operator| (R && r, Tag operation) {
        return as_range(std::forward<R>(r)) | operation;
    }

    //  |accumulate
    template<typename R
            , SFINAE_ENABLE_IF_CHECK( is_range_v<R> )
            >
    auto constexpr
    operator| (R r, accumulate_tag_t) {
        static_assert(!std::is_reference<R>{},"");
        static_assert( is_range_v<R> ,"");

        using value_type = std::remove_reference_t<decltype(orange::pull(r))>;
        value_type total = 0;

        while(!orange::empty(r)) {
            total += orange:: front(r);
            orange:: advance(r);
        }

        return total;
    }

    /*  |concat
     *      Flatten a range-of-ranges into a range
     */
    template<typename R>
    struct concat_helper
    {
        R m_r;

        static_assert(!std::is_reference<R>{} ,"");

        constexpr
        concat_helper(R && r)
        : m_r(std::move(r))
        { this->skip_if_necessary(); }

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;

        void constexpr
        skip_if_necessary() {
            while   (   !orange::empty(m_r)
                    &&   orange::empty(orange::front(m_r)))
                    { orange::advance(m_r); }
        }

        template<typename M> static constexpr bool
        orange_empty      (M &m)
        { return orange:: empty(m.m_r);}

        template<typename M> static constexpr void
        orange_advance    (M &m) {
            orange::advance( orange::front(m.m_r) );
            m.skip_if_necessary();
        }

        template<typename RR> static constexpr auto
        orange_front      (RR &r)
        ->decltype(orange::front(orange::front(r.m_r)))
        {
            return orange::front(orange::front(r.m_r));
        }
        /*
        */
    };
    template<typename R
            , SFINAE_ENABLE_IF_CHECK( is_range_v<R> )
            >
    auto constexpr
    operator| (R r, concat_tag_t)
    -> concat_helper<R>
    {
        static_assert( is_range_v<R> ,"");
        static_assert( is_range_v<decltype(orange::front(r))> ,"");

        return {std::move(r)};
    }


    /*  |memoize
     *      first time 'front' is called, store the value and return
     *      the copy later
     */
    template<typename R>
    struct memoize_helper
    {
        static_assert(!std::is_reference<R>{} ,"");
        using val_type = std::remove_reference_t<decltype(orange::front(std::declval<R&>()))>;
        static_assert(!std::is_reference<val_type>{} ,"");

        R m_r;
        std::unique_ptr<val_type> m_current;

        constexpr
        memoize_helper(R && r)
        : m_r(std::move(r))
        {
            if(!orange::empty(m_r)) {
                m_current = std::make_unique<val_type>(orange::front(m_r));
                orange::advance(m_r);
            }
        }

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;


        template<typename M> static constexpr bool
        orange_empty      (M &m)
        {
            //std::cout << m.m_current.get() << '\n';
            return !m.m_current;
        }

        template<typename M> static constexpr void
        orange_advance    (M &m)
        {
            m.m_current.reset();
            if(!orange::empty(m.m_r)) {
                m.m_current = std::make_unique<val_type>(orange::front(m.m_r));
                orange::advance(m.m_r);
            }
        }

        template<typename M> static constexpr auto
        orange_front      (M &m)
        -> val_type&
        { return *m.m_current; }
    };
    template<typename R
            , SFINAE_ENABLE_IF_CHECK( is_range_v<R> )
            >
    auto constexpr
    operator| (R r, memoize_tag_t)
    -> memoize_helper<R>
    {
        static_assert( is_range_v<R> ,"");

        return {std::move(r)};
    }

    namespace testing_namespace {
        static_assert( 10 ==  (ints(5) | accumulate)  ,"");
        constexpr double x[] = {1.0, 2.7, 3.14};
        static_assert(1.0 + 2.7 + 3.14 == (as_range(std::begin(x), std::end(x)) | accumulate) ,"");
        static_assert(1.0 + 2.7 + 3.14 == (as_range(x)                          | accumulate) ,"");

        struct greater_than_5_t {
            constexpr greater_than_5_t() {}
            constexpr bool operator() (int x) const { return x>5; }
        };
        struct odd_t {
            constexpr odd_t() {}
            constexpr bool operator() (int x) const { return x % 2 == 1; }
        };
        struct even_t {
            constexpr even_t() {}
            constexpr bool operator() (int x) const { return x % 2 == 0; }
        };
        struct negate_t {
            constexpr negate_t() {}
            template<typename T>
            constexpr auto operator() (T x) const { return -x; }
        };
        static_assert(20 == (ints(10) |filter| even_t{}             |accumulate) ,"");
        static_assert(25 == (ints(10) |filter| odd_t{}              |accumulate) ,"");
        static_assert(30 == (ints(10) |filter| greater_than_5_t{}   |accumulate) ,"");
        static_assert(-30 == (ints(10) |filter| greater_than_5_t{} |mapr| negate_t{}   |accumulate) ,"");


        struct dummy_int_range_with_pull_and_empty_only {
            int m_i = 0;

            using orange_traits_are_static_here = orange:: orange_traits_are_static_here;
            template<typename M> static constexpr bool
            orange_empty      (M &m) { return m.m_i >= 10;}
            template<typename M> static constexpr auto
            orange_pull       (M &m) { return m.m_i ++;}

        };

        constexpr void negate_me_in_place(int &x) { x = -x ; }

        constexpr int foreach_testing1()
        {
            int x[] = {1980, 1982, 1986, 1990};
            as_range(x) |foreach| negate_me_in_place;
            return as_range(x) | accumulate;
        }

        static_assert( -7938 == orange:: testing_namespace:: foreach_testing1() ,"");

    }
    namespace testing_namespace{
        constexpr double shouldbe10_1() { return as_range ( (double[]) { 1.5,0.1,2.5,2,4 } ) | accumulate; }
        static_assert(10.1 == shouldbe10_1() ,"");
        //static_assert(60 == (as_range( (int[]){10,20,30} ) | accumulate) ,""); // crashes gcc. No worries though, 'shouldbe330' works OK in its place

        constexpr int shouldbe330()
        { return as_range( (int[]) { 100,110,120 } ) | accumulate; }
        static_assert(330 == shouldbe330() , "");

        constexpr
        double modifying_the_owning_stdarray() {
            auto r = as_range ( (double[]) { 1.5,0.1,2.5,2,4 } );
            orange:: front(r) += 1.0;
            return std::move(r) | accumulate;
        }
        static_assert( 11.1 == modifying_the_owning_stdarray() , "");

        constexpr
        int foo() {
            auto ooaa = as_range( (int[]) {10,20,30} );
            orange:: front(ooaa) += 100;
            return std::move(ooaa) | accumulate;
        }
        static_assert( 160 == foo() , "");

        constexpr
        int test_as_range_conversions() {
            int x[] = {7,8,9};
            return x | accumulate;
        }
        static_assert(24 == test_as_range_conversions() ,"");

        constexpr
        int bar() {
            int ai []{ 10,11,12 };
            return (ai | accumulate)
                   + (as_range( (int[]) { 100,110,120 } ) | accumulate);
        }
        static_assert(363 == bar() ,"");
    }


    /*
     * orange_zip_iterator
     * ===================
     *      To get begin()/end() from a zipped object
     */
    template<typename Z>
    struct orange_zip_iterator {
        Z & m_z_reference;
        int m_offset;

        decltype(auto) operator == (orange_zip_iterator const & other) const { return this->m_offset == other.m_offset; }
        decltype(auto) operator != (orange_zip_iterator const & other) const { return this->m_offset != other.m_offset; }
        decltype(auto) operator -  (orange_zip_iterator const & other) const { return this->m_offset -  other.m_offset; }
        decltype(auto) operator <  (orange_zip_iterator const & other) const { return this->m_offset <  other.m_offset; }
        decltype(auto) operator =  (orange_zip_iterator const & other)       { this->m_offset =  other.m_offset; return *this; }
        decltype(auto) operator ++ ()                                        { ++ this->m_offset ;               return *this; }
        decltype(auto) operator -- ()                                        { -- this->m_offset ;               return *this; }
        decltype(auto) operator +  (int jump)                          const { return orange_zip_iterator{m_z_reference, m_offset + jump};}
        decltype(auto) operator -  (int jump)                          const { return orange_zip_iterator{m_z_reference, m_offset - jump};}
        decltype(auto) operator *  ()                                        { return dereference_helper(std::make_index_sequence<Z::width>{}); }

        template<size_t ... Indices>
        decltype(auto)
        dereference_helper         (std::index_sequence<Indices...>)   const
        { return orange_utils::mk_tuple( *(orange::begin(std::get<Indices>(m_z_reference.m_ranges)) + m_offset) ... ); }

        template<size_t ... Indices>
        auto constexpr
        dereference_helper_val     (std::index_sequence<Indices...>)   const
        ->decltype(std::make_tuple       ( *(orange::begin(std::get<Indices>(m_z_reference.m_ranges)) + m_offset) ... ))
        {   return std::make_tuple       ( *(orange::begin(std::get<Indices>(m_z_reference.m_ranges)) + m_offset) ... ); }

        template<size_t ... Indices>
        static auto
        undefined_helper_val(std::index_sequence<Indices...>)
        ->decltype( std::make_tuple       ( *(orange::begin(std::get<Indices>( std::declval<Z&>() .m_ranges)) ) ...))
        ;

        template<size_t ... Indices>
        static auto
        undefined_helper_ref(std::index_sequence<Indices...>)
        ->decltype( orange_utils::mk_tuple( *(orange::begin(std::get<Indices>( std::declval<Z&>() .m_ranges)) /*+m_offset*/ ) ...))
        ;

        using val_type = decltype( undefined_helper_val(std::make_index_sequence<Z::width>()));
        using ref_type = decltype( undefined_helper_ref(std::make_index_sequence<Z::width>()));
    };
} // leave namespace orange for a moment to define std:: iterator_traits

// std::iterator_traits<orange::orange_zip_iterator<orange::zip_t<orange:
namespace std {
    template<typename Z>
    struct iterator_traits<orange::orange_zip_iterator<Z>> {
        using value_type = typename orange::orange_zip_iterator<Z>::val_type;
        using reference  = typename orange::orange_zip_iterator<Z>::ref_type;
        using difference_type = int;
        using iterator_category  = std::random_access_iterator_tag;
    };
    template<size_t ... I, typename ...T>
    void
    swap_helper(std::index_sequence<I...>, std::tuple<T...> l, std::tuple<T...> r) {
        using std::swap;
        orange_utils:: ignore( (void(swap( std::get<I>(l) , std:: get<I>(r) )),0) ... );
    }
    template<typename ...T>
    void
    swap(std::tuple<T...> l, std::tuple<T...> r) {
        swap_helper(std::make_index_sequence< std::tuple_size<decltype(r)>{} >{}, l,r);
    }
}
namespace orange {

    /*
     * zip
     * ===
     */
    enum class enum_zip_policy_on_references
                {   to_value // get_one_item_to_return will apply std::decay_t to the result from each subrange
                ,   as_is };

    template< size_t Index
            , enum_zip_policy_on_references policy
            , typename Z
            , SFINAE_ENABLE_IF_CHECK( policy == enum_zip_policy_on_references:: to_value )
            > static constexpr auto
    get_one_item_to_return(Z & z)
    -> std::decay_t<decltype(
               orange::front(std::template get<Index>(z.m_ranges))
       )>
    {   return orange::front(std::template get<Index>(z.m_ranges)); }

    template< size_t Index
            , enum_zip_policy_on_references policy
            , typename Z
            , SFINAE_ENABLE_IF_CHECK( policy == enum_zip_policy_on_references:: as_is )
            > static constexpr auto
    get_one_item_to_return(Z & z)
    ->decltype(orange::front(std::template get<Index>(z.m_ranges)))
    {   return orange::front(std::template get<Index>(z.m_ranges)); }

    template< typename Z
            , enum_zip_policy_on_references my_policy
            >
    struct zip_helper { // This is to replace a capturing lambda. The only reason is that this is constexpr
        Z & m_z;

        template<size_t ... Indices> auto constexpr
        zip_empty(std::index_sequence<Indices...>)
        -> bool
        {
            return   !all_true(!orange::empty ( std::template get<Indices>(m_z.m_ranges)) ... );
        }

        template<size_t ... Indices> auto constexpr
        zip_advance(std::index_sequence<Indices...>)
        -> void
        {
            orange_utils:: ignore(( orange::advance ( std::template get<Indices>(m_z.m_ranges)) ,0)...);
        }

        template<size_t ... Indices> auto constexpr
        zip_front(std::index_sequence<Indices...>)
        ->decltype(orange_utils::mk_tuple(orange:: get_one_item_to_return<Indices, my_policy>(m_z)...))
        {   return orange_utils::mk_tuple(orange:: get_one_item_to_return<Indices, my_policy>(m_z)...); }
    };

    template< enum_zip_policy_on_references my_policy
            , typename ... Rs >
    struct zip_t {
        static_assert( all_true( std::is_same<Rs, std::decay_t<Rs>>{}   ... ),"");
        static_assert( all_true(  is_range_v<Rs>                        ... ),"");

        constexpr static size_t N = sizeof...(Rs);
        constexpr static size_t width = sizeof...(Rs);

        using type_of_tuple = std:: tuple<std::decay_t<Rs>...>;
        type_of_tuple m_ranges;

        template< typename ... Ts
            , std::enable_if_t<
            all_true(std::is_same<std::decay_t<Ts>, std::decay_t<Rs> >{} ...)
                    >* =nullptr
            >
        constexpr
        zip_t(Ts && ... ts) : m_ranges(std::forward<Ts>(ts)...) {}

        constexpr
        zip_t(zip_t const &) = default;
        constexpr
        zip_t(zip_t      &&) = default;

        using orange_traits_are_static_here = orange:: orange_traits_are_static_here;
        template<typename Z> static constexpr auto
        orange_empty        (Z &  z)    ->decltype(auto)
        {
            return orange:: zip_helper<Z,my_policy>{z}.zip_empty(std::make_index_sequence<N>());
        }

        template<typename Z> static constexpr auto
        orange_advance      (Z &  z)    ->void
        {
            return orange:: zip_helper<Z,my_policy>{z}.zip_advance(std::make_index_sequence<N>());
        }

        template<typename Z> static constexpr auto
        orange_front        (Z &  z)    ->decltype(auto)
        {
            return orange:: zip_helper<Z,my_policy>{z}.zip_front(std::make_index_sequence<N>());
        }

        template<typename Z> static constexpr decltype(auto)
        orange_begin      (Z & z)   {
            return orange_zip_iterator<Z>{z, 0};
        }
        template<typename Z
                ,size_t ... Indices
                > static constexpr decltype(auto)
        orange_end_helper (Z & z, std::index_sequence<Indices...>)   {
            return std::min ({  (end(std::get<Indices>(z.m_ranges))-begin(std::get<Indices>(z.m_ranges))) ...  });
        }
        template<typename Z> static constexpr decltype(auto)
        orange_end        (Z & z)   {
            return orange_zip_iterator<Z> { z, (int)orange_end_helper(z, std:: make_index_sequence<Z::width>()) };
        }
    };

    // zip
    template<typename ... Rs
            , SFINAE_ENABLE_IF_CHECK( all_true(is_range_v<Rs>...) )
            >
    auto constexpr
    zip(Rs && ... rs) {
        return  zip_t<enum_zip_policy_on_references :: to_value, std::decay_t<Rs>...>
                ( std::forward<Rs>(rs)...) ;
    }

    // zip_as_is
    template<typename ... Rs
            , SFINAE_ENABLE_IF_CHECK( all_true(is_range_v<Rs>...) )
            >
    auto constexpr
    zip_as_is(Rs && ... rs) {
        static_assert( true  ,""); // this assert saves "g++ (GCC) 5.5.0" from a crash about missing constructor!
        return  zip_t<enum_zip_policy_on_references :: as_is, std::decay_t<Rs>...>
                ( std::forward<Rs>(rs)...) ;
    }


    // If 'zip' or 'zip_as_is' is given a non-range, then apply 'as_range' and forward them
    template<typename ... Rs
            , SFINAE_ENABLE_IF_CHECK( !all_true(is_range_v<Rs>...) )
            > auto constexpr
    zip(Rs && ... rs)
    -> decltype(auto)
    { return zip( orange::as_range(std::forward<Rs>(rs))...) ; }

    template<typename ... Rs
            , SFINAE_ENABLE_IF_CHECK( !all_true(is_range_v<Rs>...) )
            > auto constexpr
    zip_as_is(Rs && ... rs)
    -> decltype(auto)
    { return zip_as_is( orange::as_range(std::forward<Rs>(rs))...) ; }

    namespace testing_namespace {
        template<typename T>
        struct summer_t {
            T & m_t;

            template<typename Arg>
            constexpr auto
            operator() (Arg && x)
            { m_t += std::get<0>(x) * std::get<1>(x); }
        };
        constexpr
        double zip_test() {
            int     i [] = {1,2,3};
            double  d [] = {1.0,2.5,3.0};
            double  t = 0.0;
            zip( i, d ) |foreach| summer_t<double>{t};
            return t;
        }

        void zip_as_is_types() {
            int ai[] = {4};
            auto z = zip_as_is(ai, ints());
            auto zi = zip_as_is(ints(), ints());
            auto za = zip_as_is(ai, ai);

            static_assert(std::is_same  < decltype(orange::front    (z ))   , std::tuple<int &, int  > >{}, "");
            static_assert(std::is_same  < decltype(orange::front    (zi))   , std::tuple<int  , int  > >{}, "");
            static_assert(std::is_same  < decltype(orange::front    (za))   , std::tuple<int &, int &> >{}, "");
        }
        void zip_types() {
            int ai[] = {4};
            auto z = zip(ai, ints());
            auto zi = zip(ints(), ints());
            auto za = zip(ai, ai);

            static_assert(std::is_same  < decltype(orange::front    (z ))   , std::tuple<int  , int  > >{}, "");
            static_assert(std::is_same  < decltype(orange::front    (zi))   , std::tuple<int  , int  > >{}, "");
            static_assert(std::is_same  < decltype(orange::front    (za))   , std::tuple<int  , int  > >{}, "");
        }
        void zip_recursive_types()
        {
            int ai[] = {4,7,2,9,3,7};
            char   ac[] = {'A','b','c'};
            double ad[] = {4,7,2,9,3,7};

            {
                auto zrec = zip_as_is(ai, zip_as_is(ac, ad));
                static_assert(std::is_same<decltype( orange::front    (zrec) ), std::tuple<int&, std::tuple<char&, double&> >    >{},"");
            }

            {
                auto zrec = zip_as_is(ai, zip_as_is(ac, ints(), ad));
                static_assert(std::is_same<decltype( orange::front    (zrec) ), std::tuple<int&, std::tuple<char&, int, double&> >    >{},"");
            }

            {
                auto zrec = zip_as_is(zip_as_is(ai, zip_as_is(ac, ints(), ad)));
                static_assert(std::is_same<decltype( orange::front    (zrec) ), std::tuple<std::tuple<int&, std::tuple<char&, int, double&> > >  >{},"");
            }

            {
                auto zrec = zip_as_is(zip_as_is(zip_as_is(ac, ints()))); // without the strange is_same<Ts,Rs> check in zip_t constructor, this failed on clang
                static_assert(std::is_same<decltype( orange::front    (zrec) ), std::tuple<std::tuple<std::tuple<char&, int > > >  >{},"");
            }
        }
        static_assert(15 == orange:: testing_namespace:: zip_test() ,"");

        struct sum_all_args_t {
            template<typename T>
            constexpr auto
            operator() (T a) const
            { return a;}

            template<typename T, typename ... Us>
            constexpr auto
            operator() (T a, T b, Us ... us) const
            { return (*this)(a+b, us...); }

            constexpr sum_all_args_t(){}
        } constexpr sum_all_args;

        template<size_t I>
        struct get_I_t {
            template<typename T>
            constexpr auto
            operator() (T && t) const
            -> decltype( orange_utils::non_rref (   std::get<I>(std::forward<T>(t)) ))
            { return                                std::get<I>(std::forward<T>(t)); }

            constexpr get_I_t(){}
        };
        struct less_than_this {
            int threshold;

            template<typename T>
            constexpr auto
            operator() (T x) const
            { return x<threshold; }

            constexpr less_than_this(int t):threshold(t) {}
        };

        template<typename L, typename R>
        struct compose_t
        {   L m_l;
            R m_r;

            template<typename ...T>
            auto constexpr
            operator() (T && ... t)
            { return m_l(m_r(std::forward<T>(t)...)); }
        };
        template<typename L, typename R>
        auto constexpr
        compose(L && l, R && r)
        {
            return compose_t    < std::remove_reference_t<L>
                                , std::remove_reference_t<R>
                                >   { std::forward<L>(l)
                                    , std::forward<R>(r)
                                    };
        }

        constexpr
        int test_zip () {
            int a1[] = {2,-3,5,-8,8};
            int a2[] = {1,10,100,1000,10000};
            auto shouldbe1010 =
                    zip(a1,a2)
                        |filter|    compose(less_than_this{0}, get_I_t<0>{})
                        |mapr|      get_I_t<1>{}
                        |accumulate;
            return shouldbe1010;
        }
        static_assert(1010 == test_zip() ,"");

        constexpr int
        zip_in_place_edits() {
            int a1[] = {2,-3,5,-8,8};
            int a2[] = {10,-20,30,-40,50};

            zip_as_is(a1,a2,ints())
                |filter|    compose(less_than_this{0}, get_I_t<0>{})
                |mapr|      get_I_t<1>{}
                |foreach|   negate_me_in_place
                ;
            return a2 | accumulate;
        }
        static_assert(150 ==zip_in_place_edits() ,"");
    }

    struct apply_pack_tag_t{constexpr apply_pack_tag_t(){}}; constexpr apply_pack_tag_t apply_pack;

    template<typename F>
    struct apply_pack_helper {
        static_assert(!std::is_rvalue_reference<F>{} ,"");

        F m_f;

        template< typename Tup
                , size_t ... I
                > auto constexpr
        call(std::index_sequence<I...>, Tup && tup) const
        ->decltype(m_f( std::template get<I>( tup ) ...))
        {   return m_f( std::template get<I>( tup ) ...); }

        template<typename Tup>
        auto constexpr
        operator() (Tup && tup) const
        ->decltype(call ( std::make_index_sequence< std::tuple_size<Tup>::value >() , std::forward<Tup>(tup)))
        {   return call ( std::make_index_sequence< std::tuple_size<Tup>::value >() , std::forward<Tup>(tup)); }
    };

    template<typename F>
    auto constexpr
    operator% (apply_pack_tag_t, F && f)
    -> apply_pack_helper<F>
    { return {std::forward<F>(f)}; }

    namespace testing_namespace {
        constexpr
        int apply_test() {
            int a1[] = {300,200,100};
            int a2[] = {3,2,1};

            return
            zip(a1,a2,ints())
                |mapr| apply_pack % sum_all_args
                |accumulate;
        }
        static_assert(609 == apply_test(), "");

        constexpr
        int
        replicate_test()
        {
            return replicate(5, 100) | accumulate;
        }
        static_assert(500 == replicate_test() ,"");

        constexpr
        int
        repeat_test()
        {
            return zip(repeat(42), replicate(101, 0))
                    |mapr| get_I_t<0>{}
                    |accumulate;
        }
        static_assert(4242 == repeat_test() ,"");

    }
} // namespace orange
