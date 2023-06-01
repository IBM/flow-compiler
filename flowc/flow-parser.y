%token_type {int}
%name flow_parser
%token_prefix FTK_

%nonassoc ID STRING INTEGER FLOAT URL
    KEYW

    CONTAINER 
    ENTRY 
    ERRCHK 
    GROUP
    IMPORT 
    INCLUDE 
    INPUT 
    NODE 
    OUTPUT 
    RETURN 

    PROP
    
    CPU 
    ENDPOINT
    ENVIRONMENT 
    GPU 
    HEADER 
    IMAGE 
    INIT 
    LIMIT 
    MBU
    MEMORY 
    MOUNT 
    PORT 
    REPO 
    .
%left SEMICOLON.
%left COMMA.
%left EQUALS COLON.
%right QUESTION.
%left OR.
%left AND.
%left BAR.
%left CARET.
%left AMP.
%nonassoc NE EQ.
%nonassoc GT LT GE LE.
%nonassoc COMP.
%left SHR SHL.
%left PLUS MINUS.
%left STAR SLASH PERCENT.
%right POW.
%right BANG TILDA.
%right DOLLAR.
%right HASH.
%right OPENPAR CLOSEPAR OPENBRA CLOSEBRA OPENSQB CLOSESQB.
%left DOT.
%left AT. 

%fallback STRING URL.
%wildcard ANY.

%include {
#include "flow-comp.H"
#define YYNOERRORRECOVERY
}

%extra_argument { struct fc::compiler *ast }

%syntax_error {
    if(yymajor > 0 && yymajor < FTK_KEYW) {
        switch(yymajor) {
            case FTK_ID:
                ast->error(ast->at(yyminor), stru::sfmt() << "identifier not expected here");
                break;
            case FTK_INTEGER: case FTK_FLOAT:
                ast->error(ast->at(yyminor), stru::sfmt() << "numeric value not expected here");
                break;
            default:
                ast->error(ast->at(yyminor), stru::sfmt() << "string value not expected here");
                break;
        }
    } else if(yymajor > FTK_KEYW && yymajor < FTK_PROP) {
        ast->error(ast->at(yyminor), stru::sfmt() << "unexpected keyword \"" << ast->at(yyminor).token.text << "\"");
    } else if(yymajor > FTK_PROP) {
        ast->error(ast->at(yyminor), stru::sfmt() << "unexpected token \"" << ast->at(yyminor).token.text << "\"");
    } else {
        ast->error(ast->input_filename, stru::sfmt() << "syntax at " << yymajor << " of " << yyminor);
    }
}
%parse_accept {
    //std::cerr << "parsed just fine, ast size: " << ast->store.size() << " root: " << ast->store.back().children[0] << "\n";
}
%parse_failure {
    ast->node(FTK_FAILED, ast->root());
}

flow(A) ::= stmts(B).                                          { A = ast->node(FTK_ACCEPT, B); }                                   
stmts(A) ::= stmt(B).                                          { A = ast->node(FTK_flow, B); }            // FTK_flow is a list of FTK_stmt
stmts(A) ::= stmts(B) stmt(C).                                 { A = ast->nappend(B, C); }

// directives & properties defaults
stmt(A) ::= dirk(B) valx(C) SEMICOLON.                         { A = ast->nappend(B, C); } 
dirk(A) ::= INCLUDE(B).                                        { A = B; }
dirk(A) ::= IMPORT(B).                                         { A = B; }

// assignments
stmt(A) ::= assign(B).                                         { A = B; }

// container 
stmt(A) ::= CONTAINER(B) id(C) bblock(D).                      { A = ast->nappend(B, C, D); }

// node
stmt(A) ::= NODE(B) nidd(C) fcond(D) bblock(E).                { A = ast->nappend(B, C, E, D); }
stmt(A) ::= NODE(B) nidd(C) bblock(D).                         { A = ast->nappend(B, C, D); }

nidd(A) ::= id(B).                                             { A = B; }
nidd(A) ::= id(B) COLON id(C).                                 { A = ast->nappend(B, C); }

// error check
stmt(A) ::= ERRCHK(B) fcond(C) SEMICOLON.                      { A = ast->nappend(B, C); }
stmt(A) ::= ERRCHK(B) fcond(C) valx(D) SEMICOLON.              { A = ast->nappend(B, C, D); }
stmt(A) ::= ERRCHK(B) fcond(C) valx(D) COMMA valx(E) SEMICOLON. { A = ast->nappend(B, C, D, E); }

// entries
stmt(A) ::= ENTRY(B) id(C) OPENPAR rid(D) CLOSEPAR bblock(E). { A = ast->nappend(B, C, D, E); }

// node condition
fcond(A) ::= OPENPAR valx(B) CLOSEPAR.                         { A = B; }

// empty statement
stmt ::= SEMICOLON. {}

//////////////////////////////////////////////////////////
// container/node body

