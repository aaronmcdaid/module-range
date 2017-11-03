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
#include<vector>

/*
 * rr_utils
 *
 * I define 'is_invokable_v' in this namespace as it's range specific and might be useful elsewhere.
 */
namespace rr_utils {
    /*  'priority_tag' is very useful to specify priority
     *  among overloads that would otherwise be ambiguous.
     *  https://stackoverflow.com/questions/43470741/how-does-eric-nieblers-implementation-of-stdis-function-work
     */
    template<int i>
    struct priority_tag;
    template<int i>
    struct priority_tag : public priority_tag<i-1> {};
    template<>
    struct priority_tag<0> {};

    namespace impl__is_invokable {
        template<typename F, typename ... Args>
        constexpr auto
        is_invokable_one_overload(rr_utils::priority_tag<2>)
        -> decltype( std::declval<F>()(std::declval<Args>()...), true )
        { return true; }

        template<typename F, typename ... Args>
        constexpr auto
        is_invokable_one_overload(rr_utils::priority_tag<1>)
        -> decltype( false )
        { return false; }

        template<typename F, typename ... Args>
        constexpr bool
        is_invokable_v =
                   is_invokable_one_overload<F, Args...>(rr_utils::priority_tag<9>{});
    }

    using impl__is_invokable:: is_invokable_v;  // to 'export' this to the rr_utils namespace

    namespace testing_namespace {
        /*
         * To make a tester which checks if a give type has a '.size()' method, we define a lambda with the
         * relevant expression 'x.size()'. And also, we test if addition, (x+x), is defined.
         */
        auto checker_for__has_size_method   = [](auto&&x)->decltype(void(  x.size() )){};
        auto checker_for__has_addition      = [](auto&&x)->decltype(void(  x + x    )){};

        template<typename Arg>
        constexpr bool has_size_method  = rr_utils:: is_invokable_v<decltype(checker_for__has_size_method), Arg >;
        template<typename Arg>
        constexpr bool has_addition     = rr_utils:: is_invokable_v<decltype(checker_for__has_addition), Arg >;

        static_assert( has_size_method< std::vector<int> > ,"");
        static_assert(!has_size_method< int              > ,"");
        static_assert( has_size_method< std::vector<int> > ,"");
        static_assert(!has_size_method< int              > ,"");
    }
}

namespace orange {
    template<typename R>
    struct traits;

    auto checker_for__is_range=[](auto&&x)->decltype(void(  traits< std::remove_reference_t<decltype(x)>>{}  )){};

    template<typename T >
    constexpr bool
    is_range_v = rr_utils:: is_invokable_v<decltype(checker_for__is_range), T>;

    // Let's start with the simplest example - a std::pair of iterators
    template<typename I, typename J>
    struct traits<std:: pair<I,J>> {
        using R = std:: pair<I,J>;
        using value_type = std::remove_reference_t<decltype( *std::declval<I>() )>;

        template<typename RR> static constexpr
        bool
        empty           (RR && r)   { return r.first == r.second ;}

        template<typename RR> static
        void
        advance         (RR && r)   { ++ r.first  ;}

        template<typename RR> static constexpr
        decltype(auto)
        front_ref       (RR && r)   { return * r.first ;}
    };

    namespace testing_namespace {
        static_assert(is_range_v< std::pair< std::vector<int>::iterator,  std::vector<int>::iterator> >, "");
        static_assert(is_range_v< std::pair<int*, int*> >, "");
    }

    /*
     * In order to 'synthesize' the user-facing functions ( orange::front_val, orange::empty, and so on )
     * for a range type R, we need a convenient way to check which functions are provided in the trait<R>.
     * These are the 'has_trait_*' functions defined here:
     */

    auto checker_for__has_trait_empty       = [](auto&&r)->decltype(void( traits<std::remove_reference_t<decltype(r)>>::empty    (r) )){};
    auto checker_for__has_trait_advance     = [](auto&&r)->decltype(void( traits<std::remove_reference_t<decltype(r)>>::advance  (r) )){};
    auto checker_for__has_trait_front_val   = [](auto&&r)->decltype(void( traits<std::remove_reference_t<decltype(r)>>::front_val(r) )){};
    auto checker_for__has_trait_front_ref   = [](auto&&r)->decltype(void( traits<std::remove_reference_t<decltype(r)>>::front_ref(r) )){};
    auto checker_for__has_trait_pull        = [](auto&&r)->decltype(void( traits<std::remove_reference_t<decltype(r)>>::pull     (r) )){};

