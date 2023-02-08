
%{
#ifndef FTK_ID
#include "flow-parser.h"
#endif
#define ATK_INTEGER FTK_INTEGER
#define ATK_FLOAT FTK_FLOAT
#define ATK_STRING FTK_STRING

#include "ast-scanner.H"

#ifdef YY_DECL
#undef YY_DECL
#endif
#define YY_DECL ast::token ast::scanner::scan()
#define yyterminate() return ast::token(ATK_EOF, num_lines, char_pos-line_start+1)
#define YY_USER_ACTION char_pos += yyleng;
%}

%option nodefault
%option yyclass="ast::scanner"
%option never-interactive
%option noyywrap
%option c++

DIGIT    [0-9]
ID       [A-Za-z][A-Za-z0-9_]*
OPT      [A-Za-z](-?[A-Za-z0-9])*
%x COMS STRS STRD
%%

%{
    mark_comments();
%}

[ \t\r\a\f]+     /* ignore whitespace */

{DIGIT}+  emit(FTK_INTEGER);
{DIGIT}+("."{DIGIT}+)?([Ee][+-]?{DIGIT}+)?  emit(FTK_FLOAT);
            
node emit(FTK_NODE);
container emit(FTK_CONTAINER);
error emit(FTK_ERROR);
entry emit(FTK_ENTRY);
import emit(FTK_IMPORT);

input emit(FTK_INPUT);
option emit(FTK_OPTION);

output emit(FTK_OUTPUT);
return emit(FTK_RETURN);
headers emit(FTK_HEADERS);

mount emit(FTK_MOUNT);
environment emit(FTK_ENVIRONMENT);

{ID} emit(FTK_ID);

";" emit(FTK_SEMICOLON);
"." emit(FTK_DOT);
"@" emit(FTK_AT);
"," emit(FTK_COMMA);
"==" emit(FTK_EQUALS);
":" emit(FTK_COLON);
"?" emit(FTK_QUESTION);
"||" emit(FTK_OR);
"&&" emit(FTK_AND);
"|" emit(FTK_BAR);
"^" emit(FTK_CARET);
"&" emit(FTK_AMP);
"!=" emit(FTK_NE);
"=" emit(FTK_EQ);
">" emit(FTK_GT);
"<" emit(FTK_LT);
">=" emit(FTK_GE);
"<=" emit(FTK_LE);
"<=>" emit(FTK_COMP);
">>" emit(FTK_SHR);
"<<" emit(FTK_SHL);
"+" emit(FTK_PLUS);
"-" emit(FTK_MINUS);
"*" emit(FTK_STAR);
"/" emit(FTK_SLASH);
"%" emit(FTK_PERCENT);
"!" emit(FTK_BANG);
"~" emit(FTK_TILDA);
"$" emit(FTK_DOLLAR);
"#" emit(FTK_HASH);
"(" emit(FTK_OPENPAR);
")" emit(FTK_CLOSEPAR);
"{" emit(FTK_OPENBRA);
"}" emit(FTK_CLOSEBRA);
"[" emit(FTK_OPENSQB);
"]" emit(FTK_CLOSESQB);

"//".*$ collect_start(); emit_comment();
"/*" BEGIN(COMS); collect_start();
"\"" BEGIN(STRD); collect_start();
"'"  BEGIN(STRS); collect_start();

\n  new_line();

.   emit(ATK_UNK);

<COMS>[^*\n]+        collect_text(); /* anything that's not a star or a new line */
<COMS>[*]+[^*/\n]*   collect_text(); /* stars not followed by slashes */
<COMS>\n             collect_text(); new_line();
<COMS>"*"+"/"        BEGIN(INITIAL); collect_text(); emit_comment(); 

<STRD>[^\\"\n]+      collect_text(); /* anything that's not a double quote or a new line */
<STRD>(\\.)+         collect_text(); /* double quote preceeded by backslash */
<STRD>\n             collect_text(); new_line(); /* allow strings to span new lines? */ 
<STRD>["]            BEGIN(INITIAL); collect_text(); emit_string();

<STRS>[^\\'\n]+      collect_text(); /* anything that's not a quote or a new line */
<STRS>(\\.)+         collect_text(); /* quote preceeded by backslash */
<STRS>\n             BEGIN(INITIAL); new_line(); scan_error("Unterminated string");  emit_string();
<STRS>[']            BEGIN(INITIAL); collect_text(); emit_string();

%%

#ifdef SCANNER_TEST
int main(int argc, char *argv[]) {
    ast::scanner sc(&std::cin);
    for(auto t = sc.scan(); t.type != ATK_EOF; t = sc.scan())
        std::cout << t << " ";
    return 0;
}
#endif

