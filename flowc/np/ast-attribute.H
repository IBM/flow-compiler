#ifndef H_AST_ATTRIBUTE_H
#define H_AST_ATTRIBUTE_H

#include <map>
#include "ast.H"

namespace ast {

template <class AT> 
struct attribute {
    std::map<int, AT> value;
    AT no_value;

    attribute(AT nv): no_value(nv) {
    }
    AT get(int node, AT default_value) const {
        auto p = value.find(node);
        return p == value.end()? default_value: p->second;
    }
    AT get(int node) const {
        auto p = value.find(node);
        return p == value.end()? no_value: p->second;
    }
    AT operator()(int node) const {
        return get(node);
    }
    AT operator()(int node, AT default_value) const {
        return get(node, default_value);
    }
    bool has(int n) const {
        return value.find(n) != value.end();
    }
    bool update(int n, AT const v) {
        bool rv = value.find(n) == value.end();
        value[n] = v;
        return rv;
    }
    bool put(int n, AT const v) {
        //MASSERT(value.find(n) == value.end() || value.find(n)->second == v) << __func__ <<  " attempting to set attr to " << v << " but node " << n << " already has " << value.find(n)->second << "\n";
        assert(value.find(n) == value.end());
        return update(n, v);
    }
    /*
    bool cat(bool move_attr, AT join, int to, std::set<int> const &from) {
        std::vector<AT> buf;
        if(has(to)) 
            buf.push_back(get(to));
        for(int n: from) if(has(n)) {
            buf.push_back(get(n));
            if(move_attr) value.erase(value.find(n));
        }
        AT nv = stru1::join(buf, join);
        return update(to, nv);
    }
    */
};

}


#endif
