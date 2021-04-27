%token_type {int}
%name flow_parser
%token_prefix FTK_

%nonassoc ID STRING INTEGER FLOAT SYMBOL NODE CONTAINER ENTRY IMPORT DEFINE OUTPUT RETURN ERROR ENDPOINT IMAGE MOUNT ENVIRONMENT INPUT HEADERS.
%left SEMICOLON.
%left DOT AT.
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
%right BANG TILDA.
%right DOLLAR.
%right HASH.
%right UMINUS.
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

flow(A) ::= stmt(B).                                           { A = ast->node(FTK_flow, B); }             // FTK_flow is a list of FTK_stmt
flow(A) ::= flow(B) stmt(C).                                   { A = ast->nappend(B, C); }

stmt(A) ::= ID(B) valx(C) SEMICOLON.                           { A = ast->stmt_keyw(B) == FTK_IMPORT? ast->node(FTK_IMPORT, C): ast->node(ast->stmt_keyw(B), B, C); 
                                                                 ast->expect(A, {FTK_DEFINE, FTK_IMPORT}, "import directive or variable definition expected"); 
                                                                 if(ast->stmt_keyw(B) == FTK_DEFINE) ast->define_var(ast->get_id(B), C);
                                                               } 
stmt(A) ::= ID(B) eqsc valx(C) SEMICOLON.                      { A = ast->node(ast->stmt_keyw(B), B, C); ast->expect(A, FTK_DEFINE, "keyword used as variable name");
                                                                 ast->define_var(ast->get_id(B), C);
                                                               } 
stmt(A) ::= ID(B) dtid(C) OPENPAR fldr(E) CLOSEPAR blck(D).    { A = ast->node(ast->stmt_keyw(B), B, C, D, E); ast->expect(A, FTK_NODE, "expected \"node\" keyword"); } 
stmt(A) ::= ID(B) dtid(C) blck(D).                             { A = ast->node(ast->stmt_keyw(B), B, C, D); 
                                                                 ast->expect(A, {FTK_CONTAINER, FTK_NODE, FTK_ENTRY}, "expected \"node\", \"entry\" or \"container\" here"); } 
stmt(A) ::= stmt(B) SEMICOLON.                                 { A = B; }                                  // Skip over extraneous semicolons

blck(A) ::= OPENBRA list(B) CLOSEBRA.                          { A = B; }                                  // Blocks must be enclosed in { }
blck(A) ::= OPENBRA(B) CLOSEBRA.                               { A = ast->chtype(B, FTK_blck); }           // Empty blocks are allowed

// list: the content of a block, is a list of elems

list(A) ::= elem(B).                                           { A = ast->node(FTK_blck, B); }             // FTK_blck is a list of declarations (elem)
list(A) ::= list(B) elem(C).                                   { A = ast->nappend(B, C); }


// elem: any of the definitions allowed in a block

elem(A) ::= ID(B) blck(C).                                     { A = ast->node(FTK_elem, B, C); }       
elem(A) ::= ID(B) lblk(C).                                     { A = ast->node(FTK_elem, B, C); }          
elem(A) ::= ID(B) valx(C) SEMICOLON.                           { A = ast->node(FTK_elem, B, C); }          
elem(A) ::= ID(B) EQUALS valx(C) SEMICOLON.                    { A = ast->node(FTK_elem, B, C); }          
elem(A) ::= ID(B) COLON valx(C) SEMICOLON.                     { A = ast->node(FTK_elem, B, C); }          
elem(A) ::= ID(B) oexp(C) SEMICOLON.                           { A = ast->node(FTK_elem, B, C); }          // Output definition (ID must be 'output')
elem(A) ::= ID(B) rexp(C) SEMICOLON.                           { A = ast->node(FTK_elem, B, C); }          // Output definition (ID must be 'return')
elem(A) ::= elem(B) SEMICOLON.                                 { A = B; }                                  // Skip over extraneous semicolons

lblk(A) ::= ID(B) blck(C).                                     { A = ast->node(FTK_lblk, B, C); }

// valn: any numeric literal

valn(A) ::= INTEGER(B).                                        { A = B; }
valn(A) ::= FLOAT(B).                                          { A = B; }
valn(A) ::= PLUS valn(B).                                      { A = B; } [UMINUS]
valn(A) ::= MINUS valn(B).                                     { A = ast->negate(B); } [UMINUS]

// valx: any literal except enum values

