#ifndef H_FLOW_COMP_H
#define H_FLOW_COMP_H

#include <string>
#include <vector>
#include <set>
#include "ast.H"
#include "stru.H"

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
class compiler: public ast::tree {
public:
    ast::attribute<int> vtype;
    ast::attribute<int> ref;
protected:
    std::ostream *errs;
public:
    std::string input_filename;
    std::vector<std::string> filenames;
    /** The number of errors and warnings currently accumulated
     */
    int warning_count, error_count;
    /** Options for displaying the line with the current error or warning.
     * They can (could) be set from the command line. 
     */
    bool show_line_with_error, show_line_with_warning;
    /** Compiler messages, and helper shortcuts
     */
    enum message_type { mt_error, mt_warning, mt_note };
    void message(std::string filename, int line, int column, message_type type, bool prompt, std::string const &msg) const;

    void error(std::string filename, int line, int column, std::string const &msg) {
        message(filename, line, column, mt_error, show_line_with_error, msg);
        ++error_count;
    }
    void warning(std::string filename, int line, int column, std::string const &msg) {
        message(filename, line, column, mt_warning, show_line_with_warning, msg);
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
        file_pos(compiler const *fc, std::string s): filename(s), line(-1), column(-1) {
        }
        file_pos(compiler const *fc, ast::atxt p): filename(fc->filenames[p.file]), line(p.line), column(p.column) {
        }
        file_pos(compiler const *fc, ast::node t): filename(fc->filenames[t.token.file]), line(t.token.line), column(t.token.column){
        }
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
    int compile(std::string filename, bool debug_on, bool trace_on);
    void print_ast(std::ostream &out, int node) const;

    /** Internal support
     */
    int parse_file(struct yyParser *pp, std::string filename, bool trace_on);

    /** Initializers
     */
    void reset();
    compiler(std::ostream *error_stream):
        vtype(0), ref(0), 
        errs(error_stream) {
        reset();
    }

    /** Static info
     */
    static std::string get_version();
    static std::string get_build_id();
    static std::string get_default_runtime();
    static std::set<std::string> available_runtimes();

    /** Coversions
     */
    static int itype(std::string type_name);
    static std::string stype(int type);
    static std::string stoken(int ftk);
};

}

#endif
