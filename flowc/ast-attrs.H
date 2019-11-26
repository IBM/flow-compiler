#ifndef H_AST_ATTRS_H
#define H_AST_ATTRS_H

namespace ast {
template<class V> struct attribute {
    std::map<int, V> value;
    V no_value;
    bool put(int n, V const v) {
        assert(value.find(n) == value.end());
        value[n] = v;
        return true;
    }
    bool has(int n) const { return value.find(n) != value.end(); }
    V const &operator ()(int n) const { 
        auto p = value.find(n); return p == value.end()? no_value: p->second;
    }
    V const get(int n, V const def_value) const { 
        auto p = value.find(n); return p == value.end()? def_value: p->second;
    }
    bool has_value(V const some_value) const {
        for(auto &nv: value) if(some_value == nv.second) return true;
        return false;
    }
    bool copy(int from, int to) {
        auto p = value.find(from);
        if(p == value.end()) return false;
        return put(to, p->second);
    }
    std::set<int> nodes() const {
        std::set<int> all;
        for(auto &nv: value) all.insert(nv.first);
        return all;
    }
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
