#ifndef H_AST_H
#define H_AST_H

#include <cstdlib>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define ATK_NONE 0

#ifndef ATK_INTEGER
#define ATK_INTEGER 1
#endif
#ifndef ATK_STRING
#define ATK_STRING 2
#endif
#ifndef ATK_FLOAT
#define ATK_FLOAT 3
#endif

namespace ast {

struct atxt {
    int file, line, column;
    std::string text;
    atxt(int f=0, int l=0, int c=0, std::string s=""): file(f), line(l), column(c), text(s) {
    }
};
struct token: public atxt {
    int type;
    token(int t=ATK_NONE, int f=0, int l=0, int c=0, std::string s=""): atxt(f, l, c, s), type(t) {
    }
};
struct node {
    int type;
    struct token token;
    std::vector<int> children;
    node(): type(ATK_NONE) {
    }
    node(struct token t): type(t.type), token(t) {
    }
};
template<class T>
struct attribute {
    typedef T value_t;
    std::map<int, value_t> values;
    value_t no_value;

    attribute() {
    }
    attribute(value_t nv): no_value(nv) {
    }
    /** Set the attribute value.
     * Returns false if a previous value was already set.
     */
    template <class V>
    bool update(int n, V &&v) {
        auto p = values.insert_or_assign(n, std::forward<V>(v));
        return p.second;
    }
    /** Set the attribute value.
     * This isn't supposed to overwrite a previously set value.
     */
    template <class V>
    void set(int n, V &&v) {
        auto p = values.emplace(n, std::forward<V>(v));
        assert(p.second);
    }
    value_t get(int n, value_t default_value) const {
        auto p = values.find(n);
        return p == values.end()? default_value: p->second;
    }
    value_t get(int n) const {
        auto p = values.find(n);
        return p == values.end()? no_value: p->second;
    }
    value_t operator ()(int n) const {
        return get(n);
    }
    bool unset(int n) {
        auto p = values.find(n);
        bool have_value = p != values.end();
        if(have_value) values.erase(p);
        return have_value;
    }
    void copy(int from, int to) {
        auto p = values.find(from);
        if(p != values.end()) {
            values.insert_or_assign(to, p->second);
        } else {
            unset(to);
        }
    }
    void move(int from, int to) {
        auto p = values.find(from);
        if(p != values.end()) {
            values.insert_or_assign(to, p->second);
            values.erase(p);
        } else {
            unset(to);
        }
    }
    bool has(int n) const {
        return values.find(n) != values.end();
    }
};
struct tree {
    int root_n = 0;
    std::vector<struct node> store;

    int root() const {
        return root_n == 0? (int) store.size(): root_n;
    }
    /** Reference to node
     */
    struct node const &at(int n) const {
        return store[n-1];
    }
    /** Reference to node's child
     */
    struct node const &atc(int n, int c) const {
        return at(at(n).children[c]);
    }
    /** Reference to a node's subnode 
     */
    struct node const &atp(int n, int c) const {
        return at(at(n).children[c]);
    }
    template <typename C, typename... MC>
    struct node const &atp(int n, C c, MC... mc) const {
        int m = at(n).children[c];
        return atp(m, mc...);
    }
    /** Create new node for token
     */
    int node(token const &token) {
        store.push_back(ast::node(token));
        return (int) store.size();
    } 
    /** Create new node of type
     */
    int node(int type) {
        store.push_back(ast::node(type));
        return (int) store.size();
    }
    /** Create new node with a child
     */
    int node(int type, int child) {
        int r = node(type);
        auto const &c = at(child);
        auto &n = store[r-1];
        n.token.line = c.token.line;
        n.token.column = c.token.column;
        n.children.push_back(child);
        return r;
    }
    /** Create new node with more children
     */
    int node(int type, int child_A, int child_B) {
        int r = node(type, child_A);
        store[r-1].children.push_back(child_B);
        return r;
    }
    int node(int type, int child_A, int child_B, int child_C) {
        int r = node(type, child_A, child_B);
        store[r-1].children.push_back(child_C);
        return r;
    }
    int node(int type, int child_A, int child_B, int child_C, int child_D) {
        int r = node(type, child_A, child_B, child_C);
        store[r-1].children.push_back(child_D);
        return r;
    }
    /** Add a child as first in the list
    */
    int nprepend(int node, int child_node) {
        auto &children = store[node-1].children;
        children.insert(children.begin(), child_node);
        return node;
    }
    /** Add a child to the node's children list
    */
    int nappend(int node, int child_node) {
        store[node-1].children.push_back(child_node);
        return node;
    }
    template <typename... MC>
    int nappend(int node, int child_node, MC... mc) {
        nappend(node, child_node);
        nappend(node, mc...);
        return node;
    }
    /** Add scion's children to node's 
     * <position> not set, append
     * <position> less than 0, clean
    */
    int graft(int node, int scion) {
        auto &children = store[node-1].children;
        auto &nc = store[scion-1].children;
        children.insert(children.end(), nc.begin(), nc.end());
        return node;
    }
    int graft(int node, int scion, int position) {
        auto &children = store[node-1].children;
        auto &nc = store[scion-1].children;
        if(position < 0) {
            children.clear();
            position = 0;
        }
        children.insert(children.begin()+position, nc.begin(), nc.end());
        return node;
    }
    /** Change a node's type
     */
    int chtype(int node, int new_type) {
        if(new_type >= 0) store[node-1].type = new_type;
        return node;
    }
    /** Iterator to walk a tree in top down left most order
     */
    struct iterator {
        std::vector<struct node> const &tree;
        std::vector<std::pair<int, int>> path;

