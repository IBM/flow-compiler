#ifndef H_COT_H
#define H_COT_H

#include <vector>
#include <algorithm>
namespace cot {
template <class VALUE> inline static
bool contains(std::vector<VALUE> const &c, VALUE const &v) {
    return std::find(c.begin(), c.end(), v) != c.end();
}
template <class CONT, class VALUE> inline static
bool contains(CONT const &c, VALUE const &v) {
    return c.find(v) != c.end();
}
template <class DEST, class CONT, class SET> inline static
bool difference(DEST &d, CONT const &a, SET const &b) {
    for(auto const &v: a) if(!contains(b, v)) d.insert(v);
    return d.size() > 0;
}
template <class CONT, class SET> inline static
bool disjunct(CONT const &c, SET const &s) {
    for(auto const v: s) if(contains(c,v)) return false;
    return true;
}
template <class CONT, class SET> inline static
bool includes(CONT const &c, SET const &s) {
    for(auto const v: s) if(!contains(c, v)) return false;
    return true;
}
template <class CONT, class VALUE> inline static 
bool has_value(CONT const &c, VALUE const &v) {
    for(auto const p: c) 
        if(p.second == v)
            return true;
    return false;
}
template <class CONT> inline static
CONT join(CONT const &c, CONT const &d) {
    CONT r; r.insert(c.begin(), c.end()); r.insert(d.begin(), d.end());
    return r;
}
}
#endif
