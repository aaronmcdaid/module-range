#include "orange.hh"
#include "../bits.and.pieces/PP.hh"
#include "../bits.and.pieces/utils.hh"
#include "../module-format/format.hh"
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

#define TEST_ME_AWARE_OF_COMMAS(description, expected)  test_me(__FILE__, __LINE__, description, expected, #expected)
#define TEST_ME(description, ...)  TEST_ME_AWARE_OF_COMMAS(description, ( __VA_ARGS__ ))

template<typename V>
struct test_me_helper_struct
{
    static_assert(!std::is_reference<V>{} ,"");
    std::string m_label;
    V m_expected;

    template<typename F>
    void
    operator^ (F f) const
    {
        auto answer = f();
        bool did_pass = answer == m_expected;
        std:: cout << (did_pass ? " pass \t" : "*FAIL*\t");
        std:: cout << m_label;
        if(!did_pass)
        {
            std::cout
                << "\t\tactual = "
                << answer;
        }
        std:: cout << '\n';
    }
};

template<typename V>
auto
test_me ( char const * file_name
        , int line_number
        , std::string description
        , V && v
        , std::string value_as_text_via_macro
        )
{
    using format:: operator"" _format;
    auto label = "{0}:{1}\t\"{2}\"\t{3}"_format(file_name, line_number, description, value_as_text_via_macro);
    return  test_me_helper_struct<std::remove_reference_t<V>>
            { label, std::forward<V>(v) };
}


int main () {

    {
        // sorting in place
        int ai[] = {4,7,2,9,3,7};
        char ac[] = {'h','e','l','l','o','_'};
        double ad[] = {0.1,0.01,0.001,1,1,1};
        auto ar = zip(ai, ac, ad);

        std:: cout << '\n';
        for(auto i = begin(ar); i!=end(ar); ++i) {
            PP(*i);
        }

        std::swap( *(begin(ar))
                 , *(begin(ar)+1)
                );

        std:: sort(begin(ar), end(ar));

        std:: cout << '\n';
        for(auto i = begin(ar); i!=end(ar); ++i) {
            PP(*i);
        }
    }

    PP(replicate(5, std::string("five")) | collect);
    {
        using ints_t = decltype(ints(42));
        (ints_t[]) { ints(3), ints(100,105) }
            |concat
            |foreach| [](auto &&x ) { PP(x); }
            ;
        ints(4)
            |mapr| intsFrom0
            |memoize
            |concat
            |foreach| [](auto &&x ) {
                PP(x);
            }
        ;

        int a[]{0,1,2};
        auto ar = as_range(a);
        PP(ar|collect);

        std::cout << '\n';
        (decltype(ar)[]) { ar, ar }
            |concat
            //|memoize // memoize is optional here, but it changes the result
            |foreach| [](auto &&x ) {
                PP(x);
                x += 100;
            };
        PP(ar|collect);
    }

    TEST_ME ( "|concat with refs. side-effects."
            , std::vector<int>{200,201,202}
            ) ^ []()
            { return std::vector<int>{200,201,202}; };

    TEST_ME ( "|concat with refs. side-effects."
            , std::vector<int>{200,202}
            ) ^ []()
            { return std::vector<int>{200,201,202}; };
}

