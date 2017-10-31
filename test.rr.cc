#include "rr.hh"
#include "../bits.and.pieces/PP.hh"
using namespace rr;
int main () {
    auto r_i = ints(3);
    while(!empty(r_i)) {
        PP(front(r_i));
        advance(r_i);
    }
}
