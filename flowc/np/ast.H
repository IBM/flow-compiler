#ifndef H_AST_H
#define H_AST_H

#include <cstdlib>
#include <iosfwd>
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

struct token {
    int type;
    int line, column;
    std::string text;
    token(): type(ATK_NONE), line(0), column(0) {
    }
    token(int t, int l=0, int c=0, std::string s=""): type(t), line(l), column(c), text(s) {
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
                auto const &node = tree[path.back().first];
                int &cx = path.back().second;
                if(node.children.size() <= cx + 1) {
                    path.push_back(std::make_pair<int, int>((int) node.children[cx], 0));
                    cx += 1;
                    return;
                }
                path.pop_back();
            }
        }
        bool at_end() const {
            return path.size() == 0;
        }
        struct node const &operator *() const {
            return tree[path.back().first-1];
        }
        struct node const *operator ->() const {
            return &tree[path.back().first];
        }
        operator int() const {
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
};
}
#endif
