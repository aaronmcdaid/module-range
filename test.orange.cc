#include "orange.hh"
#include "../bits.and.pieces/PP.hh"
#include "../bits.and.pieces/utils.hh"
#include<iostream>
#include<vector>
using std:: vector;
using std:: string;
using namespace orange;
using utils:: operator<<;

template<typename T>
void print_type(T&&) {
    std::cout << __PRETTY_FUNCTION__ << '\n';
}

int main () {
    auto r_i = ints(3);
    while(!empty(r_i)) {
        PP(front_val(r_i));
        advance(r_i);
    }

    r_i = ints(4);
    for(auto i : r_i) { PP(i); }

    vector<string> v{"hi", "world", "of", "ranges"};
    auto v_r = as_range(v);
    static_assert(                                      orange:: is_range_v< decltype(v_r) >    , "");
    static_assert(                                    ! orange:: is_range_v< decltype(v  ) >    , "");
    while(!empty(v_r)) {
        PP(front_val(v_r));
        advance(v_r);
    }
    auto mapped = v |map_range|[](auto && x)->int{
        return x.length();
    };
    while(!empty(mapped)) {
        PP(front_val(mapped));
        advance(mapped);
    }

    auto o = v |map_range|[](auto && x)->int{
        return -x.length();
    };
    for(;!empty(o);advance(o)) {
        PP(front_val(o));
    }
    auto collected = v |map_collect|[](auto && x) { return 0.5+x.length(); };
    auto recollected = collected |map_range|[](auto && x){return -x;} |collect;
    PP(collected, recollected);
    auto collected_as_range = as_range(collected);
    orange::front_ref(collected_as_range) += 100;
    PP(collected);

    { // test pull
        auto p = std::make_pair(collected.begin(), collected.end());
        while(!empty(p)) {
            PP(pull(p));
        }
        auto ar = as_range(collected);
        while(!empty(ar)) {
            PP(pull(ar));
        }
    }

    constexpr auto t = ints(5) | accumulate;
    PP(t);

    PP( as_range(std:: vector<int>{1,2,3}) | accumulate);

    {
        assert(15 == orange:: testing_namespace:: zip_test());

        auto vi = std::vector<double>{102,102,101};
        auto vd = std::vector<double>{3.3,2.2,1.1};
        auto z =
        zip_val (   as_range(std::vector<int>{3,2,1})
                ,   as_range(std::vector<std::string>{"three","two","one"})
                ,   as_range(vd)
                );
        while(!orange::empty(z)) {
            PP(orange::front_val(z));
            orange::advance(z);
        }
        auto j = std::move(z);

        auto zv =
        zip_val (   as_range(vi)
                ,   as_range(vd)
                );
        auto zv2 = zv;
        zv2 |foreach| [](auto && t){ PP(t); };
        zv2 |foreach| [](auto && t){ PP(t); };


        auto zr = zip_ref( as_range(vd), as_range(vd) );
        while(!orange::empty(zr)) {
            PP(orange::front_val(zr));
            print_type(orange::front_val(zr));
            orange::advance(zr);
        }

    }
}
