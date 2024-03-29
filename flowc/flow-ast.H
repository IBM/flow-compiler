#ifndef H_FLOW_AST_H
#define H_FLOW_AST_H

#include <iosfwd>
#include <vector>
#include <map>
#include <string>
#include <google/protobuf/compiler/importer.h>
#ifndef FTK_ID
#include "flow-parser.h"
#endif
#include "massert.H"
#include "ast-attrs.H"
#include "stru1.H"
#include "cot.H"

// AST node types used
enum {
    FTK_ACCEPT=5000,
    FTK_SYNTAX_ERROR,
    FTK_flow,
    FTK_blck,
    FTK_lblk,
    FTK_elem,
    FTK_enum,
    FTK_fldm, 
    FTK_fldd,
    FTK_fldx,
    FTK_fldr,
    FTK_dtid
};
inline static constexpr 
bool is_value_type(int type) {
    return type == FTK_INTEGER || type == FTK_STRING || type == FTK_FLOAT;
}
char const *node_name(int i);
struct flow_token {
    int type, line, column, end_column;
    std::string text;
    int64_t integer_value;
    double float_value;
};
struct flow_ast_node {
    int type;
    flow_token token;
    std::vector<int> children;
};
std::ostream &operator << (std::ostream &out, flow_ast_node const &node);


struct flow_ast {
    std::vector<flow_ast_node> store;
    /***********  Node attributes **************************
     */
    // grpc descriptors associated with this AST node
    ast::attribute<google::protobuf::Descriptor const *> message_descriptor;
    ast::attribute<google::protobuf::MethodDescriptor const *> method_descriptor;
    ast::attribute<google::protobuf::Descriptor const *> input_descriptor;
    ast::attribute<google::protobuf::FieldDescriptor const *> field_descriptor;
    // if this field can be evaluated to an enum, the descriptor of the value
    ast::attribute<google::protobuf::EnumValueDescriptor const *> enum_descriptor;
    // label associated with this node
    ast::attribute<std::string> name;
    // node type name (node, bexp, etc.)
    ast::attribute<std::string> type;
    // description extracted from leading comments
    ast::attribute<std::string> description;
    ast::attribute<int> refcount;
    // if this node can be evaluated to a constant, one of FTK_INETEGR, FTK_STRING, FTK_FLOAT
    ast::attribute<int> value_type;
    // dimension of the data at this node
    ast::attribute<int> dimension;
    // group id for nodes deployment
    ast::attribute<std::string> group;

    /** Symbol table 
     */
    std::map<std::string, int> defines;
    /** Errors accumulated while parsing. 
     */
    std::vector<std::pair<int, std::string>> syntax_errors;