bblock(A) ::= OPENBRA(C) block(B) CLOSEBRA.                    { A = ast->chtype(ast->graft(C, B), FTK_block); }
bblock(A) ::= OPENBRA(C) CLOSEBRA.                             { A = ast->chtype(C, FTK_block); }
bblock(A) ::= SEMICOLON(C).                                    { A = ast->chtype(C, FTK_block); }

block(A) ::= blke(B).                                          { A = ast->node(FTK_list, B); }
block(A) ::= block(B) blke(C).                                 { A = ast->nappend(B, C); }

blke(A) ::= assign(B).                                         { A = B; }

blke(A) ::= RETURN(B) rid(C) msgexp(D) SEMICOLON.              { A = ast->nappend(B, D, C); }
blke(A) ::= OUTPUT(B) mid(C) msgexp(D) SEMICOLON.              { A = ast->nappend(B, D, C); }
blke(A) ::= RETURN(B) msgexp(D) SEMICOLON.                     { A = ast->nappend(B, D); }
blke(A) ::= OUTPUT(B) msgexp(D) SEMICOLON.                     { A = ast->nappend(B, D); }

blke(A) ::= ERRCHK(B) valx(C) SEMICOLON.                       { A = ast->nappend(B, C); }
blke(A) ::= ERRCHK(B) valx(C) COMMA valx(D) SEMICOLON.         { A = ast->nappend(B, C, D); }

blke(A) ::= ENVIRONMENT(B) assign(C).                          { A = ast->nappend(B, C); }
blke(A) ::= ENVIRONMENT(B) OPENBRA alst(C) CLOSEBRA.           { A = ast->graft(B, C); /* assign c's children to b */}
blke(A) ::= ENVIRONMENT(B) OPENBRA CLOSEBRA.                   { A = B; }
blke(A) ::= HEADER(B) vassgn(C).                               { A = ast->nappend(B, C); }
blke(A) ::= HEADER(B) OPENBRA vlst(C) CLOSEBRA.                { A = ast->graft(B, C); }
blke(A) ::= HEADER(B) OPENBRA CLOSEBRA.                        { A = B; }
// TODO
blke(A) ::= LIMIT(B) range(C).                                 { A = ast->nappend(B, C); }
blke(A) ::= LIMIT(B) OPENBRA rlst(C) CLOSEBRA.                 { A = ast->graft(B, C); }
blke(A) ::= LIMIT(B) OPENBRA CLOSEBRA.                         { A = B; }

blke(A) ::= limit(B) valx(C) SEMICOLON.                        { A = ast->nappend(B, C); }
blke ::= SEMICOLON.                                            {} 

limit(A) ::= CPU(B).                                           { A = B; }
limit(A) ::= ENDPOINT(B).                                      { A = B; }
limit(A) ::= GPU(B).                                           { A = B; }
limit(A) ::= GROUP(B).                                         { A = B; }
limit(A) ::= IMAGE(B).                                         { A = B; }
limit(A) ::= MEMORY(B).                                        { A = B; }
limit(A) ::= PORT(B).                                          { A = B; }
limit(A) ::= REPO(B).                                          { A = B; }

alst(A) ::= assign(B).                                         { A = ast->node(FTK_list, B); }
alst(A) ::= alst(B) assign(C).                                 { A = ast->nappend(B, C); }

vlst(A) ::= vassgn(B).                                         { A = ast->node(FTK_list, B); }
vlst(A) ::= vlst(B) vassgn(C).                                 { A = ast->nappend(B, C);}

rlst(A) ::= range(B).                                         { A = ast->node(FTK_list, B); }
rlst(A) ::= rlst(B) range(C).                                 { A = ast->nappend(B, C);}

//////////////////////////////////////////////////////////
// message constructor 

msgexp(A) ::= OPENPAR faslst(B) CLOSEPAR.                      { A = B; }
faslst(A) ::= fassgn(B).                                       { A = ast->node(FTK_msgexp, B); }
faslst(A) ::= faslst(B) COMMA fassgn(C).                       { A = ast->nappend(B, C); }
faslst(A) ::= faslst(B) COMMA.                                 { A = B; /* ignore spurious commas */}

fassgn(A) ::= id(B) eqorc valx(C).                             { A = ast->node(FTK_fassgn, B, C); }
fassgn(A) ::= id(B) eqorc msgexp(C).                           { A = ast->node(FTK_fassgn, B, C); }
fassgn(A) ::= id(B) eqorc rid(D) msgexp(C).                    { A = ast->node(FTK_fassgn, B, C, D); }

eqorc(A) ::= COLON(B).                                         { A = B; }
eqorc(A) ::= EQUALS(B).                                        { A = B; }

//////////////////////////////////////////////////////////
// assignments

assign(A) ::= id(C) EQUALS(B) valx(D) SEMICOLON.               { A = ast->nappend(B, C, D); }
vassgn(A) ::= valx(C) COLON(B) valx(D) SEMICOLON.              { A = ast->nappend(B, C, D); }

//////////////////////////////////////////////////////////
// literals