        iterator(struct tree const *astp, int node=-1): tree(astp->store) {
            if(node > 0) 
                path.push_back(std::make_pair(node, 0));
            else if(node == 0)
                path.push_back(std::make_pair(astp->root(), 0));
        }
        bool operator == (iterator const &that) const {
            if(&(this->tree) != &that.tree || this->path.size() != that.path.size())
                return false;
            for(unsigned i=0, e=that.path.size(); i != e; ++i)
                if(this->path[i] != that.path[i])
                    return false;
            return true;
        }
        bool operator != (iterator const &that) const {
            return !(*this == that);
        }
        void next() {
            while(path.size() > 0) {
                auto const &node = tree[path.back().first-1];
                int cx = path.back().second, px = path.size()-1;
                if(node.children.size() >= cx + 1) {
                    path.push_back(std::make_pair<int, int>((int) node.children[cx], 0));
                    path[px].second = cx + 1;
                    return;
                }
                path.pop_back();
            }
        }
        bool at_end() const {
            return path.size() == 0;
        }
        struct node const &at() const {
            return tree[path.back().first-1];
        }
        int operator*() const {
            return path.back().first;
        }
        iterator &operator++() {
            next(); return *this;
        }
        iterator operator++(int) {
            iterator i = *this;
            i.next(); return i;
        }
        int level() const {
            return (int) path.size();
        }
    };
    /** Get an iterator for the subtree at node n 
     */
    iterator begin(int n=0) const {
        return iterator(this, n);
    }
    iterator end() const {
        return iterator(this, -1);
    }
    int node_count(int node=0) const {
        int count = 0;
        for(auto p = begin(node); !p.at_end(); ++p)
            ++count;
        return count;
    }
    bool is_child_of(int parent, int node) const {
        for(auto c: at(parent).children)
            if(c == node)
                return true;
        return false;
    }
    int parent(int node, int subtree=0) const {
        for(auto p = begin(subtree); !p.at_end(); ++p)
            if(is_child_of(*p, node))
                return *p;
        return 0;
    }
    int sibling_index(int node, int subtree=0) const {
        int p = parent(node, subtree);
        if(p == 0) return -1;
        int x = 0;
        for(auto c: at(p).children)
            if(c == node)
                break;
            else
                ++x;
        return x;
    }
    int next_sibling(int node, int subtree=0) const {
        int p = parent(node, subtree);
        if(p == 0) return -1;
        int x = 0;
        for(auto c: at(p).children)
            if(c == node)
                break;
            else
                ++x;
        if(x+1 < at(p).children.size())
            return at(p).children[x+1];
        return -1;
    }
    int sibling_count(int node, int subtree=0) const {
        int p = parent(node, subtree);
        return (int) at(p).children.size();
    }
    int child(int node, int child_index) const {
        if(child_index < 0 || child_index >= at(node).children.size())
            return 0;
        return at(node).children[child_index];
    }
    /** Node accessors
     */
    
    /** Get the first node of type <node_type> that is a subnode of <node>
     */
    int first(int node_type, int node=0) const {
        for(auto p = begin(node); !p.at_end(); ++p)
            if(at(*p).type == node_type)
                return *p;
        return 0;
    }
    /** Get the last node of type <node_type> that is a subnode of <node>
     */
    int last(int node_type, int node=0) const {
        int n = 0;
        for(auto p = begin(node); !p.at_end(); ++p)
            if(at(*p).type == node_type)
                n = *p;
        return n;
    }
    /** Get all the nodes of type <node_type> that is are subnodes of <node>
     */
    std::vector<int> all(int node_type, int node=0) const {
        std::vector<int> n;
        for(auto p = begin(node); !p.at_end(); ++p)
            if(at(*p).type == node_type)
                n.push_back(*p);
        return n;
    }
    /** Get the text value of node
     */
    std::string const &node_text(int node) const {
        return at(node).token.text;
    }
    int node_type(int node) const {
        return at(node).type;
    }
};
}
#endif
