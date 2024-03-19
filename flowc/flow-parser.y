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
    SCALE
    TIMEOUT
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
%left NE EQ.
%left GT LT GE LE.
%left COMP.
%left SHR SHL.
%left PLUS MINUS.
%left STAR SLASH PERCENT.
%right POW.
%right BANG TILDA.
%right DOLLAR.
%right HASH.
%right OPENPAR CLOSEPAR OPENBRA CLOSEBRA OPENSQB CLOSESQB.
%left AT. 
%left DOT.

%fallback STRING URL.
%wildcard ANY.

%include {
#include "flow-comp.H"
#include "sfmt.H"
// undefine this to disable error productions
//#define YYNOERRORRECOVERY
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

flow(A) ::= stmts(B).                                        { A = ast->node(FTK_ACCEPT, B); }                                   
stmts(A) ::= stmt(B).                                        { A = ast->node(FTK_flow, B); }    // FTK_flow is a list of FTK_stmt
stmts(A) ::= stmts(B) stmt(C).                               { A = ast->nappend(B, C); }

// directives & properties defaults
stmt(A) ::= INCLUDE(B) valx(C) SEMICOLON.                    { A = ast->nappend(B, C); } 
stmt(A) ::= IMPORT(B) valx(C) SEMICOLON.                     { A = ast->nappend(B, C); } 
stmt(A) ::= INPUT(B) id(C) SEMICOLON.                        { A = ast->nappend(B, C); }

// assignments
stmt(A) ::= assign(B).                                       { A = B; }

// container 
stmt(A) ::= CONTAINER(K) id(N) bblock(B).                    { A = ast->nappend(K, N, B); }

// node 
stmt(A) ::= NODE(K) id(N) fcond(C) bblock(B).                { A = ast->nappend(K, N, B, C); }
stmt(A) ::= NODE(K) id(N) bblock(B).                         { A = ast->nappend(K, N, B); }
stmt(A) ::= NODE(K) id(N) COLON id(T) fcond(C) bblock(B).    { A = ast->nappend(K, N, T, B, C); }
stmt(A) ::= NODE(K) id(N) COLON id(T) bblock(B).             { A = ast->nappend(K, N, T, B); }

// error check
stmt(A) ::= ERRCHK(K) fcond(C) SEMICOLON.                       { A = ast->nappend(K, C); }
stmt(A) ::= ERRCHK(K) fcond(C) valx(M) SEMICOLON.               { A = ast->nappend(K, C, M); }
stmt(A) ::= ERRCHK(K) fcond(C) valx(N) COMMA valx(M) SEMICOLON. { A = ast->nappend(K, C, N, M); }

// entries
stmt(A) ::= ENTRY(K) did(N) OPENPAR id(I) CLOSEPAR bblock(B). { A = ast->nappend(K, N, ast->node(FTK_INPUT, I), B); }
stmt(A) ::= ENTRY(K) did(N) bblock(B).                        { A = ast->nappend(K, N, B); }

// node condition
fcond(A) ::= OPENPAR valx(B) CLOSEPAR. { A = B; }

// empty statement
stmt ::= SEMICOLON. {}

//////////////////////////////////////////////////////////
// container/node body

bblock(A) ::= OPENBRA(C) block(B) CLOSEBRA.                    { A = ast->chtype(ast->graft(C, B), FTK_block); }

// empty blocks and error stopper
bblock(A) ::= OPENBRA(C) CLOSEBRA.                             { A = ast->chtype(C, FTK_block); }
bblock(A) ::= SEMICOLON(C).                                    { A = ast->chtype(C, FTK_block); }
bblock ::= OPENBRA error CLOSEBRA.

block(A) ::= blke(B).                                          { A = ast->node(FTK_list, B); }
block(A) ::= block(B) blke(C).                                 { A = ast->nappend(B, C); }

blke(A) ::= assign(B).                                         { A = B; }

blke(A) ::= RETURN(K) valx(X) SEMICOLON.                       { A = ast->nappend(K, X); }
blke(A) ::= OUTPUT(K) valx(X) SEMICOLON.                     { 
    if(ast->child_count(X) != 1 || ast->node_type(ast->child(X, 0)) != FTK_msgexp || ast->node_type(ast->descendant(X, 0, 0)) != FTK_did) 
        ast->error(ast->at(X), "rpc call expected as \"output\" argument");
    A = ast->nappend(K, X); 
}

blke(A) ::= ERRCHK(K) valx(M) SEMICOLON.                       { A = ast->nappend(K, M); }
blke(A) ::= ERRCHK(K) valx(N) COMMA valx(M) SEMICOLON.         { A = ast->nappend(K, N, M); }

blke(A) ::= ENVIRONMENT(B) assign(C).                          { A = ast->nappend(B, C); }
blke(A) ::= ENVIRONMENT(B) OPENBRA alst(C) CLOSEBRA.           { A = ast->graft(B, C); /* assign c's children to b */}
blke(A) ::= ENVIRONMENT(B) OPENBRA CLOSEBRA.                   { A = B; }
blke ::= ENVIRONMENT OPENBRA error CLOSEBRA.                   

blke(A) ::= HEADER(B) vassgn(C).                               { A = ast->nappend(B, C); }
blke(A) ::= HEADER(B) OPENBRA vlst(C) CLOSEBRA.                { A = ast->graft(B, C); }
blke ::= HEADER OPENBRA CLOSEBRA.                              
blke ::= HEADER OPENBRA error CLOSEBRA.                        

blke(A) ::= lment(B).                                          { A = B; }

lment(A) ::= limit(L) lmval(V) SEMICOLON.                      { A = ast->node(FTK_LIMIT, L, V); }
lment(A) ::= LIMIT(K) id(L) lmval(V) SEMICOLON.                { A = ast->nappend(K, L, V); }
lment(A) ::= LIMIT(K) id(L) eqorc lmval(V) SEMICOLON.          { A = ast->nappend(K, L, V); }
lment(A) ::= LIMIT OPENBRA lmlst(L) CLOSEBRA.                  { A = L; }
lment ::= LIMIT OPENBRA error CLOSEBRA.

lmlst(A) ::= id(L) lmval(V) SEMICOLON.                         { A = ast->node(FTK_list, ast->node(FTK_LIMIT, L, V)); }
lmlst(A) ::= id(L) eqorc lmval(V) SEMICOLON.                   { A = ast->node(FTK_list, ast->node(FTK_LIMIT, L, V)); }
lmlst(A) ::= lmlst(B) id(L) lmval(V) SEMICOLON.                { A = ast->nappend(B, ast->node(FTK_LIMIT, L, V)); }

// empty entries and error stopper
lmlst(A) ::= lmlst(B) SEMICOLON.                               { A = B; }
lmlst ::= error SEMICOLON.                          

blke ::= SEMICOLON.                                            {} 
blke ::= error SEMICOLON.                                     

limit(A) ::= CPU(B).                                           { A = B; }
limit(A) ::= ENDPOINT(B).                                      { A = B; }
limit(A) ::= GPU(B).                                           { A = B; }
limit(A) ::= GROUP(B).                                         { A = B; }
limit(A) ::= IMAGE(B).                                         { A = B; }
limit(A) ::= MEMORY(B).                                        { A = B; }
limit(A) ::= PORT(B).                                          { A = B; }
limit(A) ::= REPO(B).                                          { A = B; }
limit(A) ::= TIMEOUT(B).                                       { A = B; }
limit(A) ::= SCALE(B).                                         { A = B; }

lmval(A) ::= valx(B).                                          { A = B; } 
lmval(A) ::= valx(B) MBU(U).                                   { A = ast->nappend(B, U); } 
lmval(A) ::= range(B).                                         { A = B; } 
lmval(A) ::= range(B) MBU(U).                                  { A = ast->nappend(B, U); } 

// assignments list
alst(A) ::= assign(B).                                         { A = ast->node(FTK_list, B); }
alst(A) ::= alst(B) assign(C).                                 { A = ast->nappend(B, C); }

// list of colon separated expression tuples
vlst(A) ::= vassgn(B).                                         { A = ast->node(FTK_list, B); }
vlst(A) ::= vlst(B) vassgn(C).                                 { A = ast->nappend(B, C);}

//////////////////////////////////////////////////////////
// message constructor 

msgexp(A) ::= did(M) msgctr(C).                                { A = ast->node(FTK_msgexp, M, C); }
msgexp(A) ::= msgctr(C).                                       { A = ast->node(FTK_msgexp, C); }

msgctr(A) ::= OPENPAR CLOSEPAR(P).                             { A = ast->chtype(P, FTK_list); }
msgctr(A) ::= OPENPAR faslst(B) CLOSEPAR.                      { A = B; }
msgctr ::= OPENPAR error CLOSEPAR.    

faslst(A) ::= fassgn(B).                                       { A = ast->node(FTK_list, B); }
faslst(A) ::= faslst(B) COMMA fassgn(C).                       { A = ast->nappend(B, C); }
faslst(A) ::= faslst(B) COMMA.                                 { A = B; /* ignore spurious commas */}

fassgn(A) ::= id(B) eqorc valx(C).                             { A = ast->node(FTK_fassgn, B, C); }

eqorc(A) ::= COLON(B).                                         { A = B; }
eqorc(A) ::= EQUALS(B).                                        { A = B; }

//////////////////////////////////////////////////////////
// assignments 

assign(A) ::= id(I) EQUALS(O) valx(R) SEMICOLON.               { A = ast->nappend(O, I, R); if(ast->vtype.has(R)) ast->vtype.copy(R, O); }
vassgn(A) ::= valx(L) COLON(O) valx(R) SEMICOLON.              { A = ast->nappend(O, L, R); }

//////////////////////////////////////////////////////////
// literals

// any string literal
vals(A) ::= STRING(B).                                         { A = B; }
vals(A) ::= DOLLAR(B) id(C).                                   { A = ast->nappend(B, C); }

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
id(A) ::= TIMEOUT(B).   { A = ast->chtype(B, FTK_ID); } 
id(A) ::= SCALE(B).   { A = ast->chtype(B, FTK_ID); } 

//////////////////////////////////////////////////////////
// field, node and types references

idex(A) ::= id(B).                 { A = ast->node(FTK_did, B); }
idex(A) ::= idex(L) DOT idex(R).   { A = ast->graft(L, R); } 
idex(A) ::= idex(L) AT(O) idex(R).    { 
    /* check that L is singleton and R does not refer to a node */
    if(ast->at(L).type != FTK_did || ast->at(L).children.size() > 1) 
        ast->error(ast->at(O), stru::sfmt() << "left side of node expression must be a node identifier not a field reference");
    A = ast->chtype(ast->graft(L, R), FTK_ndid);
}
idex(A) ::= idex(L) AT(O).    { 
    /* check that L is singleton and R does not refer to a node */
    if(ast->at(L).type != FTK_did || ast->at(L).children.size() > 1)
        ast->error(ast->at(O), stru::sfmt() << "left side of node expression already refers to a node");
    A = ast->chtype(L, FTK_ndid);
}
did(A) ::= idex(B). { 
    /* check that there is no node reference  */
    if(ast->at(B).type != FTK_did)
        ast->error(ast->at(B), stru::sfmt() << "cannot reference a node here");
    A = B;
}

//////////////////////////////////////////////////////////
// constructs

// ranges
range(A) ::= valx(L) COLON(O) valx(R) COLON valx(S).        { A = ast->nappend(ast->chtype(O, FTK_range), L, R, S); }                  [BANG]
range(A) ::= valx(L) COLON(O) valx(R).                      { A = ast->nappend(ast->chtype(O, FTK_range), L, R); }                     [BANG]
range(A) ::= COLON(O) valx(R).                              { A = ast->nappend(ast->chtype(O, FTK_range), ast->node(FTK_STAR), R); }   [BANG]
range(A) ::= valx(L) COLON(O).                              { A = ast->nappend(ast->chtype(O, FTK_range), L, ast->node(FTK_STAR)); }   [BANG]

// comma separated list
vala(A) ::= valx(B).                                        { A = ast->node(FTK_list, B); }
vala(A) ::= vala(B) COMMA valx(C).                          { A = ast->nappend(B, C); }

//////////////////////////////////////////////////////////
// expression

valx(A) ::= vals(B).                                        { A = ast->node(FTK_valx, B); ast->vtype.set(A, fc::value_type(fc::fvt_str)); }
valx(A) ::= INTEGER(B).                                     { A = ast->node(FTK_valx, B); ast->vtype.set(A, fc::value_type(fc::fvt_int)); }
valx(A) ::= FLOAT(B).                                       { A = ast->node(FTK_valx, B); ast->vtype.set(A, fc::value_type(fc::fvt_flt)); }
valx(A) ::= idex(B).                                        { A = ast->node(FTK_valx, B); } [OPENPAR] 
valx(A) ::= msgexp(B).                                      { A = ast->node(FTK_valx, B); }
valx(A) ::= OPENPAR valx(B) CLOSEPAR.                       { A = B; } 
valx(A) ::= OPENSQB vala(L) CLOSESQB.                       { A = L; if(ast->vtype.has(L)) ast->vtype.copy(L, A); }
valx(A) ::= OPENSQB CLOSESQB(E).                            { A = ast->chtype(E, FTK_list); }
valx(A) ::= OPENSQB range(R) CLOSESQB.                      { 
    /* do not accept open ended ranges here */ 
    if(ast->node_type(ast->child(R, 0)) == FTK_STAR || ast->node_type(ast->child(R, 1)) == FTK_STAR) 
        ast->error(ast->at(R), "cannot initialize list from open range");
    A = R; 
}

valx(A) ::= TILDA id(F) OPENPAR CLOSEPAR.                   { A = ast->node(FTK_valx, ast->node(FTK_fun, F)); }
valx(A) ::= TILDA id(F) OPENPAR vala(L) CLOSEPAR.           { A = ast->node(FTK_valx, ast->graft(ast->node(FTK_fun, F), L, 1)); }

valx(A) ::= PLUS valx(X).                                   { A = X; } [BANG]
valx(A) ::= MINUS(O) valx(X).                               { A = ast->node(FTK_valx, O, X); ast->precedence.set(A, 3); ast->is_operator.set(O, true); } [BANG]
valx(A) ::= HASH(O) valx(X).                                { A = ast->node(FTK_valx, O, X); ast->precedence.set(A, 3); ast->is_operator.set(O, true); } // size of repeated field
valx(A) ::= BANG(O) valx(X).                                { A = ast->node(FTK_valx, O, X); ast->precedence.set(A, 3); ast->is_operator.set(O, true); }
/** bitwise NOT
valx(A) ::= BNOT(O) valx(X).                                { A = ast->node(FTK_valx, O, X); ast->precedence.set(A, 3); }
*/
valx(A) ::= valx(L) PLUS(O) valx(R).                        { A = ast->node(FTK_valx, O, L, R); ast->precedence.set(A, 6); ast->is_operator.set(O, true); }
valx(A) ::= valx(L) MINUS(O) valx(R).                       { A = ast->node(FTK_valx, O, L, R); ast->precedence.set(A, 6); ast->is_operator.set(O, true); }  
valx(A) ::= valx(L) SLASH(O) valx(R).                       { A = ast->node(FTK_valx, O, L, R); ast->precedence.set(A, 5); ast->is_operator.set(O, true); }  
valx(A) ::= valx(L) STAR(O) valx(R).                        { A = ast->node(FTK_valx, O, L, R); ast->precedence.set(A, 5); ast->is_operator.set(O, true); }  
valx(A) ::= valx(L) PERCENT(O) valx(R).                     { A = ast->node(FTK_valx, O, L, R); ast->precedence.set(A, 5); ast->is_operator.set(O, true); }  
valx(A) ::= valx(L) POW(O) valx(R).                         { A = ast->node(FTK_valx, O, L, R); }  

valx(A) ::= valx(B) COMP(O) valx(D).                        { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 8);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) EQ(O) valx(D).                          { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 10); ast->is_operator.set(O, true); }  
valx(A) ::= valx(B) NE(O) valx(D).                          { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 10);  ast->is_operator.set(O, true);}
valx(A) ::= valx(B) LT(O) valx(D).                          { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 9);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) GT(O) valx(D).                          { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 9);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) LE(O) valx(D).                          { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 9);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) GE(O) valx(D).                          { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 9);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) AMP(O) valx(D).                         { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 11);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) BAR(O) valx(D).                         { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 13);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) CARET(O) valx(D).                       { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 12);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) AND(O) valx(D).                         { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 14);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) OR(O) valx(D).                          { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 15);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) SHL(O) valx(D).                         { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 7);  ast->is_operator.set(O, true);}  
valx(A) ::= valx(B) SHR(O) valx(D).                         { A = ast->node(FTK_valx, O, B, D); ast->precedence.set(A, 7);  ast->is_operator.set(O, true);}   
valx(A) ::= valx(B) QUESTION(O) valx(D) COLON valx(E).      { A = ast->node(FTK_valx, O, B, D, E); ast->precedence.set(A, 16);  ast->is_operator.set(O, true);}