    int define_var(std::string const &var_name, int node) {
        name.put(node, var_name);
        return defines[var_name] = node;
    }
    int lookup_var(std::string const &name) const {
        auto f = defines.find(name); 
        return f == defines.end()? 0: f->second;
    }
    struct ast_iterator {
        flow_ast const *tree;
        std::vector<std::pair<unsigned short, int>> path;
        int current_node;
        ast_iterator(): tree(nullptr), current_node(0) {
        }
        ast_iterator(flow_ast const *t, int cn): tree(t), current_node(cn) {
        }
        void print(std::ostream &out) const {
            out << "<" << current_node << ":";
            for(auto &cp: path)
                out << "[" << cp.second << "->" << cp.first << "]";
            out << ">";
        }
        bool operator < (ast_iterator const &o) const {
            if(current_node == o.current_node) return false;
            if(o.current_node == 0) return true;
            for(int i = 0, e = std::min(path.size(), o.path.size()); i < e; ++i) {
                if(path[i].first > o.path[i].first) return false;
                if(path[i].first < o.path[i].first) return true;
            }
            return path.size() < o.path.size();
        }
        bool operator > (ast_iterator const &i) const {
            return i < *this;
        }
        bool operator <= (ast_iterator const &i) const {
            return !(i < *this);
        }
        bool operator >= (ast_iterator const &i) const {
            return !(*this < i);
        }
        bool operator != (ast_iterator const &i) const {
            return *this < i || i < *this;
        }
        bool operator == (ast_iterator const &i) const {
            return !(*this != i);
        }
        int operator *() const {
            return current_node;
        }
        int depth() const {
            if(current_node == 0) return -1;
            return (int) path.size();
        }
        ast_iterator &operator ++() {
            if(current_node == 0) 
                return *this;

            int n = current_node;
            if(tree->at(n).children.size() > 0) {
                path.push_back(std::make_pair(0u, n)); 
                current_node = tree->at(n).children[0];
            } else while(path.size() > 0) {
                n = path.back().second;
                int c = path.back().first;
                path.pop_back();
                if(tree->at(n).children.size() > c + 1) {
                    path.push_back(std::make_pair(c+1, n)); 
                    current_node = tree->at(n).children[c+1];
                    break;
                }
            }
            if(path.size() == 0) {
                current_node = 0;
            }
            return *this;
        }
        ast_iterator &operator --() {
            if(current_node == 0) 
                return *this;
            current_node = 0;
            if(path.size() == 0)
                return *this;
            auto p = path.back().second;
            auto &c = path.back().first;
            if(c > 0) {
                --c;
                current_node = tree->at(p).children[c];
                while(true) {
                    unsigned short cncs = tree->at(current_node).children.size();
                    if(cncs == 0) break;
                    path.push_back(std::make_pair(cncs-1, current_node));
                    current_node = tree->at(path.back().second).children[path.back().first];
                }
                return *this;
            }
            if(path.size() == 1) {
                current_node = path.back().second;
                path.pop_back();
            } else {
                path.pop_back();
                current_node = tree->at(path.back().second).children[path.back().first];
            }
            return *this;
        }
    };
    struct ast_range {
        flow_ast const *tree;
        int subtree_root;
        ast_range(flow_ast const *t, int str): tree(t), subtree_root(str) {
        }
        ast_iterator begin() const {
           return tree->begin(subtree_root);
        }
        ast_iterator end() const {
            return tree->end(subtree_root);
        }
    };
    /** Return a pointer to the first element in the tree
     */
    ast_iterator begin(int root=0) const {
        if(root == 0) root = ast_root();
        return ast_iterator(this, root);
    }
    /** Return a pointer to the last element in the tree
     */
    ast_iterator rbegin(int root=0) const {
        ast_iterator p = begin(root);
        ast_iterator r = end();
        while(*p) {
            r = p; 
            ++p;
        }
        return r;
    }
    ast_iterator end(int root=0) const {
        return ast_iterator();
    }
    ast_iterator rend(int root=0) const {
        return ast_iterator();
    }
    ast_range subtree(int node) const {
        return ast_range(this, node);
    }
    // Returns an iterator positioned at node 'n'
    ast_iterator pnode(int n, int root=0) const {
        ast_iterator p = begin(root);
        if(n <= 0) return p;
        ast_iterator e = end();
        for(; p != e; ++p)
            if(*p == n) return p;
        return e;
    }
    flow_ast(): message_descriptor(nullptr), method_descriptor(nullptr), input_descriptor(nullptr), field_descriptor(nullptr), enum_descriptor(nullptr), 
            name(""), type(""), description(""), refcount(0), value_type(0), dimension(0), group("") {
            }
    /***
     * attribute manipulators
     */
    bool has_attributes(int n) const {
        return has_attr(n, name, type, description, refcount, message_descriptor, method_descriptor, input_descriptor, field_descriptor, enum_descriptor, value_type, dimension, group);
    }
    void copy_attributes(int from, int to) {
        copy_attr(from, to, name, type, description, refcount, message_descriptor, method_descriptor, input_descriptor, field_descriptor, enum_descriptor, value_type, dimension, group);
    }
    int mk_node(flow_token const &token) {
        store.push_back(flow_ast_node());
        store.back().type = token.type;
        store.back().token = token;
        return (int) store.size();
    }
    int node(int type) {
        store.push_back(flow_ast_node());
        store.back().type = type;
        return (int) store.size();
    }
    int node(int type, int child) {
        auto r = node(type);
        store.back().token.line = store[child-1].token.line;
        store.back().token.column = store[child-1].token.column;
        store.back().token.end_column = store[child-1].token.end_column;
        store.back().children.push_back(child);
        return (int) store.size();
    }
    int node(int type, int child_A, int child_B) {
        auto r = node(type, child_A);
        store.back().children.push_back(child_B);
        return r;
    }
    int node(int type, int child_A, int child_B, int child_C) {
        auto r = node(type, child_A, child_B);
        store.back().children.push_back(child_C);
        return r;
    }
    int node(int type, int child_A, int child_B, int child_C, int child_D) {
        auto r = node(type, child_A, child_B, child_C);
        store.back().children.push_back(child_D);
        return r;
    }
    int nprepend(int node, int child_node) {
        assert(node > 0);
        auto &children = store[node-1].children;
        children.insert(children.begin(), child_node);
        return node;
    }
    int nappend(int node, int child_node) {
        assert(node > 0);
        store[node-1].children.push_back(child_node);
        return node;
    }
    flow_ast_node const &at(int node) const {
        assert(node > 0);
        return store[node-1];
    }
    flow_ast_node const &atc(int node, int child) const {
        if(child < 0) child += at(node).children.size();
        return at(at(node).children[child]);
    }
    int ast_root() const {
        return (int) store.size();
    }
    int ast_node_count() const {
        return (int) store.size();
    }
    std::string const &get_text(int node) const {
        return at(node).token.text;
    }
    /***
     * override values no type check
     */
    int chinteger(int node, int64_t value) {
        assert(node > 0);
        store[node-1].token.integer_value = value;
        return node;
    }
    int chtype(int node, int new_type) {
        assert(node > 0);
        if(new_type >= 0) store[node-1].type = new_type;
        return node;
    }
    int negate(int node) {
        assert(node > 0);
        auto &t = store[node-1].token;
        if(t.type == FTK_INTEGER) t.integer_value = -t.integer_value;
        if(t.type == FTK_FLOAT) t.float_value = -t.float_value;
        return node;
    }
    /*** 
     * type checked accessors
     */
    int64_t get_integer(int node) const;
    double get_float(int node) const;
    double get_numberf(int node) const;
    std::string get_number(int node, int grpc_type) const;
    bool can_cast(int node, int grpc_type) const;
    std::string const get_value(int node) const;
    std::string const &get_string(int node) const;
    std::string const &get_id(int node) const;
    std::string get_joined_id(int node, int start_pos, int end_pos, std::string const &j) const;
    std::string get_dotted_id(int node, int start_pos=0, int end_pos=0) const {
        return get_joined_id(node, start_pos, end_pos, ".");
    }
    int print_ast(std::ostream &sout, int node=-1, int indent=0) const;
    /***
     * Tree to text -- render the tree at this node into text
     */
    void to_text_r(std::ostream &out, int node, int op) const;
    std::string to_text(int node) const;
    /***
     * Get the full type-name for the node
     * Will return integer, string, float, the actual enum type name or the actuall message type name
     * The node must be of a basic type or have the corresponding attribute set (enum_descriptor or message_descriptor)
     */
    std::string get_full_typename(int node) const;
    /**
     * Find the first subnode that matches condition
     */
    template<class MATCH>
    int find_first(int node, MATCH match) const {
        if(match(node)) 
            return node;
        for(auto n: at(node).children) {
            int f  = find_first(n, match);
            if(f > 0) return f;
        }
        return 0;
    }
    /**
     * Find the first subnode that has an attribute of a certain value
     */
    template<class ATTR>
    int find_first(int node, ATTR const &attribute, typename ATTR::value_t value) const {
        return find_first(node, [&attribute, value](int n) -> bool {
            return attribute.has(n) && value == attribute(n);
        });
    }
    std::set<int> find_nodes_by_type(int node, std::set<int> const &node_types) const {
        std::set<int> found;
        auto const &children = at(node).children;
        if(node_types.find(at(node).type) != node_types.end())
            found.insert(node);

        for(int n: children) {
            std::set<int> cf = find_nodes_by_type(n, node_types);
            found.insert(cf.begin(), cf.end());
        }
        return found;
    }
    int get_parent(int node) const {
        for(int n: *this) 
            if(cot::contains(at(n).children, node))
                return n;
        return 0;
    }
    int get_next_sibling(int node) const {
        int p = get_parent(node);
        if(p != 0) 
            for(unsigned i = 0, e = at(p).children.size(); i < e; ++i) 
                if(at(p).children[i] == node) {
                    if(i + 1 < e) 
                        return at(p).children[i+1];
                    else
                        return 0;
                }
        return 0;
    }
    int get_previous_sibling(int node) const {
        int p = get_parent(node);
        if(p != 0) 
            for(unsigned i = 0, e = at(p).children.size(); i < e; ++i) 
                if(at(p).children[i] == node) {
                    if(i != 0) 
                        return at(p).children[i-1];
                    else
                        return 0;
                }
        return 0;
    }