valx(A) ::= STRING(B).                                         { A = B; }
valx(A) ::= valn(B).                                           { A = B; }
valx(A) ::= DOLLAR ID(C).                                      { A = ast->lookup_var(ast->get_id(C)); if(A == 0) ast->error(C, stru1::sfmt() << "reference to undefined symbol \"" << ast->get_id(A=C) << "\""); }

// vall: any literal including enum values 

vall(A) ::= valx(B).                                           { A = B; }
vall(A) ::= dtid(B).                                           { A = ast->chtype(B, FTK_enum); }

// rexp: return expression

rexp(A) ::= OPENPAR fldm(B) CLOSEPAR.                          { A = ast->node(FTK_RETURN, B); }              
rexp(A) ::= OPENPAR ID(B) AT CLOSEPAR.                         { A = ast->node(FTK_RETURN, ast->node(FTK_fldx, B)); }              

// oexp: output expression

oexp(A) ::= dtid(B) OPENPAR ID(C) AT CLOSEPAR .                { A = ast->node(FTK_OUTPUT, B, ast->node(FTK_fldx, C)); }           
oexp(A) ::= dtid(B) OPENPAR fldm(C) CLOSEPAR .                 { A = ast->node(FTK_OUTPUT, B, C); }        // Method and field map output definition

fldm(A) ::= fldd(B).                                           { A = ast->node(FTK_fldm, B); }             // fldm is a list of field definitions fldd
fldm(A) ::= fldm(B) COMMA fldd(C).                             { A = ast->nappend(B, C); }

// fldd: field definition (assignment)

fldd(A) ::= ID(B) eqsc(D) OPENPAR fldm(C) CLOSEPAR.            { A = ast->node(FTK_fldd, B, C); ast->chinteger(A, ast->at(D).token.integer_value); }  // The right side is another message definition
fldd(A) ::= ID(B) eqsc(D) fldr(C).                             { A = ast->node(FTK_fldd, B, C); ast->chinteger(A, ast->at(D).token.integer_value); }  // The right side is a field expression

// fldr: right value in a field assignemnt

fldr(A) ::= vall(B).                                           { A = B; }                                  // The right side can be a literal, 
fldr(A) ::= fldx(B).                                           { A = B; }                                  // a field expression
fldr(A) ::= OPENPAR fldr(B) CLOSEPAR.                          { A = B; }                                 
fldr(A) ::= TILDA ID(B) OPENPAR CLOSEPAR.                      { A = ast->node(FTK_fldr, B); }             // or an internal function call
fldr(A) ::= TILDA ID(B) OPENPAR fldra(C) CLOSEPAR.             { A = ast->nprepend(C, B); }                
fldr(A) ::= HASH(B) fldx(C).                                   { A = ast->node(FTK_fldr, B, C); }          // size of repeated field
fldr(A) ::= BANG(B) fldr(C).                                   { A = ast->node(FTK_fldr, B, C); }
fldr(A) ::= fldr(B) PLUS(C) fldr(D).                           { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) MINUS(C) fldr(D).                          { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) SLASH(C) fldr(D).                          { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) STAR(C) fldr(D).                           { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) PERCENT(C) fldr(D).                        { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) COMP(C) fldr(D).                           { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) EQ(C) fldr(D).                             { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) NE(C) fldr(D).                             { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) LT(C) fldr(D).                             { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) GT(C) fldr(D).                             { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) LE(C) fldr(D).                             { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) GE(C) fldr(D).                             { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) AND(C) fldr(D).                            { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) OR(C) fldr(D).                             { A = ast->node(FTK_fldr, C, B, D); }
fldr(A) ::= fldr(B) QUESTION(C) fldr(D) COLON fldr(E).         { A = ast->node(FTK_fldr, C, B, D, E); }

fldra(A) ::= fldr(B).                                          { A = ast->node(FTK_fldr, B); }
fldra(A) ::= fldra(B) COMMA fldr(C).                           { A = ast->nappend(B, C); }

// fldx: field expression node@[field[.field]]

fldx(A) ::= fldn(B).                                           { A = B; }
fldx(A) ::= fldn(B) dtid(C).                                   { A = ast->chtype(ast->nprepend(C, B), FTK_fldx); }

// fldn: node reference (node@)

fldn(A) ::= ID(B) AT.                                          { A = B; }

// dtid: dotted id id[.id]*

dtid(A) ::= ID(B).                                             { A = ast->node(FTK_dtid, B); }
dtid(A) ::= dtid(B) DOT ID(C).                                 { A = ast->nappend(B, C); }

// eqsc: assignment operator

eqsc(A) ::= EQUALS.                                            { ast->chinteger(A, 0); } 
eqsc(A) ::= COLON.                                             { ast->chinteger(A, 0); } 

