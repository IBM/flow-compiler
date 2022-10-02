#ifndef H_VEX_H
#define H_VEX_H

#if defined(VEX_DEBUG) 
#define VEX_DEBUG_DEFAULT true
#else
#define VEX_DEBUG_DEFAULT false
#endif

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
    return value_count_impl<SC, is_string_type<SC>::value, has_size<SC>::value>::count(std::forward<SC>(sc));
}
template<class SC>
static inline std::string get_ra_value(SC&& sc, int index) {
    auto count = value_count<SC>(std::forward<SC>(sc));
    switch(count) {
        case 0: 
            return "";
        case 1:
            return sc[0];
    }
    if(index >= count)
        return "";
    return sc[index];
}
template<class SC>
static inline std::string get_it_value(SC&& sc, int index) {
    auto count = value_count<SC>(std::forward<SC>(sc));
    switch(count) {
        case 0: 
            return "";
        case 1:
            return *sc.begin();
    }
    if(index >= count) 
        return "";
    for(auto p = sc.begin(), e = sc.end(); p != e; ++p) 
        if(index-- == 0) return *p;
    return "";
}
template<class SC, bool is_singleton, bool is_random_access, bool is_iterator>
struct get_value_impl {
    static_assert(is_value_type<SC>::value, STR_VALUE_NEEDED_ERRM);
    static std::string value(SC&& sc, int index) { 
        return "";
    }
};
/**
 * String convertible singleton
 */
template<class SC>
struct get_value_impl<SC, true, false, false> {
    static std::string value(SC&& sc, int index) { 
        return sc;
    }
};
/**
 * Random access string container
 */
template<class SC>
struct get_value_impl<SC, false, true, false> {
    static std::string value(SC&& sc, int index) {
        return get_ra_value<SC>(std::forward<SC>(sc), index);
    }
};
template<class SC>
struct get_value_impl<SC, false, true, true> {
    static std::string value(SC&& sc, int index) {
        return get_ra_value<SC>(std::forward<SC>(sc), index);
    }
};
/**
 * Iterable string container 
 */
template<class SC>
struct get_value_impl<SC, false, false, true> {
    static std::string value(SC&& sc, int index) {
        return get_it_value<SC>(std::forward<SC>(sc), index);
    }
};
/**
 * Get value from a string container
 */
