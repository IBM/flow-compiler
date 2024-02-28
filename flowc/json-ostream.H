#ifndef H_JSON_OSTREAM_H
#define H_JSON_OSTREAM_H

#include <ostream>
#include <string>
#include <vector>
#include <map>

namespace stru {
std::string json_escape(const std::string &s);

inline static
std::ostream &to_json(std::ostream &out, std::string const *x) { return out << json_escape(*x); }
inline static
std::ostream &to_json(std::ostream &out, std::string const &x) { return out << json_escape(x); }
inline static
std::ostream &to_json(std::ostream &out, char const *x) { return out << json_escape(x); }
template <class T> inline static 
std::ostream &to_json(std::ostream &out, T x) { return out << x; }

template <class E> 
std::ostream &to_json(std::ostream &out, std::vector<E> const &v) { 
    out << "[";
    if(v.size() > 0) to_json(out, v[0]);
    for(unsigned i = 1, e = v.size(); i < e; ++i) {
        out << ","; to_json(out, v[i]);
    }
    out << "]";
    return out;
}
template <class V> 
std::ostream &to_json(std::ostream &out, std::map<std::string, V> const &v) { 
    out << "{";
    char const *sep = "";
    for(auto const &p: v) {
        out << sep; to_json(out, p.first); out << ":"; to_json(out, p.second); sep = ",";
    }
    out << "}";
    return out;
}
}

#endif
