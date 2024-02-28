#ifndef H_JSON_OSTREAM_H
#define H_JSON_OSTREAM_H

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "cont-shop.H"

namespace stru {
std::string json_escape(std::string const &s);

template <typename STR, std::enable_if_t<stru::is_string_type<STR>::value, bool> = true>
inline static
std::string json_string(STR s) {
    return json_escape(s);
}
template <typename INT, std::enable_if_t<std::conjunction<std::is_integral<INT>, std::negation<std::is_same<INT, char>>> ::value, bool> = true>
inline static
std::string json_string(INT i) {
    return json_escape(std::to_string(i));
}
template <typename CHR, std::enable_if_t<std::is_same<CHR, char>::value, bool> = true>
inline static
std::string json_string(CHR c) {
    return json_escape(std::string(1, c));
}
class json_ostream {
public: 
    std::ostream &osref;
    json_ostream(std::ostream &o): osref(o) {
    }
};
template<typename T>
inline static 
json_ostream &json_dict(json_ostream &out, T const &m);
template<typename T>
inline static 
json_ostream &json_array(json_ostream &out, T const &m);
}

template <typename T, std::enable_if_t<stru::is_dict<T>::value, bool> = true>
inline static 
stru::json_ostream &operator << (stru::json_ostream &out, T const &m) {
    return stru::json_dict(out, m);
}
template <typename T, std::enable_if_t<std::conjunction< stru::is_container<T>, std::negation<stru::is_dict<T>> >::value, bool> = true>
inline static 
stru::json_ostream &operator << (stru::json_ostream &out, T const &m) {
    return stru::json_array(out, m);
}
template <typename STR, std::enable_if_t<stru::is_string_type<STR>::value, bool> = true>
inline static 
stru::json_ostream &operator << (stru::json_ostream &out, STR const s) {
    out.osref << stru::json_escape(s);
    return out;
}
/******
 * Operator to delegate to the std::stream operator
 */
template <typename GEN, std::enable_if_t<std::negation<std::disjunction< stru::is_string_type<GEN>, stru::is_container<GEN> >>::value, bool> = true>
inline static
stru::json_ostream &operator<<(stru::json_ostream &out, GEN v) {
    out.osref << v;
    return out;
}
template <typename A, typename B> 
inline static 
stru::json_ostream &operator << (stru::json_ostream &s, std::pair<A, B> const &p) {
    s.osref << "["; s << p.first; s.osref  << ", "; s << p.second; s.osref << "]";
    return s;
}
namespace stru {
template<typename T>
inline static 
json_ostream &json_dict(json_ostream &out, T const &m) {
    out.osref << "{";
    char const *sep = "";
    for(auto const &kv: m) {
        out.osref << sep << stru::json_string(kv.first) << ":"; out << kv.second;
        sep = ",";
    }
    out.osref << "}";
    return out;
}
template<typename T>
inline static 
json_ostream &json_array(json_ostream &out, T const &m) {
    out.osref << "[";
    char const *sep = "";
    for(auto const &v: m) {
        out.osref << sep; out << v;
        sep = ",";
    }
    out.osref << "]";
    return out;
}
}

#endif