    template<typename R> constexpr bool
    has_trait_empty     = rr_utils:: is_invokable_v<decltype(checker_for__has_trait_empty), R>;
    template<typename R> constexpr bool
    has_trait_advance   = rr_utils:: is_invokable_v<decltype(checker_for__has_trait_advance), R>;
    template<typename R> constexpr bool
    has_trait_front_val = rr_utils:: is_invokable_v<decltype(checker_for__has_trait_front_val), R>;
    template<typename R> constexpr bool
    has_trait_front_ref = rr_utils:: is_invokable_v<decltype(checker_for__has_trait_front_ref), R>;
    template<typename R> constexpr bool
    has_trait_pull      = rr_utils:: is_invokable_v<decltype(checker_for__has_trait_pull), R>;


    static_assert( has_trait_empty    < std::pair<int*, int*> > , "");
    static_assert(!has_trait_front_val< std::pair<int*, int*> > , "");
    static_assert( has_trait_front_ref< std::pair<int*, int*> > , "");
    static_assert(!has_trait_front_ref< std::vector<int> > , "");


    /*
     * Users will never call the functions in the trait object directly.
     * Instead, we synthesize all the functions, where possible, such
     * as orange:empty, orange::front_val, orange::advance.
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
    empty  (R const &r)
    ->decltype(traits<R>::empty(r))
    { return traits<R>::empty(r); }

    // two overloads for 'front_val', as we can use 'front_ref'
    // instead if it's present.
    template<typename R , std::enable_if_t<
        has_trait_front_val<R>
    > * = nullptr >
    auto constexpr
    front_val  (R const &r)
    ->decltype(auto) { return traits<R>::front_val(r); }
    template<typename R , std::enable_if_t<
        !has_trait_front_val<R> && has_trait_front_ref<R>
    > * = nullptr >
    auto constexpr
    front_val  (R const &r)
    ->decltype(auto) { return traits<R>::front_ref(r); }

    // one overload for 'front_ref'
    template<typename R>
    auto constexpr
    front_ref  (R & r)
    ->decltype(traits<R>::front_ref(r)) {
        return traits<R>::front_ref(r); }

    // one overload for 'advance'
    template<typename R>
    auto constexpr
    advance    (R       &r)
    ->decltype(traits<R>::advance(r)) {
        return traits<R>::advance(r); }

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
    ->decltype(traits<R>::begin  (r)) {
        return traits<R>::begin  (r); }

    // one overload for 'end'
    template<typename R>
    auto constexpr
    end        (R       &r)
    ->decltype(traits<R>::end    (r)) {
        return traits<R>::end    (r); }

    /* Three overloads for 'pull'.
     *  1. has 'pull' in its trait
     *  2. doesn't have 'pull' but does have 'front_val' and 'advance'
     *  3. doesn't have 'pull' nor 'front_val' but does have 'front_ref' and 'advance'
     */
    template<typename R , std::enable_if_t< has_trait_pull<R>>* =nullptr>
    auto constexpr
    pull       (R       &r)
    { return traits<R>::pull     (r); }
    template<typename R , std::enable_if_t<
        !has_trait_pull <R> && has_trait_front_val<R> && has_trait_advance<R>
    >* =nullptr>
    auto constexpr
    pull       (R       &r) {
        auto copy = traits<R>::front_val(r);
        traits<R>::advance(r);
        return copy; }
    template<typename R , std::enable_if_t<
        !has_trait_pull <R> && !has_trait_front_val<R> && has_trait_front_ref<R> && has_trait_advance<R>
    >* =nullptr>
    auto constexpr
    pull       (R       &r) {
        auto copy = traits<R>::front_ref(r);
        traits<R>::advance(r);
        return copy; }
}

namespace orange {

    /*
     * Next, a 'pair_of_iterators' type in the orange:: namespace. The main (only?)
     * reason for this (as opposed to an std::pair of iterators) is to allow
     * 'begin' and 'end' to be defined appropriately, allowing  for(auto x : r) to
     * work.  This is the class used when applying thing like '|'
     */

    template<typename B, typename E>
    struct pair_of_iterators : public std::pair<B,E>
    {
        static_assert(!std::is_reference<B>{}, "");
        static_assert(!std::is_reference<E>{}, "");

        using std:: pair<B,E> :: pair; // to inherit the constructors

        /* This struct looks pointless, but it's not.
         * This struct, because it's in the orange:: namespace,
         * can be found by ADL and therefore our begin/end are found easily
         *
         * The main place you see this is in the return from as_range()
         */
    };

    /*
     * 'pair_of_values', so that we can range between a pair of numbers.  See
     * the 'ints' function below.
     *
     * This is related to 'iter_is_own_value', which is what we get if we call 'begin'
     * and 'end' on a 'pair_of_values'.
     */
    template<typename T>
    struct pair_of_values { T m_begin;
                            T m_end; };