    /**
     * Lookup statement level keyword for node 'in'
     */
    int stmt_keyw(int in) const {
        static std::map<std::string, int> keywords = {{"node", FTK_NODE}, {"container", FTK_CONTAINER}, {"entry", FTK_ENTRY}, {"import", FTK_IMPORT}, {"error", FTK_ERROR}, {"option", FTK_OPTION}, {"input", FTK_INPUT}};
        auto ki = keywords.find(get_id(in));
        int k = ki == keywords.end()? FTK_DEFINE: ki->second;
        return k;
    }
    /**
     * Lookup block level keyword for node 'in'
     */
    int blck_keyw(int in, int def_type=-1) const {
        static std::map<std::string, int> keywords = {
            {"headers", FTK_HEADERS}, {"environment", FTK_ENVIRONMENT}, 
            {"mount", FTK_MOUNT}, {"init", FTK_INIT}, {"limits", FTK_LIMITS},
            {"output", FTK_OUTPUT}, {"return", FTK_RETURN} 
        };
        auto ki = keywords.find(get_id(in));
        int k = ki == keywords.end()? def_type: ki->second;
        return k;
    }
    /**
     * Check all the entries in the block for labels allowed in node and containers only
     */
    int check_node_blck(int n) {
        auto &blck = at(n);
        for(int c: blck.children) { // of type elem 
            auto &elem = at(c);
            assert(elem.type == FTK_elem);
            if(at(elem.children[1]).type == FTK_blck) {
                auto vtype = blck_keyw(elem.children[0]);
                switch(vtype) {
                    case FTK_ENVIRONMENT: case FTK_HEADERS:
                        chtype(elem.children[1], vtype);
                        break;
                    case FTK_MOUNT:
                        store[c-1].children[1] = node(FTK_MOUNT, elem.children[1]);
                        break;
                    case FTK_INIT: case FTK_LIMITS:
                        break;
                    default:
                        error(elem.children[0], "expected \"environment\", \"headers\" or \"mount\"");
                        break;
                }
            }
        }
        return n;
    }
    std::set<int> blck_value_nodes(int blck_node, std::string const &key) const {
        auto const &blck = at(blck_node);
        assert(blck.type == FTK_blck);
        std::set<int> values;
        for(int p: blck.children) {
            auto const &elem = at(p);
            assert(elem.type == FTK_elem);
            if(get_id(elem.children[0]) == key) 
                values.insert(elem.children[1]);
        }
        return values;
    }
    /**
     * Return the first value that matches 'key' from a blck compatible node
     */
    /*
    int get_first_value_node(int blck_node, std::string const &key) const {
        auto const &blck = at(blck_node);
        for(int p: blck.children) {
            auto const &elem = at(p);
            if(get_id(elem.children[0]) == key) 
                return elem.children[1];
        }
        return 0;
    }
    */
    /* Return the first value that matches 'key' from a blck compatible node
     */
    template <typename S>
    int get_nblck_value(int blck, S key) const {
        if(blck != 0) for(int p: at(blck).children) {
            auto &elem = at(p);
            if(elem.type == FTK_elem && get_id(elem.children[0]) == key) {
                return elem.children[1];
            }
        }
        return 0;
    }
    /* Return the value corresponding to the list of keys from a nested blck node
     */
    template <typename S, typename... SS>
    int get_nblck_value(int blck, S key, SS...keys) const {
        int r = get_nblck_value(blck, key);
        if(r == 0) return 0;
        return get_nblck_value(r, keys...);
    }
    void error(int node, std::string message) {
        syntax_errors.push_back(std::make_pair(node, message));
    }
    void expect(int node, std::set<int> const &types, std::string const &errm) {
        if(!cot::contains(types, at(node).type)) 
            error(node, errm);
    }
    void expect(int node, int type, std::string const &errm) {
        expect(node, std::set<int>({type}), errm);
    }
    /** 
     * Specialized tree accessors for node, entry, error, etc.
     */
    /**
     * Get the node pointing to the properties hash for entry or node node n.
     * The first subnode of type blck
     */
    int get_ne_block_node(int n) const {
        return find_first(n, [this](int x) -> bool {
            return at(x).type == FTK_blck;
        });
    }
    /**
     * Get the return or output node for 'n'. 'n' must be entry or node.
     */
    int get_ne_action(int n) const {
        assert(at(n).type == FTK_NODE || at(n).type == FTK_ENTRY);
        int bn = get_ne_block_node(n);
        return find_first(bn, [this](int x) -> bool {
            return at(x).type == FTK_RETURN || at(x).type == FTK_OUTPUT;
        });
    }
    /**
     * Get the message expression for an error node
     */
    int get_errnode_action(int n) const {
        assert(at(n).type == FTK_ERROR);
        return at(n).children.back();
    }
    /**
     * Return the fldm associated with this action node
     */
    int get_ne_action_fldm(int n) const {
        int an = get_ne_action(n);
        if(an == 0) 
            return 0;
        an = at(an).children.back();
        return at(an).type == FTK_fldm? an: 0;
    }
    /**
     * Return the fldx associated with this action node
     */
    int get_ne_action_fldx(int n) const {
        int an = get_ne_action(n);
        if(an == 0)
            return 0;
        an = at(an).children.back();
        return at(an).type == FTK_fldx? an: 0;
    }
    /**
     * Return the fldr associated with the condition of this node
     */
    int get_ne_condition(int n) const {
        auto const &node = at(n);
        MASSERT(node.type == FTK_NODE || node.type == FTK_ERROR) << " node " << n << " is of type " << node.type << ", needs to be NODE or ERROR\n";
        return node.type == FTK_NODE? (node.children.size() == 4? node.children[3]: 0): node.children[0];
    }
    /** 
     * Return the first node of type 'nt' 
     */
    int get_first_node(std::string const &nt) const {
        for(int n: *this)
            if(at(n).type == FTK_NODE && type(n) == nt) 
                return n;
        return 0;
    }
    /** 
     * Return all nodes in eval order: first nodes with condition in source order followed by the node without
     */
    std::vector<int> get_all_nodes(std::string const &nt) const {
        int noc_node = -1;
        std::vector<int> nns;
        for(int n: *this)
            if(at(n).type == FTK_NODE && type(n) == nt) {
                if(get_ne_condition(n) != 0)
                    nns.push_back(n);
                else
                    noc_node = n;
            }
        if(noc_node >= 0) nns.push_back(noc_node);
        return nns;
    }
    std::vector<int> get_all_nodes(int nn) const {
        return get_all_nodes(type(nn));
    }
    std::set<int> get_all_referenced_nodes() const {
        std::set<int> rns;
        for(int n: *this) 
            if(at(n).type == FTK_NODE && refcount(n) > 0) {
                rns.insert(n);
            }
        return rns;
    }
    /**
     * Returns the set of node references in the current subtree.
     * Only one node is returned for each node type.
     * If <nn> is a node, references are returned for all conditions of the nodes that
     * need to be evaluated before.
     */
    std::set<int> get_referenced_node_types(int nn) const {
        std::set<int> rns;
        if(nn != 0) for(int n: subtree(nn)) {
            if(at(n).type == FTK_fldx)
                rns.insert(get_first_node(get_id(at(n).children[0])));
        }
        if(nn != 0 && at(nn).type == FTK_NODE) 
            for(int pn: get_all_nodes(nn)) {
                if(pn == nn) break;
                auto ns = get_referenced_node_types(get_ne_condition(pn));
                rns.insert(ns.begin(), ns.end());
            }
        return rns;
    }
    std::set<int> get_action_referenced_node_types(int nn) const {
        return get_referenced_node_types(get_ne_action(nn));
    }
    std::set<int> get_condition_referenced_node_types(int nn) const {
        return get_referenced_node_types(get_ne_condition(nn));
    }
    struct blck_iterator {
        flow_ast const *tree;
        int blck_node;
        unsigned pos;
        std::string key;
        blck_iterator(flow_ast const *t=nullptr, int n = 0, std::string const &k=""): tree(t), blck_node(n), pos(0), key(k) {
            if(tree != nullptr && tree->at(blck_node).children.size() == 0) {
                tree = nullptr; pos = 0; blck_node = 0;
            }
        }
        int operator *() const {
            return tree == nullptr? 0: tree->at(tree->at(blck_node).children[pos]).children[1];
        }
        blck_iterator &operator ++() {
            if(tree != nullptr) {
                while(true) {
                    ++pos;
                    if(tree->at(blck_node).children.size() == pos) {
                        pos = 0; blck_node = 0; tree = nullptr;
                        break;
                    }
                    auto const &e = tree->at(tree->at(blck_node).children[pos]);
                    if(key.empty() || tree->get_id(e.children[0]) == key) 
                        break;
                }
            }
            return *this;
        }
        bool operator ==(blck_iterator const &other) {
            if(tree != other.tree) return false;
            if(tree == nullptr) return true;
            if(blck_node != other.blck_node) return false;
            return pos == other.pos;
        }
        bool operator !=(blck_iterator const &other) {
            return !(*this == other);
        }
    };
    blck_iterator blck_begin(int blck_node, std::string const &key) const {
        if(at(blck_node).type != FTK_blck) return blck_iterator();
        return blck_iterator(this, blck_node, key);
    }
    blck_iterator blck_end(int blck_node = 0) const {
        return blck_iterator();
    }
    struct blck_range {
        blck_iterator b, e;
        blck_range(flow_ast const *t=nullptr, int n = 0, std::string const &k=""): b(t,n,k) {}
        blck_iterator begin() { return b; }
        blck_iterator end() { return e; }
    };
    blck_range block(int blck_node, std::string const &key="") const {
        return blck_range(this, blck_node, key);
    }
};
std::ostream &operator << (std::ostream &out, class flow_ast::ast_iterator const &p);

#endif