// any string literal
vals(A) ::= STRING(B).                                         { A = B; }
vals(A) ::= DOLLAR(B) id(C).                                   { A = ast->node(B,C); } 

// valn: any numeric literal

valn(A) ::= INTEGER(B).                                        { A = B; }
valn(A) ::= FLOAT(B).                                          { A = B; }
valn(A) ::= PLUS valn(B).                                      { A = B; } [BANG]
valn(A) ::= MINUS(B) valn(C).                                  { A = ast->node(B, C); } [BANG]

id(A) ::= ID(B).        { A = B; }

id(A) ::= CPU(B).       { A = ast->chtype(B, FTK_ID); }
id(A) ::= ENDPOINT(B).  { A = ast->chtype(B, FTK_ID); }
id(A) ::= ENVIRONMENT(B). { A = ast->chtype(B, FTK_ID); }
id(A) ::= GPU(B).       { A = ast->chtype(B, FTK_ID); }
id(A) ::= HEADER(B).    { A = ast->chtype(B, FTK_ID); }
id(A) ::= IMAGE(B).     { A = ast->chtype(B, FTK_ID); }
id(A) ::= INIT(B).      { A = ast->chtype(B, FTK_ID); }
id(A) ::= LIMIT(B).     { A = ast->chtype(B, FTK_ID); }
id(A) ::= MBU(B).       { A = ast->chtype(B, FTK_ID); }
id(A) ::= MEMORY(B).    { A = ast->chtype(B, FTK_ID); }
id(A) ::= MOUNT(B).     { A = ast->chtype(B, FTK_ID); }
id(A) ::= PORT(B).      { A = ast->chtype(B, FTK_ID); }
id(A) ::= REPO(B).      { A = ast->chtype(B, FTK_ID); }

//////////////////////////////////////////////////////////
// field, node and types references

idex(A) ::= id(B).                 { A = ast->node(FTK_did, B); }
idex(A) ::= idex(L) DOT idex(R).   { A = ast->node(FTK_did, L, R); /* TODO check that both L and R are did */}
idex(A) ::= idex(L) AT idex(R).    { A = ast->node(FTK_ndid, L, R); /* TODO check that L is singleton and R is did */ }

rid(A) ::= idex(B). { A = B; /* TODO check that no node id is present (i.e. B is did), lookup message  */}
mid(A) ::= idex(B). { A = B; /* TODO check that no node id is present (i.e. B is did), lookup method  */}

//////////////////////////////////////////////////////////
// constructs

// ranges
range(A) ::= valx(L) COLON valx(R) COLON valx(S).           { A = ast->node(FTK_range, L, R, S); }                  [BANG]
range(A) ::= valx(L) COLON valx(R).                         { A = ast->node(FTK_range, L, R); }                     [BANG]
range(A) ::= COLON valx(R).                                 { A = ast->node(FTK_range, ast->node(FTK_STAR), R); }   [BANG]
range(A) ::= valx(L) COLON.                                 { A = ast->node(FTK_range, L, ast->node(FTK_STAR)); }   [BANG]

// comma separated list
vala(A) ::= valx(B).                                        { A = ast->node(FTK_list, B); }
vala(A) ::= vala(B) COMMA valx(C).                          { A = ast->nappend(B, C); }

//////////////////////////////////////////////////////////
// expression

valx(A) ::= vals(B).                                        { A = ast->node(FTK_valx, B); }
valx(A) ::= valn(B).                                        { A = ast->node(FTK_valx, B); }
valx(A) ::= idex(B).                                        { A = ast->node(FTK_valx, B); } 
valx(A) ::= OPENPAR valx(B) CLOSEPAR.                       { A = B; } 

valx(A) ::= TILDA id(B) OPENPAR CLOSEPAR.                   { A = ast->node(FTK_fun, B); }
valx(A) ::= TILDA id(B) OPENPAR vala(C) CLOSEPAR.           { A = ast->graft(ast->node(FTK_fun, B), C, 1); }

valx(A) ::= HASH(B) valx(C).                                  { A = ast->node(FTK_valx, B, C); }          // size of repeated field
valx(A) ::= BANG(B) valx(C).                                  { A = ast->node(FTK_valx, B, C); }
valx(A) ::= valx(B) PLUS(C) valx(D).                          { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) MINUS(C) valx(D).                         { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) SLASH(C) valx(D).                         { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) STAR(C) valx(D).                          { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) PERCENT(C) valx(D).                       { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) POW(C) valx(D).                           { A = ast->node(FTK_valx, C, B, D); }  

valx(A) ::= valx(B) COMP(C) valx(D).                          { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) EQ(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) NE(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) LT(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) GT(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) LE(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) GE(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) AND(C) valx(D).                           { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) OR(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) SHL(C) valx(D).                           { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) SHR(C) valx(D).                           { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) QUESTION(C) valx(D) COLON valx(E).        { A = ast->node(FTK_valx, C, B, D, E); }  

