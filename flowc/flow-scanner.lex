
%{
#ifndef FTK_ID
#include "flow-parser.h"
#endif
#define ATK_INTEGER WFTK_INTEGER
#define ATK_FLOAT WFTK_FLOAT
#define ATK_STRING WFTK_STRING

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
ALPHA    [A-Za-z]
ALNUM    {ALPHA}|{DIGIT}
ID       {ALPHA}({ALNUM}|_)*
LABEL    {ALPHA}(-?{ALNUM})*
CNAME    {LABEL}(\.{LABEL})*
HEXD     [0-9A-Fa-f]
URLESC   [%]{HEXD}{HEXD}
UNRES    {ALNUM}|[._~-]
GDELI    [:/?#\[\]@]
SDELI    [!$&'()*+,;=]
RESVD    {GDELI}|{SDELI}
PCHAR    {UNRES}|{HEXD}|{SDELI}|[:@]
QUEFRAG  ({PCHAR}|[/?])+

XALPHA   {ALNUM}|{URLESC}|[!*"'(),$_@.&+-]
URL      (?i:(https?)|s3|file):\/\/({CNAME}(:{DIGIT}+)?)?(\/+{XALPHA})*
INTH     ({DIGIT}{HEXD}*[hH])|(0[xX]{HEXD}+)
INTB     [01]+[bB]
INTO     0[0-7]+
INT      {DIGIT}+
INTX     {INT}|{INTB}|{INTH}|{INTO}
FLT      {INT}("."{INT})?([Ee][+-]?{INT})? 
NUM      {INT}|{FLT}
MBU      [kKMGT]i?[Bb]?

%x COMS STRS STRD
%%

%{
    mark_comments();
%}

[ \t\r\a\f]+     /* ignore whitespace */

{INT} emit(FTK_INTEGER);
{FLT} emit(FTK_FLOAT);
            
include                emit(FTK_INCLUDE);
node                   emit(FTK_NODE);
import                 emit(FTK_IMPORT);
image                  emit(FTK_IMAGE);
output                 emit(FTK_OUTPUT);
return                 emit(FTK_RETURN);
entry                  emit(FTK_ENTRY);
error                  emit(FTK_ERRCHK);
container              emit(FTK_CONTAINER);
group                  emit(FTK_GROUP);

env((iron)(ment)?)?    emit(FTK_ENVIRONMENT);
headers?               emit(FTK_HEADER);
(?i:cpus?)             emit(FTK_CPU);
(?i:gpus?)             emit(FTK_GPU);
mem(ory)?              emit(FTK_MEMORY);
repo(sitory)?          emit(FTK_REPO);
endpoint               emit(FTK_ENDPOINT);
port                   emit(FTK_PORT);
init                   emit(FTK_INIT);
limits?                emit(FTK_LIMIT);
mounts?                emit(FTK_MOUNT);
timeout                emit(FTK_TIMEOUT);
scale                  emit(FTK_SCALE);

{MBU} emit(FTK_MBU);
{URL} emit(FTK_URL);
{ID} emit(FTK_ID);

";" emit(FTK_SEMICOLON);
"." emit(FTK_DOT);
"@" emit(FTK_AT);
"," emit(FTK_COMMA);
"==" emit(FTK_EQ);
":" emit(FTK_COLON);
"?" emit(FTK_QUESTION);
"||" emit(FTK_OR);
"&&" emit(FTK_AND);
"|" emit(FTK_BAR);
"^" emit(FTK_CARET);
"**" emit(FTK_POW);
"&" emit(FTK_AMP);
"!=" emit(FTK_NE);
"=" emit(FTK_EQUALS);
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

<STRD>[^\\"\n]+      collect_text(); /* anything that's not a double quote, backslash or new line */
<STRD>(\\.)+         collect_text(); /* anything preceeded by a backslash */
<STRD>(\\\n)         collect_text(); new_line();
<STRD>\n             collect_text(); new_line(); /* allow strings to span new lines? */ 
<STRD>["]            BEGIN(INITIAL); collect_text(); emit_string(FTK_STRING);

<STRS>[^'\n]+        collect_text(); /* anything that's not a quote, backslash or new line */
<STRS>\n             BEGIN(INITIAL); new_line(); scan_error(buffer_line, buffer_column, "unterminated string"); emit_string(FTK_STRING);
<STRS>[']            BEGIN(INITIAL); collect_text(); emit_string(FTK_STRING);

%%

#ifdef SCANNER_TEST
inline static 
std::ostream &print_token(std::ostream &out, struct ast::token const &tok) {
    if(tok.type == ATK_NONE) 
        out << "<NONE>";
    else if(tok.line > 0)
        out << "<" << tok.type << "(" << tok.file << "-" << tok.line << ":" << tok.column << "):" << tok.text << ">";
    else 
        out << "<" << tok.type << ":" << tok.text << ">";
    return out;
}
int main(int argc, char *argv[]) {
    ast::scanner sc(&std::cin);
    for(auto t = sc.scan(); t.type != ATK_EOF; t = sc.scan())
        std::cout << t << " ";
    return 0;
}
#endif

