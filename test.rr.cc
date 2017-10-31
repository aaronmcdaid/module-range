#include "rr.hh"
#include "../bits.and.pieces/PP.hh"
#include<iostream>
#include<vector>
using std:: vector;
using namespace rr;
int main () {
    auto r_i = ints(3);
    while(!empty(r_i)) {
        PP(front(r_i));
        advance(r_i);
    }

    r_i = ints(4);
    for(auto i : r_i) { PP(i); }
}
