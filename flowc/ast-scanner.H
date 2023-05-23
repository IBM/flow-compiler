#ifndef H_AST_SCANNER_H
#define H_AST_SCANNER_H

#include <cstdlib>
#include <iosfwd>
#include <string>

#include "ast.H"

#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

#define ATK_EOF 6000
#define ATK_UNK 7000

namespace ast {

class scanner: public yyFlexLexer {
    int file_id, num_lines, line_start, char_pos;
    int buffer_line, buffer_column;
    std::string buffer;
    std::vector<std::string> comments;
    std::vector<atxt> errors;

    int new_line() {
        ++num_lines; line_start = char_pos;
        return num_lines;
    }
public: 
    scanner(std::istream *in, int f_id):yyFlexLexer(in), file_id(f_id), num_lines(0), char_pos(0) {
        new_line();
    }
    void emit_comment() {
        comments.push_back(buffer);
    }
    void mark_comments() {
        comments.push_back("");
    }
    void scan_error(int l, int c, std::string message) {
        errors.push_back(token(ATK_NONE, file_id, l, c, message));
    }
    token scan();
    bool has_error() const {
        return errors.size() > 0;
    }
    atxt get_error() {
        if(!has_error())
            return atxt();
        atxt e = errors[0];
        errors.erase(errors.begin());
        return e;
    }

    //using FlexLexer::yylex;
};

}


#define emit(tok) return token(tok, file_id, num_lines, char_pos-line_start+1-yyleng, std::string(yytext, yyleng));
#define emit_string(tok) return token(tok, file_id, buffer_line, buffer_column, buffer);
#define collect_start() buffer = std::string(yytext, yyleng); buffer_line = num_lines; buffer_column = char_pos-line_start+1-yyleng;
#define collect_text() buffer += std::string(yytext, yyleng);

static inline std::ostream &operator << (std::ostream &str, struct ast::token const &tok) {
    str << "<" << tok.type << "(" << tok.line << ":" << tok.column << ")" << tok.text << ">";
    return str;
}

#endif
