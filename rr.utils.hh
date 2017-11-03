// a number of things useful in 'rr.hh', but which are more general purpose and
// don't really need to be seen to understand 'rr.hh'.

#include<utility>

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

    namespace impl {
        template<typename F, typename G>
        struct overload_first_t {
            F f;
            G g;

            template<typename FF, typename GG>
            overload_first_t(FF&&ff, GG&&gg) : f(std::forward<FF>(ff))
                                             , g(std::forward<GG>(gg)) {}

            // make two indirect function, first and second, so
            // that the other args don't cause overload resolution
            // tie-breaking issues
            template<typename ... Ts> auto constexpr
            high    (Ts && ... ts)
            ->decltype(f(   std::forward<Ts>(ts)... )  )
            {   return f(   std::forward<Ts>(ts)... ); }
            template<typename ... Ts> auto constexpr
            low     (Ts && ... ts)
            ->decltype(g(   std::forward<Ts>(ts)... )  )
            {   return g(   std::forward<Ts>(ts)... ); }

            // Two call_me functions, with the priority_tag
            // to mark which is most important
            template<typename ... Ts> auto constexpr
            call_me (priority_tag<2>, Ts && ... ts)
            ->decltype(high(    std::forward<Ts>(ts)... )  )
            {   return high(    std::forward<Ts>(ts)... ); }

            template<typename ... Ts> auto constexpr
            call_me (priority_tag<1>, Ts && ... ts)
            ->decltype(low (    std::forward<Ts>(ts)... )  )
            {   return low (    std::forward<Ts>(ts)... ); }

            // Finally, the actual call operator. Maybe I should do cv-qualified versions too :-(
            template<typename ... Ts> decltype(auto) constexpr
            operator() (Ts && ... ts) { return call_me( priority_tag<9>{}, std::forward<Ts>(ts)... ); }
        };
    } // subnamespace impl

    template<typename F, typename G>
    auto overload_first(F&& f, G&& g) {
        return impl:: overload_first_t<F,G>(   std::forward<F>(f)
                                    ,   std::forward<G>(g) );
    }

    namespace impl {
        template<typename F, typename ... Ts> constexpr auto
        can_apply_(rr_utils::priority_tag<2>, F && f, Ts && ... ts)
        -> decltype( f(std::forward<Ts>(ts)...) , std::true_type{} )
        { return {}; }

        template<typename F, typename ... Ts> constexpr auto
        can_apply_(rr_utils::priority_tag<1>, F &&  , Ts && ...   )
        -> std:: false_type
        { return {}; }
    }

    template<typename T>
    T declVal() // better than std:: declval, because it complains less about being called!
    { struct wrap { T t; }; return ((wrap*)nullptr) -> t; }

    template<typename ... Ts> constexpr auto
    can_apply_ptr(Ts && ... ts)
    ->decltype( impl::can_apply_(rr_utils::priority_tag<9>{}, std::forward<Ts>(ts)...)) *
    { return nullptr; } // a pointer to either std::true_type or std::false_type

    template<typename T> constexpr bool
    demo_of_can_apply_ptr     =(true?nullptr:can_apply_ptr([](auto&&x)->decltype( *x ,0){return 0;}, declVal<T>()))->value;

    static_assert( demo_of_can_apply_ptr<int*>, "");
    static_assert(!demo_of_can_apply_ptr<int >, "");

}
