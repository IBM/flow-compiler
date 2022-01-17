#ifndef H_VEX_H
#define H_VEX_H

#include <type_traits>
#include <functional>
#include <string>
#include <iostream>

namespace vex {

template<typename S>
struct is_string_type: std::is_convertible<S, std::string> {};

struct has_size_test {
    template<typename T, typename has_size = decltype(std::declval<const T&>().size())>
    static std::true_type test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct has_size: decltype(has_size_test::test<T>(0)) {};

struct is_string_vector_test {
    template<typename T, typename has_at = decltype(std::declval<const T&>()[0]),
                         typename has_size = decltype(std::declval<const T&>().size())>
    static is_string_type<decltype(std::declval<const T&>()[0])> test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_string_vector: decltype(is_string_vector_test::test<T>(0)) {};

struct is_string_container_test {
    template<typename T, typename has_begin = decltype(std::declval<const T&>().begin()),
                         typename has_end = decltype(std::declval<const T&>().end()),
                         typename has_size = decltype(std::declval<const T&>().size())>
    static is_string_type<decltype(*std::declval<const T&>().begin())> test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_string_container: decltype(is_string_container_test::test<T>(0)) {};

template<class SC>
struct is_value_type: std::integral_constant<bool, is_string_type<SC>::value || is_string_vector<SC>::value || is_string_container<SC>::value> {};

#define STR_VALUE_NEEDED_ERRM "Must be a string convertible type or a container of a string convertible type"

template<class SC, bool is_str, bool has_sz>
struct value_count_impl {
    static_assert(is_value_type<SC>::value, STR_VALUE_NEEDED_ERRM);
    static size_t count(SC&& sc) { 
        return -1;
    }
};
template<class SC>
struct value_count_impl<SC, false, true> {
    static size_t count(SC&& sc) {
        return sc.size();
    }
};
template<class SC>
struct value_count_impl<SC, true, false> {
    static size_t count(SC&& sc) {
        return 1;
    }
};
template<class SC>
struct value_count_impl<SC, true, true> {
    static size_t count(SC&& sc) {
        return 1;
    }
};
template<class SC>
static inline size_t value_count(SC&& sc) {
    return value_count_impl<SC, is_string_type<SC>::value, has_size<SC>::value>::count(sc);
}
template<class SC>
static inline std::string get_ra_value(SC&& sc, int index) {
    auto count = value_count<SC>(sc);
    switch(count) {
        case 0: 
            return "";
        case 1:
            return sc[0];
    }
    return sc[index >= count? 0: index];
}
template<class SC>
static inline std::string get_it_value(SC&& sc, int index) {
    auto count = value_count<SC>(sc);
    switch(count) {
        case 0: 
            return "";
        case 1:
            return *sc.begin();
    }
    if(index >= count) index = 0;
    for(auto p = sc.begin(), e = sc.end(); p != e; ++p) 
        if(index-- == 0) return *p;
    return "";
}
template<class SC, bool is_s, bool is_ra, bool it_it>
struct get_value_impl {
    static_assert(is_value_type<SC>::value, STR_VALUE_NEEDED_ERRM);
    static std::string value(SC&& sc, int index) { 
        return "";
    }
};
template<class SC>
struct get_value_impl<SC, true, false, false> {
    static std::string value(SC&& sc, int index) { 
        return sc;
    }
};
template<class SC>
struct get_value_impl<SC, false, true, false> {
    static std::string value(SC&& sc, int index) {
        return get_ra_value<SC>(sc, index);
    }
};
template<class SC>
struct get_value_impl<SC, false, true, true> {
    static std::string value(SC&& sc, int index) {
        return get_ra_value<SC>(sc, index);
    }
};
template<class SC>
struct get_value_impl<SC, false, false, true> {
    static std::string value(SC&& sc, int index) {
        return get_it_value<SC>(sc, index);
    }
};
template<class SC>
static inline std::string get_value(SC&& sc, int index) {
    return get_value_impl<SC, is_string_type<SC>::value, is_string_vector<SC>::value, is_string_container<SC>::value>::value(sc, index);
}
struct is_value_dictionary_test {
    template<typename T, typename has_find = decltype(std::declval<const T&>().find("")),
                         typename has_size = decltype(std::declval<const T&>().size())>
    static is_value_type<decltype(std::declval<const T&>().find("")->second)> test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_value_dictionary: decltype(is_value_dictionary_test::test<T>(0)) {};
struct is_value_dictionary_container_test {
    template<typename T, typename has_begin = decltype(std::declval<const T&>().begin()),
                         typename has_end = decltype(std::declval<const T&>().end())>
    static is_value_dictionary<decltype(*std::declval<const T&>().begin())> test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_value_dictionary_container: decltype(is_value_dictionary_container_test::test<T>(0)) {};
template<class SC>
struct is_value_store: std::integral_constant<bool, is_value_dictionary<SC>::value || is_value_dictionary_container<SC>::value> {};

#define VALUE_STORE_NEEDED_ERRM "Must be a map from string to single or multiple strings, or a container of such maps"
#define VALUE_MAP_NEEDED_ERRM "Must be a map from string to single or multiple strings"
#define VALUE_MAP_CONTAINER_NEEDED_ERRM "Must be a container of maps from string to single or multiple strings"
template<class VS, bool is_m, bool is_s>
struct get_dict_value_impl {
    static_assert(is_value_store<VS>::value, VALUE_STORE_NEEDED_ERRM);
    static std::pair<bool, std::string> value(VS&& s, std::string const &name, int index) { 
        return std::make_pair(false, "");
    }
    static int count(VS&& s, std::string const &name) { 
        return -1;
    }
};
template<class VS>
struct get_dict_value_impl<VS, true, false> {
    static_assert(is_value_dictionary<VS>::value, VALUE_MAP_NEEDED_ERRM);
    static std::pair<bool, std::string> value(VS&& sc, std::string const &name, int index) {
        auto fp = sc.find(name.c_str());
        return fp == sc.end()? std::make_pair(false, ""):
            std::make_pair(true, get_value(fp->second, index));
    }
    static int count(VS&& sc, std::string const &name) { 
        auto fp = sc.find(name.c_str());
        return fp == sc.end()? -1: value_count(fp->second);
    }
};
template<class VS>
struct get_dict_value_impl<VS, false, true> {
    static_assert(is_value_dictionary_container<VS>::value, VALUE_MAP_CONTAINER_NEEDED_ERRM);
    static std::pair<bool, std::string> value(VS&& sc, std::string const &name, int index) {
        for(auto p = sc.begin(), e = sc.end(); p != e; ++p) {
            auto r = get_dict_value_impl<decltype(*p), is_value_dictionary<decltype(*p)>::value, is_value_dictionary_container<decltype(*p)>::value>::value(*p, name, index);
            if(r.first) return r;
        }
        return std::make_pair(false, "");
    }
    static int count(VS&& sc, std::string const &name) { 
        for(auto p = sc.begin(), e = sc.end(); p != e; ++p) {
            auto r = get_dict_value_impl<decltype(*p), is_value_dictionary<decltype(*p)>::value, is_value_dictionary_container<decltype(*p)>::value>::count(*p, name);
            if(r >= 0) return r;
        }
        return -1;
    }
};
template<typename M>
static inline 
std::pair<bool, std::string> get_dict_value(std::string const &name, int index, M&& dict) {
    return get_dict_value_impl<M, is_value_dictionary<M>::value, is_value_dictionary_container<M>::value>::value(dict, name, index);
}
template<typename M, typename... MM>
static inline 
std::pair<bool, std::string> get_dict_value(std::string const &name, int index, M&& dict, MM&&... args) {
    auto r = get_dict_value(name, index, dict);
    if(r.first) return r;
    return get_dict_value(name, index, args...);
}
template<typename M>
static inline 
int dict_value_count(std::string const &name, M&& dict) {
    return get_dict_value_impl<M, is_value_dictionary<M>::value, is_value_dictionary_container<M>::value>::count(dict, name);
}
template<typename M, typename... MM>
static inline 
int dict_value_count(std::string const &name, M&& dict, MM&&... args) {
    auto r = dict_value_count(name, dict);
    if(r >= 0) return r;
    return dict_value_count(name, args...);
}

std::pair<int, int> render_varsub(std::ostream &out, char const *templ, char const *templ_end, std::function<std::pair<bool, std::string>(std::string const &, int)> fgv, std::function<int(std::string const &)> fgc);

template <typename... MM> 
static 
int expand(std::ostream &out, char const *in, char const *end_in, MM&&... maps) {
    auto fgv = [&](std::string const &name, int index) -> std::pair<bool, std::string> {
        return get_dict_value(name, index, maps...);
    };
    auto fgc = [&](std::string const &name) -> int {
        return dict_value_count(name, maps...);
    };
    auto rc = render_varsub(out, in, end_in, fgv, fgc); 
    return rc.second;
}
template <typename... MM> 
static 
int expand(std::ostream &out, char const *in, MM&&... maps) {
    return expand(out, in, strchr(in, '\0'), maps...); 
}
template <typename... MM> 
static 
int expand(std::ostream &out, std::string const &in, MM&&... maps) {
    return expand(out, in.c_str(), in.c_str()+in.length(), maps...); 
}
template <typename... MM> 
static 
int expand(std::ostream &out, std::istream &is, MM&&... maps) {
    std::istreambuf_iterator<char> eos;
    std::string templ(std::istreambuf_iterator<char>(is), eos);
    return expand(out, templ, maps...);
}

}
#endif
