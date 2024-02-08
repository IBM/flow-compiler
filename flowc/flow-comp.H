#ifndef H_FLOW_COMP_H
#define H_FLOW_COMP_H

#include <string>
#include <vector>
#include <set>
#include <initializer_list>
#include <iosfwd>
#include "ast.H"
#include "stru.H"
#include "grpcu.H"

// AST node types -- 
// Some of these overlap with non-terminals but there is no way currently 
// to align the numerical values, since only terminals are declared in
// the header file produced by lemon.
enum ast_nonterm {
    FTK_ACCEPT=200,
    FTK_FAILED,

    FTK_flow,

    FTK_block, 
    FTK_list,
    FTK_fassgn,
    FTK_msgexp,

    FTK_fun, 
    FTK_did, 
    FTK_ndid,
    FTK_valx,
    FTK_range,

    FTK_MAX_NONTERM
};

#ifndef WFTK_ID
#include "flow-parser.h"
#endif

namespace fc {

enum fvt_type {
    fvt_none = 0,
    fvt_int, fvt_flt, fvt_str,  
    fvt_enum, 
    fvt_array, 
    fvt_struct,
};

struct value_type {
    fvt_type type;
    std::vector<value_type> inf;
    std::string gname;
    std::string fname;

    value_type(fvt_type t=fvt_none, std::string gn=""):type(t), gname(gn) {
    }
    value_type(fvt_type t, std::initializer_list<value_type> tl, std::string gn=""):
        type(t), inf(tl), gname(gn) { 
    }
    void add_type(value_type t) {
        inf.push_back(t);
    }
    std::string to_string() const;
    std::string struct_name() const { return gname; }
    std::string enum_name() const { return gname; }
    std::string field_name() const { return fname; }
    bool is_basic() const { return type == fvt_int || type == fvt_flt || type == fvt_str; }
    bool is_null() const { return type == fvt_none; }
    bool is_str() const { return type == fvt_str; }
    bool is_num() const { return type == fvt_int || type == fvt_flt; }
    bool is_int() const { return type == fvt_int; }
    bool is_flt() const { return type == fvt_int; }
    bool is_array() const { return type == fvt_array; }
    bool is_struct() const { return type == fvt_struct; }
    value_type a_type() const { return type == fvt_array? inf[0]: value_type(); }

    inline bool operator ==(value_type const &other) const {
        return type == other.type;
    }
    inline bool operator !=(value_type const &other) const {
        return !(*this == other);
    }
};

class compiler: public ast::tree {
public:
    ast::attribute<value_type> vtype;   // type expression for NODE, ENTRY, INPUT and valx
    ast::attribute2<int, 0> ref;        // reference to another valx
    ast::attribute<std::string> rpc;    // grpc method for NODE, ENTRY
    ast::attribute2<int, 0> const_level;// 3 when valx is a constant expression, 2 when constant at global level, 1 when constant at call level
    ast::attribute<std::string> cmsg;   // msg type for msgexp nodes
    ast::attribute<std::string> unid;   // individual node identifier
    grpcu::store gstore;
protected:
    std::ostream *errs;
public:
    std::string input_filename;
    std::vector<std::string> filenames;
    /** The number of errors and warnings currently accumulated
     */
    int warning_count, error_count;
    /** Compiler messages, and helper shortcuts
     */
    enum message_type { mt_error, mt_warning, mt_note };
    void message(std::string filename, int line, int column, message_type type, bool prompt, std::string const &msg) const;

    void error(std::string filename, int line, int column, std::string const &msg) {
        message(filename, line, column, mt_error, true, msg);
        ++error_count;
    }
    void warning(std::string filename, int line, int column, std::string const &msg) {
        message(filename, line, column, mt_warning, true, msg);
        ++warning_count;
    }
    void note(std::string filename, int line, int column, std::string const &msg) const {
        message(filename, line, column, mt_note, false, msg);
    }
    void notep(std::string filename, int line, int column, std::string const &msg) const {
        message(filename, line, column, mt_note, true, msg);
    }
    struct file_pos {
        std::string filename;
        int line, column;
        file_pos(compiler const *fc, std::string s): filename(s), line(-1), column(-1) { }
        file_pos(compiler const *fc, ast::atxt p): filename(fc->filenames[p.file]), line(p.line), column(p.column) { }
        file_pos(compiler const *fc, ast::node t): filename(fc->filenames[t.token.file]), line(t.token.line), column(t.token.column){ }
    };
    template <class L>
    void error(L fpos, std::string const &msg) {
        file_pos f(this, fpos);
        error(f.filename, f.line, f.column, msg);
    }
    template <class L>
    void warning(L fpos, std::string const &msg) {
        file_pos f(this, fpos);
        warning(f.filename, f.line, f.column, msg);
    }
    template <class L>
    void note(L fpos, std::string const &msg) const {
        file_pos f(this, fpos);
        note(f.filename, f.line, f.column, msg);
    }
    template <class L>
    void notep(L fpos, std::string const &msg) const {
        file_pos f(this, fpos);
        notep(f.filename, f.line, f.column, msg);
    }

