/*
 * Aaron McDaid - redoing my range library. Calling it rr.hh for now
 * with namespace 'rr'
 */
#include<utility>
#include<vector>

namespace rr {
    template<typename R, typename = void>
    struct traits;

    namespace impl {
        template<int i>
        struct priority_tag;
        template<int i>
        struct priority_tag : public priority_tag<i-1> {};
        template<>
        struct priority_tag<0> {};
    }

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

    pair_of_values<int> ints(int u) { return {0,u}; }
    pair_of_values<int> ints(int l, int u) { return {l,u}; }

    template <typename T>
    auto
    as_range(T &v)
    ->decltype(std:: make_pair(v.begin(), v.end()))
    {
        return std:: make_pair(v.begin(), v.end());
    }

    template<typename I>
    struct traits<std:: pair<I,I>> {
        using R = std:: pair<I,I>;
        using value_type = typename I:: value_type;
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

    template<typename F, typename Tag_type>
    struct forward_this_with_a_tag {
        F m_r; // may be lvalue or rvalue
    };

    template<typename R, typename F>
    struct mapping_range {
        static_assert(!std::is_reference<R>{},"");
        static_assert(!std::is_reference<F>{},"");
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

    template<typename Tag_type>
    struct tagger_t {
    };


    struct map_tag_t            {};     tagger_t<map_tag_t          >   map_range;
    struct map_collect_tag_t    {};     tagger_t<map_collect_tag_t  >   map_collect;
    struct collect_tag_t        {};     collect_tag_t                   collect;    // no need for 'tagger_t', this directly runs
    struct take_collect_tag_t   {};     tagger_t<take_collect_tag_t >   take_collect;

    template<typename R, typename Tag_type>
    auto operator| (R && r, tagger_t<Tag_type>) {
        return forward_this_with_a_tag<R, Tag_type>    {   std::forward<R>(r)  };
    }

    template<typename R, typename Func>
    auto operator| (forward_this_with_a_tag<R,map_tag_t> f, Func && func) {
        return mapping_range<   std::remove_reference_t<R>
                            ,   std::remove_reference_t<Func>
                            > { std::forward<R   >(f.m_r)
                              , std::forward<Func>(func)
                              };
    }

    template<typename R, typename Func>
    auto operator| (forward_this_with_a_tag<R,map_collect_tag_t> f, Func func) {

        auto r = std::forward<R   >(f.m_r); // copy/move the range here

        using value_type = decltype (   func   (  rr::front_val( r )  ));
        std:: vector<value_type> res;

        while(!rr::empty(r)) {
            res.push_back( func(rr::front_val(r)));
            rr::advance(r);
        }

        return res;
    }

    template<typename R>
    auto operator| (R && r, collect_tag_t) {
        using value_type = decltype (   rr::front_val( r )  );
        std:: vector<value_type> res;

        while(!rr::empty(r)) {
            res.push_back( rr::front_val(r) );
            rr::advance(r);
        }

        return res;
    }

    template<typename R>
    auto operator| (forward_this_with_a_tag<R,take_collect_tag_t> f, int how_many) {

        auto r = std::forward<R   >(f.m_r); // copy/move the range here

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
