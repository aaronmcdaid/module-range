/*
 * Aaron McDaid - redoing my range library. Calling it rr.hh for now
 * with namespace 'rr'
 */
#include<utility>
#include<vector>

#include"rr.utils.hh" // a number of useful things, not really range-specific

/*
 * Brief description, and overview of this code
 * ============================================
 *
 * ( This documentation includes some stuff that isn't implemented. We
 *   should implement more! )
 *
 *      vector<int>  v {2,3,5,7};
 *      // print every value
 *      v |foreach| [](auto x) { std::cout << x << '\n'; };
 *      // print the square of each value
 *      v   |mapr|      [](auto x) { return x*x; }
 *          |foreach|   [](auto y) { std:: cout "x^2=" << y << '\n';};
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
 * These names are all available in the rr:: namespace. They will use
 * the underlying traits actions where they are provided and, in some cases,
 * this library can synethesize extra functions where they are not explicit
 * in your trait; for example, we can synthesize 'rr::pull' from 'front_val'
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
 * details. For example, an input range must be able to support the 'rr::empty'
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
 * Next, the functions in 'rr::' are defined, relying on the operations
 * provided in the traits object. For example, this defines 'rr::front_val':
 *
 *  template<typename R>
 *  auto front_val  (R const &r)
 *  ->decltype(traits<R>::front_val(r)) {
 *      return traits<R>::front_val(r); }
 *
 * Another overload of 'rr::front_val' could be provided to synthesize
 * front_val where the traits has 'front_ref', but not 'front_val'.
 *
 */

namespace rr {
    template<typename R, typename = void> // second arg is in case I want to use 'void_t' with some traits. http://en.cppreference.com/w/cpp/types/void_t
    struct traits;

    template<typename Possible_Range >
    constexpr
    bool is_range_v =   (true?nullptr:rr_utils::can_apply_ptr([](auto x)-> decltype(sizeof(
                                    traits<decltype(x)>{}    // test if this expression is valid ...
                        )) {return 0;} ,
                                    rr_utils::declVal< std::remove_reference_t<Possible_Range> >() // ... when x has this type
                        ))->value;

    // Let's start with the simplest example - and std::pair of iterators
    template<typename I>
    struct traits<std:: pair<I,I>> {
        using R = std:: pair<I,I>;
        using value_type = std::remove_reference_t<decltype( *std::declval<I>() )>;
        static_assert(!std::is_reference<value_type>{}, "");
        static
        bool empty      (R const &r) {
            return r.first == r.second ;}
        static
        void advance    (R       &r) {
                ++ r.first  ;}
        static
        value_type front_val      (R const &r) {
            return *r.first ;}
    };
    static_assert(is_range_v< std::pair< std::vector<int>::iterator,  std::vector<int>::iterator> >, "");
    static_assert(is_range_v< std::pair<int*, int*> >, "");

    template<typename R> constexpr bool
    has_trait_empty     =(true?nullptr:rr_utils::can_apply_ptr([](auto&&r)->decltype( traits<std::remove_reference_t<decltype(r)>>::empty    (r) ,0){return 0;}, rr_utils::declVal<R>()))->value;
    template<typename R> constexpr bool
    has_trait_advance   =(true?nullptr:rr_utils::can_apply_ptr([](auto&&r)->decltype( traits<std::remove_reference_t<decltype(r)>>::advance  (r) ,0){return 0;}, rr_utils::declVal<R>()))->value;
    template<typename R> constexpr bool
    has_trait_front_val =(true?nullptr:rr_utils::can_apply_ptr([](auto&&r)->decltype( traits<std::remove_reference_t<decltype(r)>>::front_val(r) ,0){return 0;}, rr_utils::declVal<R>()))->value;
    template<typename R> constexpr bool
    has_trait_pull      =(true?nullptr:rr_utils::can_apply_ptr([](auto&&r)->decltype( traits<std::remove_reference_t<decltype(r)>>::pull     (r) ,0){return 0;}, rr_utils::declVal<R>()))->value;


    static_assert( has_trait_empty    < std::pair<int*, int*> > , "");
    static_assert( has_trait_front_val< std::pair<int*, int*> > , "");


