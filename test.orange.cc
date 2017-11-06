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

    {
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
        PP(*(begin(ar)+0));
        PP(*(begin(ar)+1));
        PP(*(begin(ar)+2));
        print_type(*(begin(ar)+2));

        std:: cout << '\n';
        for(auto i = begin(ar); i!=end(ar); ++i) {
            print_type(*i);
            PP(*i);
        }

        std:: sort(begin(ar), end(ar));

        std:: cout << '\n';
        for(auto i = begin(ar); i!=end(ar); ++i) {
            print_type(*i);
            PP(*i);
        }
    }
}