    template<typename I>
    struct iter_is_own_value {
        I m_i;

        bool    operator!=  (iter_is_own_value const & other) const { return  m_i != other.m_i; }
        void    operator++  ()                                      {       ++m_i; }
        I       operator*   ()                                const { return  m_i; }
    };


    template<typename T>
    struct traits< pair_of_values<T> > {
        template<typename R> static constexpr
        bool empty      (R &  r)   { return r.m_begin == r.m_end ;}

        template<typename R> static constexpr
        T    front_val  (R &  r)   { return r.m_begin; }

        template<typename R> static constexpr
        void advance    (R &  r)   {     ++ r.m_begin; }

        template<typename R> static constexpr
        auto begin      (R &  r)   { return iter_is_own_value<T>{r.m_begin};}

        template<typename R> static constexpr
        auto end        (R &  r)   { return iter_is_own_value<T>{r.m_end  };}
    };

    inline
    constexpr
    pair_of_values<int> ints(int u) { return {0,u}; }
    inline
    constexpr
    pair_of_values<int> ints(int l, int u) { return {l,u}; }

    /*
     * as_range
     * Converts a non-range to a range, where appropriate. The obvious examples
     * are a container such as 'std::vector' or 'std::list'.  as_range can also
     * be called with two iterators.
     * I should also define an 'as_range' overload that accepts a range and
     * forwards it as-is.
     */
    template <typename T>
    auto constexpr
    as_range(T &v)
    -> pair_of_iterators<   decltype(v.begin()) ,   decltype(v.end  ()) >
    { return {v.begin(),v.end()}; }

    template <typename T, std:: size_t N>
    auto constexpr
    as_range(T (&v)[N])
    -> pair_of_iterators< T*, T* >
    { return {std::begin(v),std::end(v)}; }

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
        front_ref       (R & r)   { return * std::forward<R>(r) .first ;}

        template<typename R> static constexpr
        auto begin      (R & r)   { return r.first; }

