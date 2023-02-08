#ifndef H_FLOW_COMPILER_H
#define H_FLOW_COMPILER_H

#include <string>
#include <vector>

#include "fcast.H"
#include "gru.H"

namespace fc {

class flow_compiler: public proto_compiler {
protected:
    struct ast ast;
    std::string flow_file;

    void AddNote(int line, int column, std::string const &message) {
        pcerr.AddNote(flow_file, line, column, message);
    }
    void AddError(int line, int column, std::string const &message) {
        pcerr.AddError(flow_file, line, column, message);
    }
    void AddWarning(int line, int column, std::string const &message) {
        pcerr.AddWarning(flow_file, line, column, message);
    }
    void AddError(::ast::token const &token, std::string const &message) { 
        AddError(token.line, token.column, message);
    }
    void AddError(::ast::node const &pos, std::string const &message) { 
        AddError(pos.token, message);
    }
    void AddWarning(::ast::token const &token, std::string const &message) { 
        AddWarning(token.line, token.column, message);
    }
    void AddWarning(::ast::node const &pos, std::string const &message) { 
        AddWarning(pos.token, message);
    }
    void AddNote(::ast::token const &token, std::string const &message) { 
        AddNote(token.line, token.column, message);
    }
    void AddNote(::ast::node const &pos, std::string const &message) { 
        AddNote(pos.token, message);
    }

    void add_comments(int token, std::vector<std::string> const &comments);
public:
    flow_compiler();
    int compile_flow(std::string file, bool trace_on = false);
};

}

#endif