    /** Actions
     */
    int compile(std::string filename, bool debug_on, bool trace_on, std::string default_input_symbol);
    void print_ast(std::ostream &out, int node) const;
    void print_ast(int node) const;
    void ast_to_json(std::ostream &out, int node) const;
    int eval(int node, std::string &value, value_type *vtp=nullptr);
    int walk(int node);

    /** Internal support
     */
private:
    int parse_file(struct yyParser *pp, std::string filename, bool trace_on);
    int resolve_references(bool debug_on);
    int resolve_node_types(bool debug_on);
    int resolve_field_types(bool debug_on);
    int check_node_references(bool debug_on);
    int check_node_types(bool debug_on);
    int fixup_symbol_references(bool debug_on);
    int fixup_nodes(bool debug_on);
    int compute_value_type(bool debug_on, int node);
    int propagate_value_types(bool debug_on);

    int cpp_generator(std::ostream &buf) const;
    int cpp_initiate_nf(struct cpp_data &, std::string family) const;
    int cpp_rexpr(struct cpp_data &, int node, std::string name) const;

    int generate(std::ostream &buf, int entry_node, int input_node, bool debug_on);
    /*
    int generate_valx(stru::indented_stream &indenter, int vn, bool debug_on);
    int generate_valx_ndid(stru::indented_stream &indenter, int nn, bool debug_on);
    int generate_valx_msgexp(stru::indented_stream &indenter, int mn, bool debug_on);
    int dep_tree(stru::indented_stream &indenter, int vn, std::string input_label, std::map<std::string, int> &visited, int depth=0);
*/
public:
    /** Initializers
     */
    void reset();
    compiler(std::ostream *error_stream):
        vtype(value_type(fvt_none)), 
        errs(error_stream) {
        reset();
    }

    /** Static info
     */
    static std::string get_version();
    static std::string get_build_id();
    static std::string get_default_runtime();
    static std::set<std::string> available_runtimes();

    /** Conversions
     */
    virtual std::string tk_to_string(int ftk) const;
    virtual int string_to_tk(std::string ftk) const;

    /** Accesors
     */
    /** Get the ids from a did ndid or ID node
     */
    std::vector<std::string> get_ids(int node) const;
    /** Get the first node in tree order that has a return type
     */
    int main_node_by_type(std::string node_type) const;
    /** Retrive the entire node family in code order
     */
    std::vector<int> node_family(std::string family_name) const;
    /** Get all referenced families in the expression node
     */
    std::set<std::string> get_referenced_families(int valx_node) const;

    /** Get all the node referenced by the subtree at <node_node>
     */
    int get_node_references(std::map<std::string, std::set<int>> &refs, int node_node) const;
    /** Get all node families refereced in this subtree
     */
    int get_family_references(std::set<std::string> &refs, int n) const;
    std::set<std::string> get_family_references(int n) const {
        std::set<std::string> refs;
        get_family_references(refs, n);
        return refs;
    }
    /** Get all the node referenced by the subtrees of all nodes in the same <node_family>
     */
    int get_node_references(std::map<std::string, std::set<int>> &refs, std::string node_family) const;
    /** Recursively get all the nodes referenced by <node_family>
     */
    int get_deep_node_references(std::map<std::string, std::set<int>> &refs, std::string node_family) const;
    /** Get all the node families referenced by the subtree at n
     */
    int get_deep_family_references(std::set<std::string> &refs, int n, std::string input_label) const;
    std::set<std::string> get_deep_family_references(int n, std::string input_label) const {
        std::set<std::string> refs;
        get_deep_family_references(refs, n, input_label);
        return refs;
    }
    /** Annotators
     */
    /** Set the const attribute *const_level*
     */
    int set_const_level(int node=0);
};
}
std::ostream &operator << (std::ostream &s, fc::value_type const &vt);

#endif