        template<typename R> static constexpr
        auto end        (R & r)   { return r.second; }
    };

    /*
     * Above, all the basic underlying technology for a range has
     * been defined. Now, the 'user-facing' code must be implemented,
     * allowing   |mapr|  and  |filter|  and so on.
     *
     *  v |foreach| [](auto x){ std::cout << x << '\n'; }
     *
     * The above works because we overload the '|' operator. 'foreach'
     * is an object of an empty tag type. Therefore  v|foreach  is
     * a valid expression which doesn't do much except capture a reference
     * to  v  . Then, via another overload of  |  , we apply the lambda.
     * So, the above can be read as
     *
     *  (v | foreach)  |  [](auto x){ std::cout << x << '\n'; }
     */

    template<typename Tag_type>
    struct tagger_t {
        constexpr tagger_t() {} // clang-3.8.0 insists on a user-provided default constructor
    };

    /* Don't worry about the 'extern' below. I think it's safe.
     * I've asked StackOverflow about it:
     *  https://stackoverflow.com/questions/47073805/safe-to-pass-empty-variables-by-value-even-with-no-linkage
     */

    struct foreach_tag_t        {};     constexpr   tagger_t<foreach_tag_t      >   foreach;
    struct map_tag_t            {};     constexpr   tagger_t<map_tag_t          >   map_range;
                                        constexpr   tagger_t<map_tag_t          >   mapr;
    struct map_collect_tag_t    {};     constexpr   tagger_t<map_collect_tag_t  >   map_collect;
    struct collect_tag_t{constexpr collect_tag_t(){}};
                                        constexpr            collect_tag_t          collect;    // no need for 'tagger_t', this directly runs
    struct accumulate_tag_t{constexpr accumulate_tag_t(){}};
                                        constexpr            accumulate_tag_t       accumulate;    // no need for 'tagger_t', this directly runs
    struct take_collect_tag_t   {};     constexpr   tagger_t<take_collect_tag_t >   take_collect;

    template<typename R, typename Tag_type>
    struct forward_this_with_a_tag {
        R m_r;
        static_assert(!std:: is_reference<R>{}, ""); // lvalue or rvalue, but not value
        static_assert( is_range_v< std::remove_reference_t<R> >, "");
    };

    // These two are crucial. This is where we overload '|' so this works:
    //
    //    x|oper
    //
    // where 'x' is a range (or is convertible to a range via 'as_range' and
    // 'oper' is one of {foreach,map_range,mapr,map_collect,collect,take_collec}
    template<typename R, typename Tag_type
        , std::enable_if_t< is_range_v<R> > * = nullptr // if 'r' is a range
        >
    auto operator| (R r, tagger_t<Tag_type>) {
        static_assert( is_range_v<R> ,"");
        return forward_this_with_a_tag<R, Tag_type>    {   std::move(r)  };
    }
    template<typename R, typename Tag_type
        , typename Rnonref = std::remove_reference_t<R>
        , std::enable_if_t<!is_range_v<Rnonref> > * = nullptr // if 'nr' is a not a range
        >
    auto operator| (R && nr, tagger_t<Tag_type> tag)
    ->decltype(as_range(std::forward<R>(nr)) | tag)
    {
        return as_range(std::forward<R>(nr)) | tag;
    }

    /*
     * Now, to start defining the various  |operations|
     */
    /*
     * |mapr|   (also known as |map_range|
     *
     * Define the object which stores the underlying range, and the
     * functor, by value.
     * {Note, I should make sure it can 'move' non-copyables in}
     *
     * Then, define the traits for this 'mapping_range'
     */

    template<typename R, typename F>
    struct mapping_range {
        static_assert(!std::is_reference<R>{},"");
        static_assert(!std::is_reference<F>{},"");
        static_assert( is_range_v<R>, "");
        R m_r;
        F m_f;
    };

    template<typename under_R, typename F>
    struct traits<mapping_range<under_R,F>> {
        using R = mapping_range<under_R,F>;
        using value_type = decltype( orange::front_val  ( std::declval<R>().m_r ));
        static
        bool empty      (R const &r) { return orange:: empty(r.m_r);}
        static
        void advance    (R       &r) { orange::advance( r.m_r ) ;}
        static
        auto front_val      (R const &r) { return r.m_f(orange::front_val  ( r.m_r )) ;}
    };

    // |mapr| or |map_range|
    template<typename R, typename Func>
    auto operator| (forward_this_with_a_tag<R,map_tag_t> f, Func && func) {
        return mapping_range<   std::remove_reference_t<R>      // so we store it by value
                            ,   std::remove_reference_t<Func>
                            > { std::move         (f.m_r)
                              , std::forward<Func>(func)
                              };
    }

    // |collect|
    template<typename R, typename Func>
    auto operator| (forward_this_with_a_tag<R,map_collect_tag_t> f, Func func) {

        static_assert(!std::is_reference<decltype(f.m_r)>{}, "");

        using value_type = decltype (   func   (  orange::front_val( f.m_r )  ));
        std:: vector<value_type> res;

        while(!orange::empty(f.m_r)) {
            res.push_back( func(orange::pull(f.m_r)));
        }

        return res;
    }

    template<typename R
        , typename Rnonref = std::remove_reference_t<R>
        , std::enable_if_t< is_range_v<Rnonref> > * = nullptr
        >
    auto operator| (R r, collect_tag_t) {
        static_assert( is_range_v<R> ,"");
        using value_type = decltype (   orange::front_val( r )  );
        std:: vector<value_type> res;

        while(!orange::empty(r)) {
            res.push_back( orange::pull(r) );
        }

        return res;
    }

    // next, forward 'as_range()' if the lhs is not a range
    template<typename R
        , typename Rnonref = std::remove_reference_t<R>
        , std::enable_if_t< !is_range_v<Rnonref> > * = nullptr >
    auto operator| (R && r, collect_tag_t) {
        return as_range(std::forward<R>(r)) | orange:: collect;
    }

    //  |accumulate
    template<typename R
        , std::enable_if_t< is_range_v<R> > * = nullptr
        >
    constexpr
    auto operator| (R r, accumulate_tag_t) {
        static_assert(!std::is_reference<R>{},"");
        static_assert( is_range_v<R> ,"");

        using value_type = std::remove_reference_t<decltype(orange::pull(r))>;
        value_type total = 0;

        while(!orange::empty(r)) {
            total += orange::pull(r);
        }

        return total;
    }

    // |take_collect
    template<typename R>
    auto operator| (forward_this_with_a_tag<R,take_collect_tag_t> f, int how_many) {

        static_assert(!std::is_reference<decltype(f.m_r)>{}, "");

        using value_type = decltype (   orange::front_val( f.m_r )  );
        std:: vector<value_type> res;

        while(!orange::empty(f.m_r) && how_many>0) {
            res.push_back( orange::front_val(f.m_r));
            orange::advance(f.m_r);
            --how_many;
        }

        return res;
    }

    namespace testing_namespace {
        static_assert( 10 ==  (ints(5) | accumulate)  ,"");
        constexpr double x[] = {1.0, 2.7, 3.14};
        static_assert(1.0 + 2.7 + 3.14 == (as_range(std::begin(x), std::end(x)) | accumulate) ,"");
        static_assert(1.0 + 2.7 + 3.14 == (as_range(x)                          | accumulate) ,"");
    }
} // namespace orange