template<class SC>
static inline std::string get_value(SC&& sc, int index) {
    return get_value_impl<SC, is_string_type<SC>::value, is_string_vector<SC>::value, is_string_container<SC>::value>::value(std::forward<SC>(sc), index);
}
struct is_value_dictionary_test {
    template<typename T, typename has_find = decltype(std::declval<const T&>().find("")),
                         typename has_end = decltype(std::declval<const T&>().end())>
    static is_value_type<decltype(std::declval<const T&>().find("")->second)> test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_value_dictionary: decltype(is_value_dictionary_test::test<T>(0)) {};

struct is_value_accessor_test {
    template<typename T, typename has_get = decltype(std::declval<const T&>().get("")),
                         typename has_end = decltype(std::declval<const T&>().end())>
    static is_value_type<decltype(true? std::declval<const T&>().get(""): std::declval<const T&>().end())> test(int);
    template<typename...>
    static std::false_type test(...);
};
template<typename T>
struct is_value_accessor: decltype(is_value_accessor_test::test<T>(0)) {};

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
struct is_value_store: std::integral_constant<bool, is_value_dictionary<SC>::value || is_value_accessor<SC>::value || is_value_dictionary_container<SC>::value> {};

#define VALUE_STORE_NEEDED_ERRM "Must be a map from string to single or multiple strings, or a container of such maps"
#define VALUE_MAP_NEEDED_ERRM "Must be a map from string to single or multiple strings"
#define VALUE_MAP_CONTAINER_NEEDED_ERRM "Must be a container of maps from string to single or multiple strings"
template<class VS, bool is_map, bool is_array, bool is_container_container>
struct get_dict_value_impl {
    static_assert(is_value_store<VS>::value, VALUE_STORE_NEEDED_ERRM);
    static std::pair<int, std::string> getv(VS&& s, std::string const &name, int index) { 
        return std::make_pair(-1, "");
    }
    static int count(VS&& s, std::string const &name) { 
        return -1;
    }
};
/**
 * Container with std::map like accessors
 */
template<class VS>
struct get_dict_value_impl<VS, true, false, false> {
    static_assert(is_value_dictionary<VS>::value, VALUE_MAP_NEEDED_ERRM);
    static std::pair<int, std::string> getv(VS&& sc, std::string const &name, int index) { 
        auto &&fp = sc.find(name.c_str());
        return fp == sc.end()? std::make_pair(-1, ""):
            std::make_pair(value_count(fp->second), get_value(fp->second, index));
    }
    static int count(VS&& sc, std::string const &name) { 
        auto &&fp = sc.find(name.c_str());
        return fp == sc.end()? -1: value_count(fp->second);
    }
};
/**
 * Container with get() accessor
 */
template<class VS>
struct get_dict_value_impl<VS, false, true, false> {
    static_assert(is_value_accessor<VS>::value, VALUE_MAP_NEEDED_ERRM);
    static std::pair<int, std::string> getv(VS&& sc, std::string const &name, int index) { 
        auto &&fp = sc.get(name.c_str());
        return fp == sc.end()? std::make_pair(-1, ""):
            std::make_pair(value_count(fp), get_value(fp, index));
    }
    static int count(VS&& sc, std::string const &name) {
        auto &&fp = sc.get(name.c_str());
        return fp == sc.end()? -1: value_count(fp);
    }
};
/**
 * Container iterable with begin() and end() of other containers
 */
template<class VS>
struct get_dict_value_impl<VS, false, false, true> {
    static_assert(is_value_dictionary_container<VS>::value, VALUE_MAP_CONTAINER_NEEDED_ERRM);
    static std::pair<int, std::string> getv(VS&& sc, std::string const &name, int index) { 
        for(auto p = sc.begin(), e = sc.end(); p != e; ++p) {
            auto r = get_dict_value_impl<decltype(*p), 
                 is_value_dictionary<decltype(*p)>::value, 
                 is_value_accessor<decltype(*p)>::value, 
                 is_value_dictionary_container<decltype(*p)>::value>::getv(*p, name, index);
            if(r.first >= 0) return r;
        }
        return std::make_pair(-1, "");
    }
    static int count(VS&& sc, std::string const &name) { 
        for(auto p = sc.begin(), e = sc.end(); p != e; ++p) {
            auto r = get_dict_value_impl<decltype(*p), 
                 is_value_dictionary<decltype(*p)>::value, 
                 is_value_accessor<decltype(*p)>::value, 
                 is_value_dictionary_container<decltype(*p)>::value>::count(*p, name);
            if(r >= 0) return r;
        }
        return -1;
    }
};
template<typename M>
static inline 
std::pair<int, std::string> dict_value_get(std::string const &name, int index, M&& dict) {
    return get_dict_value_impl<M, is_value_dictionary<M>::value, is_value_accessor<M>::value, is_value_dictionary_container<M>::value>::getv(std::forward<M>(dict), name, index);
}
template<typename M, typename... MM>
static inline 
std::pair<int, std::string> dict_value_get(std::string const &name, int index, M&& dict, MM&&... args) {
    auto r = dict_value_get(name, index, std::forward<M>(dict));
    if(r.first >= 0) return r;
    return dict_value_get(name, index, std::forward<MM>(args)...);
}

template <typename... MM>
static inline
std::function<std::pair<int, std::string>(std::string const &, int)> make_da(MM&&... maps) {
    auto fgv = [&](std::string const &name, int index) -> std::pair<int, std::string> {
        return dict_value_get(name, index, std::forward<MM>(maps)...);
    };
    return fgv;
}
extern char const *default_escape_strings[4];
/**
 * Expand variables in a template into the output stream.
 * Returns the number of errror encountered.
 * 
 * out  -- the output stream
 * templ, templ_end  -- the template endpoints
 * fgv  -- the variable accessor that returns the value and the number of available values
 * label -- label to display in error messages
 * bo, bi, ei, eo  -- macro delimiters
 * esp  -- pointer to the errorstream set to nullptr to disable error reporting
 *
 */
int pand(std::ostream &out,     // output stream
        char const *templ, char const *templ_end,  // template
        std::function<std::pair<int, std::string>(std::string const &, int)> fgv, // variable accessor
        std::string label = "vex", 
        bool debug=VEX_DEBUG_DEFAULT,
        char const *bo=vex::default_escape_strings[0], char const *bi=vex::default_escape_strings[1], char const *ei=vex::default_escape_strings[2], char const *eo=vex::default_escape_strings[3], // macro delimiters 
        std::ostream *esp = &std::cerr
        );
template <typename... MM> 
static inline
int expandx(std::ostream &out, std::string const &in, MM&&... maps) {
    auto rc = pand(out, in.c_str(), in.c_str()+in.length(), make_da(std::forward<MM>(maps)...)); 
    return rc;
}
template <typename... MM> 
static inline
int expand(std::ostream &out, std::string const &in, std::string const &label, MM&&... maps) {
    auto rc = pand(out, in.c_str(), in.c_str()+in.length(), make_da(std::forward<MM>(maps)...), label); 
    return rc;
}
}
#endif
