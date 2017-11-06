#include "orange.hh"
#include "../bits.and.pieces/PP.hh"
#include "../bits.and.pieces/utils.hh"
#include<iostream>
#include<vector>
#include<memory>
using std:: vector;
using std:: string;
using namespace orange;
using utils:: operator<<;

template<typename ... T>
void print_type(T && ...) {
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

        zip_val (   as_range(vi)
                ,   as_range(vd)
                ,   ints()  )
            |foreach|
                [](auto && t)
                {
                    PP(t);
                    print_type(t);
                };


        auto zr = zip_ref( as_range(vd), as_range(vd) );
        while(!orange::empty(zr)) {
            PP(orange::front_val(zr));
            print_type(orange::front_val(zr));
            orange::advance(zr);
        }

        zip_ref (   std:: vector<int>{7,6,5,4}
                ,   std:: vector<char>{'a','b','c','d'}
                )
            |foreach|
                [](auto && t) { PP(t); }
        ;
        auto unzipped =
        zip     (   std:: vector<int>{7,6,5,4}
                ,   as_range(std:: vector<char>{'a','b','c','d'})
                ,   ints()
                )
            |unzip_map|
                [](auto && ... tz) {
                    print_type(std::forward<decltype(tz)>(tz)...);
                    PP(std::make_tuple(std::forward<decltype(tz)>(tz)...));
                    return 7.3;
                }
            |collect ;
        PP(unzipped);

    }
    {
        auto ar = as_range( std::array<int, 3> {{ 1,2,3 }} );
        auto s = std::move(ar) | accumulate;
        PP(s);
        constexpr
        auto ar2 = as_range( (int[]) { 1,2,3 } );
        auto p = &orange:: front_ref(ar2);
        PP(p);
        auto ar2_front =
        orange:: front_ref(ar2);
        PP(ar2_front);
        constexpr
        auto yz = as_range( (double[]) { 1.0,2.5,2.5,4.0 } ) | accumulate;
        static_assert(yz == 10,"");
    }
    {
        constexpr
        int array[] = {7,3,4};
        static_assert(14 == (as_range( array ) | accumulate), "");
        auto oar = as_range( (int[]) {7,3,4} );
        PP(oar.m_array[0]);
        PP(oar.m_array[1]);
        PP(oar.m_array[2]);
        auto oar2 = as_range( (std::unique_ptr<double>[])   { std::make_unique<double>(7)
                                                            , std::make_unique<double>(3)
                                                            , std::make_unique<double>(4)
                                                            } );
        PP(*oar2.m_array[0]);
        PP(*oar2.m_array[1]);
        PP(*oar2.m_array[2]);
        as_range( (std::unique_ptr<double>[])   { std::make_unique<double>(7)
                                                , std::make_unique<double>(3)
                                                , std::make_unique<double>(4)
                                                } )
            |foreach|
                [](auto&&x) { PP(*x) ; } ;

    }
    {
        int ai[] = {4};
        zip_val(ai, ints())
            |foreach| [](auto &&x)
                { print_type(x);};
        zip(ai, ints())
            |foreach| [](auto &&x)
                { print_type(x);};
        auto z = zip(ai, ints());

        std::cout<<'\n';
        print_type(orange::front(z));
        print_type(orange::front_val(z));
        //print_type(orange::front_ref(z)); // good that it won't work

        std::cout<<'\n';
        auto zr = zip(ai, ai);
        print_type(orange::front(zr));
        print_type(orange::front_val(zr));
        //print_type(orange::front_ref(zr));
    }
    if(0){
        // sorting in place
        int ai[] = {4,7,2,9,3,7};
        char ac[] = {'h','e','l','l','o'};
        double ad[] = {0.1,0.01,0.001};
        PP(ai| collect);
        auto ar = zip(ai, ac, ad);
        auto b = begin(ar);
        auto e = end(ar);
        PP(e.m_offset);
        //std::sort(begin(ar), end(ar));
        auto B = *b;
        print_type(B);
        while(b != e) {
            PP(*b);
            ++b;
        }

        //std:: sort(begin(ar), end(ar));
    }
}
