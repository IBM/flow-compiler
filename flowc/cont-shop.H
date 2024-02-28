#ifndef H_CONT_SHOP_H
#define H_CONT_SHOP_H

/****
 * Stream shift operators for containers
 */

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

namespace stru {
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
std::ostream &operator << (std::ostream &s, std::pair<A, B> const &p) {
    return s << "<" << p.first << ", "<< p.second << ">";
}

namespace stru {
template<typename T>
inline static 
std::ostream &print_dict(std::ostream &out, T const &m) {
    out << "{";
    char const *sep = "";
    for(auto const &kv: m) {
        out << sep << kv.first << ": " << kv.second;
        sep = ", ";
    }
    out << "}";
    return out;
}
template<typename T>
inline static 
std::ostream &print_container(std::ostream &out, T const &m) {
    out << "[";
    char const *sep = "";
    for(auto const &v: m) {
        out << sep << v;
        sep = ", ";
    }
    out << "]";
    return out;
}
}

#endif