    /*
     * Users will never call the functions in the trait object directly.
     * Instead, we synthesize all the functions, where possible, such
     * as rr:empty, rr::front_val, rr::advance.
     *
     * This design allows us to synthesize extra functions. For example,
     * if a trait has 'front' and 'advance', but not 'pull', then we
     * are still able to synthesize 'rr::pull' using the first two.
     * This allows each trait to focus on the smallest subset of
     * necessary behaviour.
     */

    template<typename R>
    auto empty  (R const &r)
    ->decltype(traits<R>::empty(r)) {
        return traits<R>::empty(r); }
    template<typename R>
    auto front_val  (R const &r)
    ->decltype(traits<R>::front_val(r)) {
        return traits<R>::front_val(r); }
    template<typename R>
    auto advance    (R       &r)
    ->decltype(traits<R>::advance(r)) {
        return traits<R>::advance(r); }
    template<typename R>
    auto begin      (R       &r)
    ->decltype(traits<R>::begin  (r)) {
        return traits<R>::begin  (r); }
    template<typename R>
    auto end        (R       &r)
    ->decltype(traits<R>::end    (r)) {
        return traits<R>::end    (r); }

    template<typename R , std::enable_if_t< has_trait_pull<R>>* =nullptr>
    auto pull       (R       &r)
    { return traits<R>::pull     (r); }
    template<typename R , std::enable_if_t<
        !has_trait_pull <R> && has_trait_front_val<R> && has_trait_advance  <R>
    >* =nullptr>
    auto pull       (R       &r) {
        auto copy = traits<R>::front_val(r);
        traits<R>::advance(r);
        return copy; }

    /*
     * Next, a 'pair_of_iterators' type in the rr:: namespace. The main (only?)
     * reason for this is to allow 'begin' and 'end' to be defined appropriately,
     * allowing  for(auto x : r) to work.
     * This is the class used when applying thing like |
     */

    template<typename B, typename E>
    struct pair_of_iterators : public std::pair<B,E>
    {
        static_assert(!std::is_reference<B>{}, "");
        static_assert(!std::is_reference<E>{}, "");
        pair_of_iterators(B b, E e) : std::pair<B,E>(b,e) {}
        /* This struct looks pointless, but it's not.
         * This struct, because it's in the rr:: namespace,
         * can be found by ADL and therefore our begin/end are found easily
         *
         * The main place you see this is in the return from as_range()
         */
    };

