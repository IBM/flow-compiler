%token_type {int}
%name flow_parser
%token_prefix FTK_

%nonassoc ID STRING INTEGER FLOAT SYMBOL.
%left SEMICOLON.
%left DOT AT.
%left COMMA.
%nonassoc EQUALS COLON.
%left OR.
%left AND.
%left BAR.
%left CARET.
%left AMP.
%left NE EQ.
%left GT LT GE LE.
%right BANG. 
%right HASH.
%right OPENPAR CLOSEPAR OPENBRA CLOSEBRA OPENSQB CLOSESQB.

%include {
#include <iostream>
#include "flow-ast.H"
#define YYNOERRORRECOVERY
}

%extra_argument { flow_ast *ast }

%syntax_error {
    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
}
%parse_accept {
    //std::cerr << "parsed just fine, ast size: " << ast->store.size() << " root: " << ast->store.back().children[0] << "\n";
}
%parse_failure {
    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
}

main(A) ::= flow(B).                                           { A = ast->node(FTK_ACCEPT, B); }

flow(A) ::= stmt(B).                                           { A = ast->node(FTK_flow, B); }              // FTK_flow is a list of FTK_stmt
flow(A) ::= flow(B) stmt(C).                                   { A = ast->nappend(B, C); }

stmt(A) ::= ID(B) vali(C) SEMICOLON.                           { A = ast->node(FTK_stmt, B, C); }               
stmt(A) ::= ID(B) dtid(C) OPENPAR bexp(E) CLOSEPAR blck(D).    { A = ast->node(FTK_stmt, B, C, D, E); }
stmt(A) ::= ID(B) dtid(C) blck(D).                             { A = ast->node(FTK_stmt, B, C, D); }
stmt(A) ::= stmt(B) SEMICOLON.                                 { A = B; }                                   // Skip over extraneous semicolons

blck(A) ::= OPENBRA list(B) CLOSEBRA.                          { A = B; }                                   // Blocks must be enclosed in { }
blck(A) ::= OPENBRA(B) CLOSEBRA.                               { A = ast->chtype(B, FTK_blck); }                // Empty blocks are allowed

list(A) ::= elem(B).                                           { A = ast->node(FTK_blck, B); }              // FTK_blck is a list of declarations (elem)
list(A) ::= list(B) elem(C).                                   { A = ast->nappend(B, C); }

elem(A) ::= ID(B) blck(C).                                     { A = ast->node(FTK_elem, B, C); }           // Name/Map definition        
elem(A) ::= ID(B) lblk(C).                                     { A = ast->node(FTK_elem, B, C); }           // Name/Labeled Map definition        
elem(A) ::= ID(B) valx(C) SEMICOLON.                           { A = ast->node(FTK_elem, B, C); }           // Name/Value definition           
elem(A) ::= ID(B) eqc valx(C) SEMICOLON.                       { A = ast->node(FTK_elem, B, C); }           // Name/Value definition           
elem(A) ::= ID(B) oexp(C) SEMICOLON.                           { A = ast->node(FTK_elem, B, C); }           // Output definition (ID must be 'output')
elem(A) ::= ID(B) rexp(C) SEMICOLON.                           { A = ast->node(FTK_elem, B, C); }           // Output definition (ID must be 'return')
elem(A) ::= elem(B) SEMICOLON.                                 { A = B; }                                   // Skip over extraneous semicolons

lblk(A) ::= ID(B) blck(C).                                     { A = ast->node(FTK_lblk, B, C); }

valx(A) ::= STRING(B).                                         { A = B; }
valx(A) ::= INTEGER(B).                                        { A = B; }
valx(A) ::= FLOAT(B).                                          { A = B; }

vali(A) ::= valx(B).                                           { A = B; }
vali(A) ::= dtid(B).                                           { A = B; }

