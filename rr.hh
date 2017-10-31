/*
 * Aaron McDaid - redoing my range library. Calling it rr.hh for now
 * with namespace 'rr'
 */

namespace rr {
    template<typename T>
    struct pair_of_values { T begin;
                            T end; };

    template<typename R, typename = void>
    struct traits;

    template<typename T>
    struct traits< pair_of_values<T> > {
        using R = pair_of_values<T>;
        static
        bool empty      (R const &r) { return r.begin == r.end ;}
        static
        T    front      (R const &r) { return r.begin; }
        static
        void advance    (R       &r) {     ++ r.begin; }
    };

    pair_of_values<int> ints(int u) { return {0,u}; }

    template<typename R>
    auto empty  (R const &r)
    ->decltype(traits<R>::empty(r)) {
        return traits<R>::empty(r); }
    template<typename R>
    auto front  (R const &r)
    ->decltype(traits<R>::front(r)) {
        return traits<R>::front(r); }
    template<typename R>
    auto advance    (R       &r)
    ->decltype(traits<R>::advance(r)) {
        return traits<R>::advance(r); }
} // namespace rr
