%token_type {int}
%name flow_parser
%token_prefix FTK_

%nonassoc ID STRING INTEGER FLOAT URL
    NODE CONTAINER ENTRY IMPORT INPUT OUTPUT RETURN ERRCHK INCLUDE 
    MOUNT ENVIRONMENT HEADER INIT LIMITS REPO
    CPU GPU MEMORY MBU
    .
%left SEMICOLON.
%left DOT.
%left AT.
%left COMMA.
%nonassoc EQUALS COLON.
%right QUESTION.
%left OR.
%left AND.
%left BAR.
%left CARET.
%left AMP.
%left NE EQ.
%left GT LT GE LE.
%left COMP.
%left SHR SHL.
%left PLUS MINUS.
%left STAR SLASH PERCENT.
%left POW.
%right BANG TILDA.
%right DOLLAR.
%right HASH.
%right OPENPAR CLOSEPAR OPENBRA CLOSEBRA OPENSQB CLOSESQB.

%fallback ID CPU GPU MEMORY MBU MOUNT ENVIRONMENT HEADER INIT LIMITS REPO ENDPOINT.
%fallback STRING URL.
%wildcard ANY.

%include {
#include "flow-comp.H"
#define YYNOERRORRECOVERY
}

%extra_argument { struct fc::compiler *ast }

