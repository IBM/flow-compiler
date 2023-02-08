#ifndef H_FCAST_H
#define H_FCAST_H

#include <iosfwd>
#include <map>
#include <utility>
#include <vector>

#ifndef FTK_ID
#include "flow-parser.h"
#endif

#define ATK_STRING FTK_STRING
#define ATK_INTEGER FTK_INTEGER
#define ATK_FLOAT FTK_FLOAT

#include "ast.H"
#include "ast-attribute.H"

// AST node types 
enum {
    FTK_ACCEPT=1000,
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

namespace fc {

inline static constexpr 
bool is_value_type(int type) {
    return type == FTK_INTEGER || type == FTK_STRING || type == FTK_FLOAT;
}
char const *node_name(int i);

struct ast: public ::ast::tree {
    attribute<std::string> name;
    attribute<std::string> type;
    attribute<std::string> description;
    attribute<int> refcount;
    attribute<void *> ptr;
    /** Errors accumulated while parsing
     */
    std::vector<std::pair<int, std::string>> syntax_errors;

    ast():name(""), type(""), description(""), refcount(0), ptr(nullptr) { }

    void error(int node, std::string message);
    std::string const &get_id(int node) const;
    /** Lookup statement level keyword for node 'in'
     */
    int stmt_kwd(int in) const;
    /** Lookup block level keyword for node 'in'
     */
    int blck_kwd(int in, int def_type=-1) const;
    /** Generate an error if <node> is not of the expected <type>
     */
    void expect(int node, std::set<int> const &types, std::string const &errm);
    void expect(int node, int type, std::string const &errm);

    /** Find the first node of the expected <type> in the subtree at node,
     *  with left most child matching <id>. If node is 0, the entire tree is searched.
     *  Returns the first matching node or 0 if not found.
     */
    int find(int node, int type, std::string const &id) const;
};

}

std::ostream &operator << (std::ostream &out, ast::node const &node);

#endif
