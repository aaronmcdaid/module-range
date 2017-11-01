#include "rr.hh"
#include "../bits.and.pieces/PP.hh"
#include<iostream>
#include<vector>
using std:: vector;
using std:: string;
using namespace rr;
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
    while(!empty(v_r)) {
        PP(front_val(v_r));
        advance(v_r);
    }
    //*
    as_range(v) |map_range;
    as_range(v) |map_range|[](){};
    auto v_r2 = as_range(v);
    auto mapped = v_r2 |map_range|[](auto && x)->int{
        return x.length();
    };
    while(!empty(mapped)) {
        PP(front_val(mapped));
        advance(mapped);
    }
    //*/

    auto o = as_range(v) |map_range|[](auto && x)->int{
        return -x.length();
    };
    for(;!empty(o);advance(o)) {
        PP(front_val(o));
    }

}