%syntax_error {
    ast->error(ast->input_filename, "syntax");
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
stmt(A) ::= assign(B).                                          { A = B; }

// container 
stmt(A) ::= CONTAINER(B) ID(C) OPENBRA block(D) CLOSEBRA.      { A = ast->nappend(B, C, D); }

// node
stmt(A) ::= NODE(B) nidd(C) fcond(D) OPENBRA block(E) CLOSEBRA. { A = ast->nappend(B, C, E, D); }
stmt(A) ::= NODE(B) nidd(C) OPENBRA block(D) CLOSEBRA.         { A = ast->nappend(B, C, D); }

nidd(A) ::= ID(B).                                             { A = B; }
nidd(A) ::= ID(B) COLON ID(C).                                 { A = ast->nappend(B, C); }

// error check
stmt(A) ::= ERRCHK(B) fcond(C) SEMICOLON.                      { A = ast->nappend(B, C); }
stmt(A) ::= ERRCHK(B) fcond(C) valx(D) SEMICOLON.              { A = ast->nappend(B, C, D); }
stmt(A) ::= ERRCHK(B) fcond(C) valx(D) COMMA valx(E) SEMICOLON. { A = ast->nappend(B, C, D, E); }

// entries
stmt(A) ::= ENTRY(B) ID(C) OPENPAR ID(D) CLOSEPAR OPENBRA block(E) CLOSEBRA. { A = ast->nappend(B, C, D, E); }

// node condition
fcond(A) ::= OPENPAR valx(B) CLOSEPAR.                         { A = B; }

// empty statement
stmt ::= SEMICOLON. {}

//////////////////////////////////////////////////////////
// container/node body

block(A) ::= blke(B).                                          { A = ast->node(FTK_block, B); }
block(A) ::= block(B) blke(C).                                 { A = ast->nappend(B, C); }

blke(A) ::= assign(B).                                         { A = B; }

blke(A) ::= RETURN(B) did(C) msgexp(D) SEMICOLON.              { A = ast->nappend(B, D, C); }
blke(A) ::= OUTPUT(B) did(C) msgexp(D) SEMICOLON.              { A = ast->nappend(B, D, C); }
blke(A) ::= RETURN(B) msgexp(D) SEMICOLON.                     { A = ast->nappend(B, D); }
blke(A) ::= OUTPUT(B) msgexp(D) SEMICOLON.                     { A = ast->nappend(B, D); }

blke(A) ::= ERRCHK(B) valx(C) SEMICOLON.                       { A = ast->nappend(B, C); }
blke(A) ::= ERRCHK(B) valx(C) COMMA valx(D) SEMICOLON.         { A = ast->nappend(B, C, D); }


blke(A) ::= ENVIRONMENT(B) assign(C).                          { A = ast->nappend(B, C); }
blke(A) ::= ENVIRONMENT(B) OPENBRA alst(C) CLOSEBRA.           { A = ast->graft(B, C); /* assign c's children to b */}
blke(A) ::= HEADER(B) vassgn(C).                               { A = ast->nappend(B, C); }
blke(A) ::= HEADER(B) OPENBRA vlst(C) CLOSEBRA.                { A = ast->graft(B, C); /* assign c's children to b */}

blke(A) ::= limit(B) valx(C) SEMICOLON.                        { A = ast->nappend(B, C); }
blke ::= SEMICOLON.                                            {} 

limit(A) ::= MEMORY(B).                                        { A = B; }
limit(A) ::= CPU(B).                                           { A = B; }
limit(A) ::= GPU(B).                                           { A = B; }
limit(A) ::= REPO(B).                                          { A = B; }
limit(A) ::= IMAGE(B).                                         { A = B; }
limit(A) ::= ENDPOINT(B).                                      { A = B; } 


alst(A) ::= assign(B).                                         { A = ast->node(FTK_list, B); }
alst(A) ::= alst(B) assign(C).                                 { A = ast->nappend(B, C); }

vlst(A) ::= vassgn(B).                                         { A = ast->node(FTK_list, B); }
vlst(A) ::= vlst(B) vassgn(C).                                 { A = ast->nappend(B, C);}


//////////////////////////////////////////////////////////
// message constructor 

msgexp(A) ::= OPENPAR faslst(B) CLOSEPAR.                      { A = B; }
faslst(A) ::= fassgn(B).                                       { A = ast->node(FTK_msgexp, B); }
faslst(A) ::= faslst(B) COMMA fassgn(C).                       { A = ast->nappend(B, C); }
faslst(A) ::= faslst(B) COMMA.                                 { A = B; }

fassgn(A) ::= ID(B) COLON valx(C).                             { A = ast->node(FTK_fassgn, B, C); }
fassgn(A) ::= ID(B) EQUALS valx(C).                            { A = ast->node(FTK_fassgn, B, C); }

//////////////////////////////////////////////////////////
// assignments

assign(A) ::= ID(C) EQUALS(B) valx(D) SEMICOLON.               { A = ast->nappend(B, C, D); }
vassgn(A) ::= valx(C) COLON(B) valx(D) SEMICOLON.              { A = ast->nappend(B, C, D); }

//////////////////////////////////////////////////////////
// literals

// any string literal
vals(A) ::= STRING(B).                                         { A = B; }
vals(A) ::= DOLLAR(B) ID(C).                                   { A = ast->node(B,C); } 

// valn: any numeric literal

valn(A) ::= INTEGER(B).                                        { A = B; }
valn(A) ::= FLOAT(B).                                          { A = B; }
valn(A) ::= PLUS valn(B).                                      { A = B; } [BANG]
valn(A) ::= MINUS(B) valn(C).                                  { A = ast->node(B, C); } [BANG]

//////////////////////////////////////////////////////////
// field, node and types references

// ndid: node @ [dtid]
ndid(A) ::= ID(B) AT.                                          { A = ast->node(FTK_ndid, B); } 
ndid(A) ::= ID(B) AT did(C).                                   { A = ast->node(FTK_ndid, B, C); } 
// dtid: dotted id id[.id]*
did(A) ::= ID(B).                                              { A = ast->node(FTK_did, B); }
did(A) ::= did(B) DOT ID(C).                                   { A = ast->nappend(B, C); }

//////////////////////////////////////////////////////////
// expression

valx(A) ::= vals(B).                                           { A = ast->node(FTK_valx, B); }
valx(A) ::= valn(B).                                           { A = ast->node(FTK_valx, B); }
valx(A) ::= did(B).                                            { A = ast->node(FTK_valx, B); }
valx(A) ::= ndid(B).                                           { A = ast->node(FTK_valx, B); }
valx(A) ::= OPENPAR valx(B) CLOSEPAR.                          { A = B; } 

valx(A) ::= ID(B) OPENPAR CLOSEPAR.                            { A = ast->node(FTK_fun, B); }
valx(A) ::= ID(B) OPENPAR vala(C) CLOSEPAR.                    { A = ast->graft(ast->node(FTK_fun, B), C, 1); }
vala(A) ::= valx(B).                                           { A = ast->node(FTK_list, B); }
vala(A) ::= vala(B) COMMA valx(C).                             { A = ast->nappend(B, C); }

valx(A) ::= HASH(B) valx(C).                                   { A = ast->node(FTK_valx, B, C); }          // size of repeated field
valx(A) ::= BANG(B) valx(C).                                   { A = ast->node(FTK_valx, B, C); }
valx(A) ::= valx(B) PLUS(C) valx(D).                           { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) MINUS(C) valx(D).                          { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) SLASH(C) valx(D).                          { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) STAR(C) valx(D).                           { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) PERCENT(C) valx(D).                        { A = ast->node(FTK_valx, C, B, D); }  
valx(A) ::= valx(B) POW(C) valx(D).                            { A = ast->node(FTK_valx, C, B, D); }  

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

