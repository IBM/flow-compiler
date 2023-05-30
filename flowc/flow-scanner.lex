
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

{INT}  emit(FTK_INTEGER);
{FLT}  emit(FTK_FLOAT);
            
(?i:include)                emit(FTK_INCLUDE);
(?i:node)                   emit(FTK_NODE);
(?i:cont(ainer)?)           emit(FTK_CONTAINER);
(?i:env((iron)(ment)?)?)    emit(FTK_ENVIRONMENT);
(?i:head(er)?s?)            emit(FTK_HEADER);
(?i:import)                 emit(FTK_IMPORT);
(?i:image)                  emit(FTK_IMAGE);
(?i:output)                 emit(FTK_OUTPUT);
(?i:return)                 emit(FTK_RETURN);
(?i:entry)                  emit(FTK_ENTRY);
(?i:cpus?)                  emit(FTK_CPU);
(?i:gpus?)                  emit(FTK_GPU);
(?i:mem(ory)?)              emit(FTK_MEMORY);
(?i:repo(sitory)?)          emit(FTK_REPO);
(?i:error)                  emit(FTK_ERRCHK);
(?i:endpoint)               emit(FTK_ENDPOINT);

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
int main(int argc, char *argv[]) {
    ast::scanner sc(&std::cin);
    for(auto t = sc.scan(); t.type != ATK_EOF; t = sc.scan())
        std::cout << t << " ";
    return 0;
}
#endif