rexp(A) ::= OPENPAR fldm(B) CLOSEPAR.                          { A = ast->node(FTK_rexp, B); }              
rexp(A) ::= OPENPAR ID(B) AT CLOSEPAR.                         { A = ast->node(FTK_rexp, B); }              
oexp(A) ::= dtid(B) OPENPAR CLOSEPAR.                          { A = ast->node(FTK_oexp, B); }              // Method only output definition
oexp(A) ::= dtid(B) OPENPAR ID(C) AT CLOSEPAR .                { A = ast->node(FTK_oexp, B, C); }           
oexp(A) ::= dtid(B) OPENPAR fldm(C) CLOSEPAR .                 { A = ast->node(FTK_oexp, B, C); }           // Method and field map output definition

fldm(A) ::= fldd(B).                                           { A = ast->node(FTK_fldm, B); }              // fldm is a list of field mappings fldd
fldm(A) ::= fldm(B) COMMA fldd(C).                             { A = ast->nappend(B, C); }                 

fldd(A) ::= ID(B) eqc OPENPAR fldm(C) CLOSEPAR.                { A = ast->node(FTK_fldd, B, C); }           // The right side is another message definition
fldd(A) ::= ID(B) eqc fldr(C).                                 { A = ast->node(FTK_fldd, B, C); }           // The right side is a field expression

fldr(A) ::= vali(B).                                           { A = B; }                                   // The right side can be a literal, 
fldr(A) ::= fldx(B).                                           { A = B; }                                   // a field expression node@[field[.field]] (fldx)
fldr(A) ::= TILDA ID(B) OPENPAR CLOSEPAR.                      { A = ast->node(FTK_fldr, B); }              // or an internal function call
fldr(A) ::= TILDA ID(B) OPENPAR flda(C) CLOSEPAR.              { A = ast->nprepend(C, B); }                 

flda(A) ::= fldr(B).                                           { A = ast->node(FTK_fldr, B); }
flda(A) ::= flda(B) COMMA fldr(C).                             { A = ast->nappend(B, C); }

fldx(A) ::= fldn(B).                                           { A = B; }
fldx(A) ::= fldn(B) dtid(C).                                   { A = ast->chtype(ast->nprepend(C, B), FTK_fldx); }

fldn(A) ::= ID(B) AT.                                          { A = B; }

dtid(A) ::= ID(B).                                             { A = ast->node(FTK_dtid, B); }
dtid(A) ::= dtid(B) DOT ID(C).                                 { A = ast->nappend(B, C); }


eqc ::= EQUALS.
eqc ::= COLON.

bexp(A) ::= vali(B).                                           { A = ast->node(FTK_bexp, B); }
bexp(A) ::= fldx(B).                                           { A = ast->node(FTK_bexp, B); }
bexp(A) ::= HASH(B) fldx(C).                                   { A = ast->node(FTK_bexp, B, C); }
bexp(A) ::= OPENPAR bexp(B) CLOSEPAR.                          { A = B; }
bexp(A) ::= BANG(B) bexp(C).                                   { A = ast->node(FTK_bexp, B, C); }
bexp(A) ::= bexp(B) OR(C) bexp(D).                             { A = ast->node(FTK_bexp, B, C, D); }
bexp(A) ::= bexp(B) AND(C) bexp(D).                            { A = ast->node(FTK_bexp, B, C, D); }
bexp(A) ::= bexp(B) EQ(C) bexp(D).                             { A = ast->node(FTK_bexp, B, C, D); }
bexp(A) ::= bexp(B) NE(C) bexp(D).                             { A = ast->node(FTK_bexp, B, C, D); }
bexp(A) ::= bexp(B) GT(C) bexp(D).                             { A = ast->node(FTK_bexp, B, C, D); }
bexp(A) ::= bexp(B) LT(C) bexp(D).                             { A = ast->node(FTK_bexp, B, C, D); }
bexp(A) ::= bexp(B) LE(C) bexp(D).                             { A = ast->node(FTK_bexp, B, C, D); }
bexp(A) ::= bexp(B) GE(C) bexp(D).                             { A = ast->node(FTK_bexp, B, C, D); }