    /*
     * 'pair_of_values', so that we can range between a pair of numbers.  This
     * is related to 'iter_is_own_value', which is what we get if we call 'begin'
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
        using R = pair_of_values<T>;
        using value_type = T;
        static
        bool empty      (R const &r) { return r.m_begin == r.m_end ;}
        static
        T    front_val  (R const &r) { return r.m_begin; }
        static
        void advance    (R       &r) {     ++ r.m_begin; }
        static
        auto begin      (R       &r) { return iter_is_own_value<T>{r.m_begin};}
        static
        auto end        (R       &r) { return iter_is_own_value<T>{r.m_end  };}
    };

    inline
    pair_of_values<int> ints(int u) { return {0,u}; }
    inline
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
    auto
    as_range(T &v)
    -> pair_of_iterators<   decltype(v.begin())
                        ,   decltype(v.end  ())
                        >
    {
        return {v.begin(),v.end()};
    }

    template <typename T>
    auto
    as_range(T b, T e)
    ->decltype(pair_of_iterators<   decltype(b)
                                ,   decltype(e)
                                >   {b,e})
    {
        return {b,e};
    }

    template<typename B, typename E>
    struct traits<pair_of_iterators<B,E>> {
        using R = pair_of_iterators<B,E>;
        using value_type = typename B:: value_type;
        static
        bool empty      (R const &r) {
            return r.first == r.second ;}
        static
        void advance    (R       &r) {
                ++ r.first  ;}
        static
        value_type front_val      (R const &r) {
            return *r.first ;}
        static
        auto begin      (R       &r) { return r.first; }
        static
        auto end        (R       &r) { return r.second; }
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
    struct collect_tag_t{constexpr collect_tag_t(){}};     constexpr            collect_tag_t          collect;    // no need for 'tagger_t', this directly runs
    struct take_collect_tag_t   {};     constexpr   tagger_t<take_collect_tag_t >   take_collect;

    template<typename R, typename Tag_type>
    struct imperfect_forward_this_with_a_tag {
        R m_r; /* may be lvalue or value, but *not* rvalue, as the temporary
                * will soon be destroyed. e.g. The as_range object made from
                * 'v' in this code will be destroyed quickly:
                *           v |mapr| [](auto){}
                */
        static_assert(!std:: is_rvalue_reference<R>{}, ""); // lvalue or rvalue, but not value
        static_assert( is_range_v< std::remove_reference_t<R> >, "");
    };

    // These two are crucial. This is where we overload '|' so this works:
    //
    //    x|oper
    //
    // where 'x' is a range (or is convertible to a range via 'as_range' and
    // 'oper' is one of {foreach,map_range,mapr,map_collect,collect,take_collec}
    template<typename R, typename Tag_type
        , typename Rnonref = std::remove_reference_t<R>
        , std::enable_if_t< is_range_v<Rnonref> > * = nullptr // if 'x' is a range
        >
    auto operator| (R && r, tagger_t<Tag_type>) {
        static_assert( is_range_v<R> ,""); // TODO: make sure I can break this with an lvalue!
        return imperfect_forward_this_with_a_tag<R, Tag_type>    {   std::forward<R>(r)  };
    }
    template<typename R, typename Tag_type
        , typename Rnonref = std::remove_reference_t<R>
        , std::enable_if_t<!is_range_v<Rnonref> > * = nullptr // if 'x' is a not a range
        >
    auto operator| (R && r, tagger_t<Tag_type> tag)
    ->decltype(as_range(std::forward<R>(r)) | tag)
    {
        return as_range(std::forward<R>(r)) | tag;
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
        using value_type = decltype( rr::front_val  ( std::declval<R>().m_r ));
        static
        bool empty      (R const &r) { return rr:: empty(r.m_r);}
        static
        void advance    (R       &r) { rr::advance( r.m_r ) ;}
        static
        auto front_val      (R const &r) { return r.m_f(rr::front_val  ( r.m_r )) ;}
    };

    // |mapr| or |map_range|
    template<typename R, typename Func>
    auto operator| (imperfect_forward_this_with_a_tag<R,map_tag_t> f, Func && func) {
        return mapping_range<   std::remove_reference_t<R>      // so we store it by value
                            ,   std::remove_reference_t<Func>
                            > { std::forward<R   >(f.m_r)
                              , std::forward<Func>(func)
                              };
    }

    // |collect|
    template<typename R, typename Func>
    auto operator| (imperfect_forward_this_with_a_tag<R,map_collect_tag_t> f, Func func) {

        auto r = std::forward<R   >(f.m_r); // copy/move the range here
        static_assert(!std::is_reference<decltype(r)>{}, "");

        using value_type = decltype (   func   (  rr::front_val( r )  ));
        std:: vector<value_type> res;

        while(!rr::empty(r)) {
            res.push_back( func(rr::front_val(r)));
            rr::advance(r);
        }

        return res;
    }

    template<typename R
        , typename Rnonref = std::remove_reference_t<R>
        , std::enable_if_t< is_range_v<Rnonref> > * = nullptr
        >
    auto operator| (R && r, collect_tag_t) {
        // should I copy 'r' in here?
        // Or, adjust the other functions to work directly on
        // the 'r' or 'f.m_r' that has been passed in?
        static_assert( is_range_v<R> ,"");
        using value_type = decltype (   rr::front_val( r )  );
        std:: vector<value_type> res;

        while(!rr::empty(r)) {
            res.push_back( rr::front_val(r) );
            rr::advance(r);
        }

        return res;
    }

    // next, forward 'as_range()' if the lhs is not a range
    template<typename R
        , typename Rnonref = std::remove_reference_t<R>
        , std::enable_if_t< !is_range_v<Rnonref> > * = nullptr >
    auto operator| (R && r, collect_tag_t) {
        return as_range(std::forward<R>(r)) | rr:: collect;
    }

    // |take_collect
    template<typename R>
    auto operator| (imperfect_forward_this_with_a_tag<R,take_collect_tag_t> f, int how_many) {

        auto r = std::forward<R   >(f.m_r); // copy/move the range here
        static_assert(!std::is_reference<decltype(r)>{}, "");

        using value_type = decltype (   rr::front_val( r )  );
        std:: vector<value_type> res;

        while(!rr::empty(r) && how_many>0) {
            res.push_back( rr::front_val(r));
            rr::advance(r);
            --how_many;
        }

        return res;
    }
#if 0
#endif
} // namespace rr
