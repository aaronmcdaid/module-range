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
template<typename T>
auto type_as_string(T) {
    string t = __PRETTY_FUNCTION__;
    t = t.substr( t.find('[')+1 ,string::npos);
    t = t.substr( t.find('=')+1 ,string::npos);
    if(t.back() == ' ') t=t.substr(1);
    if(t.back() == ']') t=t.substr(0, t.length()-1);
    return t;
}

#define TEST_ME_AWARE_OF_COMMAS(description, expected)  test_me(__FILE__, __LINE__, description, expected, #expected)
#define TEST_ME(description, ...)  TEST_ME_AWARE_OF_COMMAS(description, ( __VA_ARGS__ ))

auto checker_for__has_equality        = [](auto&&x,auto&&y)->decltype(void( x==y )){};
auto suppress_a_linker_warning_about_unused() { return &checker_for__has_equality; }

template<typename T, typename U> constexpr bool
has_equality     = orange_utils:: is_invokable_v<decltype(checker_for__has_equality), T, U>;

template<typename V>
struct test_me_helper_struct
{
    static_assert(!std::is_reference<V>{} ,"");
    std::string m_label;
    std::string m_text_of_expected;
    V m_expected;


    template<typename F>
    auto
    operator^ (F f) const
    -> std::enable_if_t< has_equality< decltype(f()) , decltype(m_expected) >>
    {
        auto answer = f();
        bool did_pass = answer == m_expected;
        std:: cout << (did_pass ? " pass \t" : "*FAIL*\t");
        std:: cout << m_label;

        std:: cout << "\t" << m_text_of_expected;
        if(!did_pass)
        { std::cout << "\t!!!===\t" << answer; }

        std:: cout << '\n';
    }

    template<typename F>
    auto
    operator^ (F f) const
    -> std::enable_if_t<!has_equality< decltype(f()) , decltype(m_expected) >>
    {
        std:: cout << "*FAIL*\t";
        std:: cout << m_label;
        std:: cout
            << "\t"
            << type_as_string(m_expected)
            << "\t!!!!====\t" << type_as_string(f());

        std:: cout << "\t" << m_text_of_expected;
        std::cout
                << "\t\tvalues: {{ "
                << f()
                << " }}"
                ;

        std::cout << '\n';
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
    auto label = "{0}:{1}\t\"{2}\""_format(file_name, line_number, description);
    return  test_me_helper_struct<std::remove_reference_t<V>>
            { label
            , value_as_text_via_macro
            , std::forward<V>(v)
            };
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

    TEST_ME ( "|concat with refs. side-effects."
            , std::vector<int>{200,202}
            ) ^ []()
            {
                int a[]{1,2,3};
                return
                a
                    |mapr|
                        [](auto x){return x * 1.5;}
                    |collect;
            };
}

