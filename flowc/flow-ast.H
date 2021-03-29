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
#include "ast-attrs.H"
#include "stru1.H"

// AST node types used
enum {
    FTK_ACCEPT=5000,
    FTK_SYNTAX_ERROR,
    FTK_flow,
    FTK_blck,
    FTK_lblk,
    FTK_elem,
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
    // Preserve comments that have a @id reference
    std::map<std::string, std::vector<std::string>> comments;              
    // Preserve comments that do not start with punctuation
    std::vector<std::pair<int, std::string>> token_comment;
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
    // node value for the condition expression associated with this node
    ast::attribute<int> condition;
    // if this node can be evaluated to a constant, one of FTK_INETEGR, FTK_STRING, FTK_FLOAT
    ast::attribute<int> value_type;
    // dimension of the data at this node
    ast::attribute<int> dimension;

    flow_ast(): message_descriptor(nullptr), method_descriptor(nullptr), input_descriptor(nullptr), field_descriptor(nullptr), enum_descriptor(nullptr), 
            name(""), type(""), description(""), condition(0), value_type(0), dimension(0)  {
            }

    bool has_attributes(int n) const {
        return has_attr(n, name, type, description, condition, message_descriptor, method_descriptor, input_descriptor, field_descriptor, enum_descriptor, value_type, dimension);
    }
    void copy_attributes(int from, int to) {
        copy_attr(from, to, name, type, description, condition, message_descriptor, method_descriptor, input_descriptor, field_descriptor, enum_descriptor, value_type, dimension);
    }
    int mk_node(flow_token const &token) {
        store.push_back(flow_ast_node());
        store.back().type = token.type;
        store.back().token = token;
        return (int) store.size();
    }
    int node(int type, int child) {
        store.push_back(flow_ast_node());
        store.back().type = type;
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
        auto &children = store[node-1].children;
        children.insert(children.begin(), child_node);
        return node;
    }
    int nappend(int node, int child_node) {
        store[node-1].children.push_back(child_node);
        return node;
    }
    flow_ast_node const &at(int node) const {
        return store[node-1];
    }
    flow_ast_node const &atc(int node, int child) const {
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
        store[node-1].token.integer_value = value;
        return node;
    }
    int chtype(int node, int new_type) {
        store[node-1].type = new_type;
        return node;
    }
    int negate(int node) {
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
    std::string get_joined_id(int node, int start_pos, std::string const &j) const;
    std::string get_dotted_id(int node, int start_pos=0) const {
        return get_joined_id(node, start_pos, ".");
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
    std::set<int> find_nodes(int node, std::set<int> const &node_types) const {
        std::set<int> found;
        auto const &children = at(node).children;
        if(node_types.find(at(node).type) != node_types.end())
            found.insert(node);

        for(int n: children) {
            std::set<int> cf = find_nodes(n, node_types);
            found.insert(cf.begin(), cf.end());
        }
        return found;
    }
    template<class MATCH>
    std::set<int> find_nodes(int node, MATCH match) const {
        std::set<int> found;
        if(match(node)) found.insert(node);
        for(int c: at(node).children) {
            std::set<int> cf = find_nodes(c, match);
            found.insert(cf.begin(), cf.end());
        }
        return found;
    }
    int stmt_keyw(int in) const {
        static std::map<std::string, int> keywords = {{"node", FTK_NODE}, {"container", FTK_CONTAINER}, {"entry", FTK_ENTRY}, {"import", FTK_IMPORT}};
        auto ki = keywords.find(get_id(in));
        int k = ki == keywords.end()? FTK_DEFINE: ki->second;
        return k;
    }
    std::vector<std::pair<int, std::string>> syntax_errors;
    void expect(int node, std::set<int> const &types, std::string const &errm) {
        if(!stru1::contains(types, at(node).type)) 
            syntax_errors.push_back(std::make_pair(node, errm));
    }
    void expect(int node, int type, std::string const &errm) {
        expect(node, std::set<int>({type}), errm);
    }
};

#endif
