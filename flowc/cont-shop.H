#ifndef H_CONT_SHOP_H
#define H_CONT_SHOP_H

/****
 * Stream shift operators for containers
 */

#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stru {

const auto container_left = '[';
const auto container_right = ']';
const auto container_sep = ", ";

const auto dict_left = '{';
const auto dict_right = '}';
const auto dict_sep = ", ";
const auto dict_element_left = "";
const auto dict_element_right = "";
const auto dict_element_sep = ": ";

const auto pair_left = '<';
const auto pair_right = '>';
const auto pair_sep = ", ";

const auto tuple_left = '(';
const auto tuple_right = ')';
const auto tuple_sep = ", ";

template<typename S>
struct is_string_type: std::is_convertible<S, std::string> {};

template <typename>
struct is_pair_type: std::false_type {};
template <typename T, typename U>
struct is_pair_type<std::pair<T, U>>: std::true_type {};

/*** 
 * A container is any iterable (has begin(), end()) that is not a string
 */
struct is_container_test { // i.e. has begin() end () and size() but is not a string()
    template<typename T, 
        typename has_begin = decltype(std::declval<const T&>().begin()),
        typename has_end = decltype(std::declval<const T&>().end()),
        typename has_size = decltype(std::declval<const T&>().size())
    >
    static std::negation<is_string_type<T>> test(int);
    static std::true_type test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_container: decltype(is_container_test::test<T>(0)) {};

/***
 * A map is a container with an [] operator and a key and a mapped type.
 */
struct is_map_test { 
    template<typename T, 
        typename has_begin = decltype(std::declval<const T&>().begin()),
        typename has_end = decltype(std::declval<const T&>().end()),
        typename has_key_type = typename T::key_type,
        typename has_mapped_type = typename T::mapped_type,
        typename has_mapop = decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])
    >
    static is_pair_type<typename T::value_type> test(int);
    static std::true_type test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_map: decltype(is_map_test::test<T>(0)) {};

/***
 * A dictionary is a a map with a string like or intergal key.
 */
struct is_dict_test { 
    template<typename T, 
        typename has_begin = decltype(std::declval<const T&>().begin()),
        typename has_end = decltype(std::declval<const T&>().end()),
        typename has_key_type = typename T::key_type,
        typename has_mapped_type = typename T::mapped_type,
        typename has_mapop = decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])
    >
    static std::conjunction<is_pair_type<typename T::value_type>, std::disjunction<std::is_integral<typename T::key_type>, is_string_type<typename T::key_type>>> test(int);
    static std::true_type test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_dict: decltype(is_dict_test::test<T>(0)) {};
template<typename T>
inline static std::ostream &print_dict(std::ostream &out, T const &m);
template<typename T>
inline static std::ostream &print_container(std::ostream &out, T const &m);
/***
 * Helper to allow redefinition of separator types
 */
template <typename S>
inline static 
std::ostream &outsep(std::ostream &out, int n, S sep) {
    if(n != 1) out << sep;
    return out;
}
}

template <typename T, std::enable_if_t<stru::is_dict<T>::value, bool> = true>
inline static 
std::ostream &operator << (std::ostream &out, T const &m) {
    return stru::print_dict(out, m);
}
template <typename T, std::enable_if_t<std::conjunction< stru::is_container<T>, std::negation<stru::is_dict<T>> >::value, bool> = true>
inline static 
std::ostream &operator << (std::ostream &out, T const &m) {
    return stru::print_container(out, m);
}
template <typename A, typename B> 
inline static 
std::ostream &operator << (std::ostream &out, std::pair<A, B> const &p) {
    return out << stru::pair_left << p.first << stru::pair_sep << p.second << stru::pair_right;
}
template<typename... T>
inline static 
std::ostream &operator<<(std::ostream &out, std::tuple<T...> const &tu) {
    std::apply(
        [&out](T const &... ta) {
            out << stru::tuple_left;
            int n = 0;
            ((stru::outsep(out, ++n, stru::tuple_sep) << ta), ...);
            out << stru::tuple_right;
        }, tu
    );
    return out;
}

namespace stru {
template<typename T>
inline static 
std::ostream &print_dict(std::ostream &out, T const &m) {
    out << stru::dict_left;
    int n = 0;
    for(auto const &kv: m) 
        stru::outsep(out, ++n, stru::dict_sep)
            << stru::dict_element_left << kv.first 
            << stru::dict_element_sep << kv.second 
            << stru::dict_element_right;
    out << stru::dict_right;
    return out;
}
template<typename T>
inline static 
std::ostream &print_container(std::ostream &out, T const &m) {
    out << stru::container_left;
    int n = 0;
    for(auto const &v: m) 
        stru::outsep(out, ++n, stru::container_sep) << v;
    
    out << stru::container_right;
    return out;
}
}
#endif
