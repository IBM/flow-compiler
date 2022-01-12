#ifndef H_AST_ATTRS_H
#define H_AST_ATTRS_H

#include <type_traits>
#include "massert.H"

namespace ast {
template<class V> struct attribute {
    typedef V value_t;
    std::map<int, value_t> value;
    V no_value;

    bool put(int n, value_t const v) {
        MASSERT(value.find(n) == value.end() || value.find(n)->second == v) << __func__ <<  " attempting to set attr to " << v << " but node " << n << " already has " << value.find(n)->second << "\n";
        value[n] = v;
        return true;
    }
    bool update(int n, value_t const v) {
        value[n] = v;
        return true;
    }
    bool has(int n) const { return value.find(n) != value.end(); }
    value_t const get(int n, value_t const def_value) const { 
        auto p = value.find(n); return p == value.end()? def_value: p->second;
    }
    value_t const operator()(int n, V const def_value) const { 
        return get(n, def_value);
    }
    value_t const &operator ()(int n) const { 
        auto p = value.find(n); return p == value.end()? no_value: p->second;
    }
    bool has_value(value_t const some_value) const {
        for(auto &nv: value) if(some_value == nv.second) return true;
        return false;
    }
    bool copy(int from, int to) {
        auto p = value.find(from);
        if(p == value.end()) return false;
        return put(to, p->second);
    }
    bool move(int to, int from) {
        auto p = value.find(from);
        if(p == value.end()) return false;
        value[to] = p->second;
        value.erase(value.find(from));
        return true;
    }
    bool cat(bool move_attr, V join, int to, int from) {
        if(!has(from))
            return false;
        if(!has(to))
            return move_attr? move(to, from): copy(from, to);
        update(to, value.find(to)->second + join + value.find(from)->second);
        if(move_attr)
            value.erase(value.find(from));
        return true;
    }
    bool cat(bool move_attr, V join, int to, std::set<int> const &from) {
        bool r = false;
        for(int f: from)
            if(cat(move_attr, join, to, f)) r = true;
        return r;
    }
    std::set<int> nodes() const {
        std::set<int> all;
        for(auto &nv: value) all.insert(nv.first);
        return all;
    }
    attribute(value_t p): no_value(p) { }
};
template <typename AT> 
static bool has_attr(int n, AT const &attrs) {
    return attrs.has(n);
}
template<typename AT, typename... ATArgs>
static bool has_attr(int n, AT const &first, ATArgs const &... args) {
    return has_attr(n, first) || has_attr(n, args...);
}
template <typename AT> 
static void copy_attr(int from, int to, AT &attrs) {
    attrs.copy(from, to);
}
template<typename AT, typename... ATArgs>
static void copy_attr(int from, int to, AT &first, ATArgs &... args) {
    copy_attr(from, to, first); copy_attr(from, to, args...);
}
}
#endif
