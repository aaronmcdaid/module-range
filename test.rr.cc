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
        PP(front(r_i));
        advance(r_i);
    }

    r_i = ints(4);
    for(auto i : r_i) { PP(i); }

    vector<string> v{"hi", "world", "of", "ranges"};
    auto v_r = as_range(v);
    while(!empty(v_r)) {
        PP(front(v_r));
        advance(v_r);
    }
}
