#ifndef H_COT_H
#define H_COT_H
namespace cot {
template <class CONT, class SET> inline static
bool disjunct(CONT const &c, SET const &s) {
    for(auto const &v: s) if(c.find(v) != c.end()) return false;
    return true;
}
template <class CONT, class SET> inline static
bool includes(CONT const &c, SET const &s) {
    for(auto const &v: s) if(c.find(v) == c.end()) return false;
    return true;
}
template <class CONT, class VALUE> inline static
bool contains(CONT const &c, VALUE const &v) {
    return c.find(v) != c.end();
}
template <class DEST, class CONT, class SET> inline static
bool difference(DEST &d, CONT const &a, SET const &b) {
    for(auto const &v: a) if(b.find(v) == b.end()) d.insert(v);
    return d.size() > 0;
}
}
#endif
