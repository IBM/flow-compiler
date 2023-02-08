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
    int num_lines, line_start, char_pos;
    int buffer_line, buffer_column;
    std::string buffer;
    std::vector<std::string> comments;

    int new_line() {
        ++num_lines; line_start = char_pos;
        return num_lines;
    }
public: 
    scanner(std::istream *in):yyFlexLexer(in), num_lines(0), char_pos(0) {
        new_line();
    }
    void emit_comment() {
        comments.push_back(buffer);
    }
    void mark_comments() {
        comments.push_back("");
    }
    void scan_error(std::string message) {
    }
    token scan();

    //using FlexLexer::yylex;
};

}


#define emit(t) return token(t, num_lines, char_pos-line_start+1-yyleng, std::string(yytext, yyleng));
#define emit_string() return token(FTK_STRING, buffer_line, buffer_column, buffer);
#define collect_start() buffer = std::string(yytext, yyleng); buffer_line = num_lines; buffer_column = char_pos-line_start+1-yyleng;
#define collect_text() buffer += std::string(yytext, yyleng);

static inline std::ostream &operator << (std::ostream &str, struct ast::token const &tok) {
    str << "<" << tok.type << "(" << tok.line << ":" << tok.column << ")" << tok.text << ">";
    return str;
}

#endif
