/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 27 "flow-parser.y"

#include <iostream>
#include "flow-ast.H"
#define YYNOERRORRECOVERY
#line 13 "flow-parser.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    flow_parserTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is flow_parserTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    flow_parserARG_SDECL     A static variable declaration for the %extra_argument
**    flow_parserARG_PDECL     A parameter declaration for the %extra_argument
**    flow_parserARG_STORE     Code to store %extra_argument into yypParser
**    flow_parserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 64
#define YYACTIONTYPE unsigned char
#define flow_parserTOKENTYPE int
typedef union {
  int yyinit;
  flow_parserTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define flow_parserARG_SDECL  flow_ast *ast ;
#define flow_parserARG_PDECL , flow_ast *ast 
#define flow_parserARG_FETCH  flow_ast *ast  = yypParser->ast 
#define flow_parserARG_STORE yypParser->ast  = ast 
#define YYNSTATE 114
#define YYNRULE 62
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    34,   93,   99,   98,    2,   59,   93,   99,   98,   71,
 /*    10 */    72,   41,   29,  104,   55,   55,  102,   50,   17,   52,
 /*    20 */    53,   92,   71,   72,   24,   23,  177,   28,   46,   24,
 /*    30 */    23,   42,   65,   22,    6,   19,   21,    1,    4,   13,
 /*    40 */     5,    3,   18,   19,    8,    7,    9,   10,   11,   12,
 /*    50 */    13,    5,   45,   65,   63,    8,    7,    9,   10,   11,
 /*    60 */    12,   94,   59,   93,   99,   98,   59,   93,   99,   98,
 /*    70 */     5,   27,   33,   66,    8,    7,    9,   10,   11,   12,
 /*    80 */     8,    7,    9,   10,   11,   12,   24,   23,   44,   65,
 /*    90 */    24,   23,  109,   54,   95,   20,   93,   99,   98,   76,
 /*   100 */   110,   56,   91,   92,   32,   56,   70,  106,   43,   32,
 /*   110 */    96,   87,   54,   38,   87,   54,   97,   14,   76,   24,
 /*   120 */    23,   76,   92,   99,   98,   92,   88,    1,   32,   88,
 /*   130 */    25,   32,   87,   54,   40,   67,   93,   99,   98,   76,
 /*   140 */    85,   31,   47,   92,   31,   24,   23,   88,   77,   32,
 /*   150 */    49,   87,   54,   39,   87,   54,  103,   67,   76,   24,
 /*   160 */    23,   76,   92,   84,   78,   92,   88,   79,   32,   88,
 /*   170 */    64,   32,   87,   54,  105,   87,   54,   68,   60,   76,
 /*   180 */    71,   72,   76,   92,   58,   19,   92,   88,   74,   32,
 /*   190 */    88,   82,   32,   87,   54,  108,   90,   31,   48,   26,
 /*   200 */    76,   80,   51,  107,   92,   76,   92,  101,   88,   92,
 /*   210 */    32,   87,   54,   37,   59,   93,   99,   98,   76,   55,
 /*   220 */   109,   54,   92,   83,  114,   16,   88,   76,   32,  109,
 /*   230 */    54,   92,   30,   62,  111,  106,   76,   32,   24,   23,
 /*   240 */    92,  100,   15,  113,  106,   61,   32,   71,   72,   89,
 /*   250 */    87,   54,   36,   56,   86,   73,   57,   76,   16,   69,
 /*   260 */    75,   92,   87,   54,   35,   88,   81,   32,  112,   76,
 /*   270 */   178,  178,  178,   92,  178,  178,  178,   88,  178,   32,
 /*   280 */   178,  178,    9,   10,   11,   12,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     1,    2,    3,    4,   34,    1,    2,    3,    4,   10,
 /*    10 */    11,   46,    1,   48,    7,    7,   51,   52,   53,   54,
 /*    20 */    55,   56,   10,   11,   25,   26,   42,   43,   44,   25,
 /*    30 */    26,   57,   58,   34,   30,   36,   32,    1,   34,   13,
 /*    40 */    14,   34,   34,   36,   18,   19,   20,   21,   22,   23,
 /*    50 */    13,   14,   57,   58,    1,   18,   19,   20,   21,   22,
 /*    60 */    23,   35,    1,    2,    3,    4,    1,    2,    3,    4,
 /*    70 */    14,    1,   35,   37,   18,   19,   20,   21,   22,   23,
 /*    80 */    18,   19,   20,   21,   22,   23,   25,   26,   57,   58,
 /*    90 */    25,   26,   45,   46,   56,   34,    2,    3,    4,   52,
 /*   100 */    35,   40,   60,   56,   62,   40,   59,   60,   61,   62,
 /*   110 */    56,   45,   46,   47,   45,   46,   47,   53,   52,   25,
 /*   120 */    26,   52,   56,    3,    4,   56,   60,    1,   62,   60,
 /*   130 */     1,   62,   45,   46,   47,    1,    2,    3,    4,   52,
 /*   140 */    48,    9,   44,   56,    9,   25,   26,   60,   58,   62,
 /*   150 */    50,   45,   46,   47,   45,   46,   47,    1,   52,   25,
 /*   160 */    26,   52,   56,   37,   35,   56,   60,   35,   62,   60,
 /*   170 */    35,   62,   45,   46,   47,   45,   46,   47,    8,   52,
 /*   180 */    10,   11,   52,   56,   46,   36,   56,   60,   48,   62,
 /*   190 */    60,   48,   62,   45,   46,   47,    6,    9,   45,   46,
 /*   200 */    52,    6,   52,    6,   56,   52,   56,    6,   60,   56,
 /*   210 */    62,   45,   46,   47,    1,    2,    3,    4,   52,    7,
 /*   220 */    45,   46,   56,   35,    0,    1,   60,   52,   62,   45,
 /*   230 */    46,   56,   49,   50,   59,   60,   52,   62,   25,   26,
 /*   240 */    56,    6,    9,   59,   60,    8,   62,   10,   11,    6,
 /*   250 */    45,   46,   47,   40,    6,    1,    1,   52,    1,    8,
 /*   260 */    35,   56,   45,   46,   47,   60,   35,   62,   35,   52,
 /*   270 */    63,   63,   63,   56,   63,   63,   63,   60,   63,   62,
 /*   280 */    63,   63,   20,   21,   22,   23,
};
#define YY_SHIFT_USE_DFLT (-31)
#define YY_SHIFT_MAX 63
static const short yy_shift_ofst[] = {
 /*     0 */   257,   -1,   65,    4,    4,    4,    4,    4,    4,    4,
 /*    10 */     4,    4,    4,    4,   61,  213,  134,   94,  129,  126,
 /*    20 */    11,   53,   70,  120,  120,  237,    7,  170,  224,   12,
 /*    30 */    36,   11,  156,  149,  149,   26,   37,   56,   62,  262,
 /*    40 */   262,    8,  132,  233,  188,  135,  190,  190,  195,  197,
 /*    50 */   201,  235,  243,  248,  212,  254,  255,  -30,  212,  251,
 /*    60 */   225,  231,  197,  251,
};
#define YY_REDUCE_USE_DFLT (-36)
#define YY_REDUCE_MAX 34
static const short yy_reduce_ofst[] = {
 /*     0 */   -16,  -35,   47,  205,  217,   66,   69,   87,  106,  109,
 /*    10 */   127,  130,  148,  166,  175,  184,  153,  150,   31,  183,
 /*    20 */    -5,   42,  -26,   38,   54,   64,   92,   64,   98,   64,
 /*    30 */   100,   90,  138,  140,  143,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   176,  176,  176,  176,  176,  176,  176,  176,  176,  176,
 /*    10 */   176,  176,  176,  176,  176,  176,  176,  176,  176,  176,
 /*    20 */   176,  176,  176,  176,  176,  176,  140,  176,  176,  176,
 /*    30 */   176,  176,  156,  176,  159,  176,  176,  168,  169,  171,
 /*    40 */   170,  176,  176,  176,  176,  176,  115,  116,  176,  124,
 /*    50 */   176,  176,  176,  176,  140,  176,  176,  176,  157,  159,
 /*    60 */   176,  176,  123,  176,  148,  146,  121,  159,  175,  158,
 /*    70 */   154,  161,  162,  160,  118,  142,  139,  147,  143,  141,
 /*    80 */   117,  144,  132,  145,  122,  119,  130,  163,  164,  129,
 /*    90 */   120,  165,  138,  137,  166,  136,  135,  167,  134,  133,
 /*   100 */   128,  127,  126,  172,  125,  173,  151,  131,  174,  150,
 /*   110 */   152,  149,  153,  155,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  flow_parserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void flow_parserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "ID",            "STRING",        "INTEGER",     
  "FLOAT",         "SYMBOL",        "SEMICOLON",     "DOT",         
  "AT",            "COMMA",         "EQUALS",        "COLON",       
  "QUESTION",      "OR",            "AND",           "BAR",         
  "CARET",         "AMP",           "NE",            "EQ",          
  "GT",            "LT",            "GE",            "LE",          
  "COMP",          "PLUS",          "MINUS",         "STAR",        
  "SLASH",         "PERCENT",       "BANG",          "DOLLAR",      
  "HASH",          "UMINUS",        "OPENPAR",       "CLOSEPAR",    
  "OPENBRA",       "CLOSEBRA",      "OPENSQB",       "CLOSESQB",    
  "TILDA",         "error",         "main",          "flow",        
  "stmt",          "vall",          "dtid",          "bexp",        
  "blck",          "list",          "elem",          "lblk",        
  "valx",          "eqc",           "oexp",          "rexp",        
  "valn",          "fldm",          "fldd",          "fldr",        
  "fldx",          "flda",          "fldn",        
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= flow",
 /*   1 */ "flow ::= stmt",
 /*   2 */ "flow ::= flow stmt",
 /*   3 */ "stmt ::= ID vall SEMICOLON",
 /*   4 */ "stmt ::= ID dtid OPENPAR bexp CLOSEPAR blck",
 /*   5 */ "stmt ::= ID dtid blck",
 /*   6 */ "stmt ::= stmt SEMICOLON",
 /*   7 */ "blck ::= OPENBRA list CLOSEBRA",
 /*   8 */ "blck ::= OPENBRA CLOSEBRA",
 /*   9 */ "list ::= elem",
 /*  10 */ "list ::= list elem",
 /*  11 */ "elem ::= ID blck",
 /*  12 */ "elem ::= ID lblk",
 /*  13 */ "elem ::= ID valx SEMICOLON",
 /*  14 */ "elem ::= ID eqc valx SEMICOLON",
 /*  15 */ "elem ::= ID oexp SEMICOLON",
 /*  16 */ "elem ::= ID rexp SEMICOLON",
 /*  17 */ "elem ::= elem SEMICOLON",
 /*  18 */ "lblk ::= ID blck",
 /*  19 */ "valn ::= INTEGER",
 /*  20 */ "valn ::= FLOAT",
 /*  21 */ "valn ::= PLUS valn",
 /*  22 */ "valn ::= MINUS valn",
 /*  23 */ "valx ::= STRING",
 /*  24 */ "valx ::= valn",
 /*  25 */ "vall ::= valx",
 /*  26 */ "vall ::= dtid",
 /*  27 */ "rexp ::= OPENPAR fldm CLOSEPAR",
 /*  28 */ "rexp ::= OPENPAR ID AT CLOSEPAR",
 /*  29 */ "oexp ::= dtid OPENPAR CLOSEPAR",
 /*  30 */ "oexp ::= dtid OPENPAR ID AT CLOSEPAR",
 /*  31 */ "oexp ::= dtid OPENPAR fldm CLOSEPAR",
 /*  32 */ "fldm ::= fldd",
 /*  33 */ "fldm ::= fldm COMMA fldd",
 /*  34 */ "fldd ::= ID eqc OPENPAR fldm CLOSEPAR",
 /*  35 */ "fldd ::= ID eqc fldr",
 /*  36 */ "fldr ::= vall",
 /*  37 */ "fldr ::= fldx",
 /*  38 */ "fldr ::= TILDA ID OPENPAR CLOSEPAR",
 /*  39 */ "fldr ::= TILDA ID OPENPAR flda CLOSEPAR",
 /*  40 */ "flda ::= fldr",
 /*  41 */ "flda ::= flda COMMA fldr",
 /*  42 */ "fldx ::= fldn",
 /*  43 */ "fldx ::= fldn dtid",
 /*  44 */ "fldn ::= ID AT",
 /*  45 */ "dtid ::= ID",
 /*  46 */ "dtid ::= dtid DOT ID",
 /*  47 */ "eqc ::= EQUALS",
 /*  48 */ "eqc ::= COLON",
 /*  49 */ "bexp ::= vall",
 /*  50 */ "bexp ::= fldx",
 /*  51 */ "bexp ::= HASH fldx",
 /*  52 */ "bexp ::= OPENPAR bexp CLOSEPAR",
 /*  53 */ "bexp ::= BANG bexp",
 /*  54 */ "bexp ::= bexp OR bexp",
 /*  55 */ "bexp ::= bexp AND bexp",
 /*  56 */ "bexp ::= bexp EQ bexp",
 /*  57 */ "bexp ::= bexp NE bexp",
 /*  58 */ "bexp ::= bexp GT bexp",
 /*  59 */ "bexp ::= bexp LT bexp",
 /*  60 */ "bexp ::= bexp LE bexp",
 /*  61 */ "bexp ::= bexp GE bexp",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to flow_parser and flow_parserFree.
*/
void *flow_parserAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  flow_parserARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from flow_parserAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void flow_parserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int flow_parserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   flow_parserARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
   flow_parserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 42, 1 },
  { 43, 1 },
  { 43, 2 },
  { 44, 3 },
  { 44, 6 },
  { 44, 3 },
  { 44, 2 },
  { 48, 3 },
  { 48, 2 },
  { 49, 1 },
  { 49, 2 },
  { 50, 2 },
  { 50, 2 },
  { 50, 3 },
  { 50, 4 },
  { 50, 3 },
  { 50, 3 },
  { 50, 2 },
  { 51, 2 },
  { 56, 1 },
  { 56, 1 },
  { 56, 2 },
  { 56, 2 },
  { 52, 1 },
  { 52, 1 },
  { 45, 1 },
  { 45, 1 },
  { 55, 3 },
  { 55, 4 },
  { 54, 3 },
  { 54, 5 },
  { 54, 4 },
  { 57, 1 },
  { 57, 3 },
  { 58, 5 },
  { 58, 3 },
  { 59, 1 },
  { 59, 1 },
  { 59, 4 },
  { 59, 5 },
  { 61, 1 },
  { 61, 3 },
  { 60, 1 },
  { 60, 2 },
  { 62, 2 },
  { 46, 1 },
  { 46, 3 },
  { 53, 1 },
  { 53, 1 },
  { 47, 1 },
  { 47, 1 },
  { 47, 2 },
  { 47, 3 },
  { 47, 2 },
  { 47, 3 },
  { 47, 3 },
  { 47, 3 },
  { 47, 3 },
  { 47, 3 },
  { 47, 3 },
  { 47, 3 },
  { 47, 3 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  flow_parserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* main ::= flow */
#line 45 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_ACCEPT, yymsp[0].minor.yy0); }
#line 843 "flow-parser.c"
        break;
      case 1: /* flow ::= stmt */
#line 47 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_flow, yymsp[0].minor.yy0); }
#line 848 "flow-parser.c"
        break;
      case 2: /* flow ::= flow stmt */
      case 10: /* list ::= list elem */ yytestcase(yyruleno==10);
#line 48 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 854 "flow-parser.c"
        break;
      case 3: /* stmt ::= ID vall SEMICOLON */
#line 50 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_stmt, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); }
#line 859 "flow-parser.c"
        break;
      case 4: /* stmt ::= ID dtid OPENPAR bexp CLOSEPAR blck */
#line 51 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_stmt, yymsp[-5].minor.yy0, yymsp[-4].minor.yy0, yymsp[0].minor.yy0, yymsp[-2].minor.yy0); }
#line 864 "flow-parser.c"
        break;
      case 5: /* stmt ::= ID dtid blck */
#line 52 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_stmt, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 869 "flow-parser.c"
        break;
      case 6: /* stmt ::= stmt SEMICOLON */
      case 7: /* blck ::= OPENBRA list CLOSEBRA */ yytestcase(yyruleno==7);
      case 17: /* elem ::= elem SEMICOLON */ yytestcase(yyruleno==17);
      case 44: /* fldn ::= ID AT */ yytestcase(yyruleno==44);
      case 52: /* bexp ::= OPENPAR bexp CLOSEPAR */ yytestcase(yyruleno==52);
#line 53 "flow-parser.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 878 "flow-parser.c"
        break;
      case 8: /* blck ::= OPENBRA CLOSEBRA */
#line 56 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(yymsp[-1].minor.yy0, FTK_blck); }
#line 883 "flow-parser.c"
        break;
      case 9: /* list ::= elem */
#line 58 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_blck, yymsp[0].minor.yy0); }
#line 888 "flow-parser.c"
        break;
      case 11: /* elem ::= ID blck */
      case 12: /* elem ::= ID lblk */ yytestcase(yyruleno==12);
#line 61 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 894 "flow-parser.c"
        break;
      case 13: /* elem ::= ID valx SEMICOLON */
      case 15: /* elem ::= ID oexp SEMICOLON */ yytestcase(yyruleno==15);
      case 16: /* elem ::= ID rexp SEMICOLON */ yytestcase(yyruleno==16);
#line 63 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); }
#line 901 "flow-parser.c"
        break;
      case 14: /* elem ::= ID eqc valx SEMICOLON */
#line 64 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 906 "flow-parser.c"
        break;
      case 18: /* lblk ::= ID blck */
#line 69 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_lblk, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 911 "flow-parser.c"
        break;
      case 19: /* valn ::= INTEGER */
      case 20: /* valn ::= FLOAT */ yytestcase(yyruleno==20);
      case 21: /* valn ::= PLUS valn */ yytestcase(yyruleno==21);
      case 23: /* valx ::= STRING */ yytestcase(yyruleno==23);
      case 24: /* valx ::= valn */ yytestcase(yyruleno==24);
      case 25: /* vall ::= valx */ yytestcase(yyruleno==25);
      case 26: /* vall ::= dtid */ yytestcase(yyruleno==26);
      case 36: /* fldr ::= vall */ yytestcase(yyruleno==36);
      case 37: /* fldr ::= fldx */ yytestcase(yyruleno==37);
      case 42: /* fldx ::= fldn */ yytestcase(yyruleno==42);
#line 74 "flow-parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 925 "flow-parser.c"
        break;
      case 22: /* valn ::= MINUS valn */
#line 77 "flow-parser.y"
{ yygotominor.yy0 = ast->negate(yymsp[0].minor.yy0); }
#line 930 "flow-parser.c"
        break;
      case 27: /* rexp ::= OPENPAR fldm CLOSEPAR */
#line 89 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_rexp, yymsp[-1].minor.yy0); }
#line 935 "flow-parser.c"
        break;
      case 28: /* rexp ::= OPENPAR ID AT CLOSEPAR */
#line 90 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_rexp, yymsp[-2].minor.yy0); }
#line 940 "flow-parser.c"
        break;
      case 29: /* oexp ::= dtid OPENPAR CLOSEPAR */
#line 91 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_oexp, yymsp[-2].minor.yy0); }
#line 945 "flow-parser.c"
        break;
      case 30: /* oexp ::= dtid OPENPAR ID AT CLOSEPAR */
#line 92 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_oexp, yymsp[-4].minor.yy0, yymsp[-2].minor.yy0); }
#line 950 "flow-parser.c"
        break;
      case 31: /* oexp ::= dtid OPENPAR fldm CLOSEPAR */
#line 93 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_oexp, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 955 "flow-parser.c"
        break;
      case 32: /* fldm ::= fldd */
#line 95 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldm, yymsp[0].minor.yy0); }
#line 960 "flow-parser.c"
        break;
      case 33: /* fldm ::= fldm COMMA fldd */
      case 41: /* flda ::= flda COMMA fldr */ yytestcase(yyruleno==41);
      case 46: /* dtid ::= dtid DOT ID */ yytestcase(yyruleno==46);
#line 96 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 967 "flow-parser.c"
        break;
      case 34: /* fldd ::= ID eqc OPENPAR fldm CLOSEPAR */
#line 98 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-4].minor.yy0, yymsp[-1].minor.yy0); }
#line 972 "flow-parser.c"
        break;
      case 35: /* fldd ::= ID eqc fldr */
#line 99 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 977 "flow-parser.c"
        break;
      case 38: /* fldr ::= TILDA ID OPENPAR CLOSEPAR */
#line 103 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-2].minor.yy0); }
#line 982 "flow-parser.c"
        break;
      case 39: /* fldr ::= TILDA ID OPENPAR flda CLOSEPAR */
#line 104 "flow-parser.y"
{ yygotominor.yy0 = ast->nprepend(yymsp[-1].minor.yy0, yymsp[-3].minor.yy0); }
#line 987 "flow-parser.c"
        break;
      case 40: /* flda ::= fldr */
#line 106 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[0].minor.yy0); }
#line 992 "flow-parser.c"
        break;
      case 43: /* fldx ::= fldn dtid */
#line 110 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(ast->nprepend(yymsp[0].minor.yy0, yymsp[-1].minor.yy0), FTK_fldx); }
#line 997 "flow-parser.c"
        break;
      case 45: /* dtid ::= ID */
#line 114 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_dtid, yymsp[0].minor.yy0); }
#line 1002 "flow-parser.c"
        break;
      case 49: /* bexp ::= vall */
      case 50: /* bexp ::= fldx */ yytestcase(yyruleno==50);
#line 121 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_bexp, yymsp[0].minor.yy0); }
#line 1008 "flow-parser.c"
        break;
      case 51: /* bexp ::= HASH fldx */
      case 53: /* bexp ::= BANG bexp */ yytestcase(yyruleno==53);
#line 123 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_bexp, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1014 "flow-parser.c"
        break;
      case 54: /* bexp ::= bexp OR bexp */
      case 55: /* bexp ::= bexp AND bexp */ yytestcase(yyruleno==55);
      case 56: /* bexp ::= bexp EQ bexp */ yytestcase(yyruleno==56);
      case 57: /* bexp ::= bexp NE bexp */ yytestcase(yyruleno==57);
      case 58: /* bexp ::= bexp GT bexp */ yytestcase(yyruleno==58);
      case 59: /* bexp ::= bexp LT bexp */ yytestcase(yyruleno==59);
      case 60: /* bexp ::= bexp LE bexp */ yytestcase(yyruleno==60);
      case 61: /* bexp ::= bexp GE bexp */ yytestcase(yyruleno==61);
#line 126 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_bexp, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1026 "flow-parser.c"
        break;
      default:
      /* (47) eqc ::= EQUALS */ yytestcase(yyruleno==47);
      /* (48) eqc ::= COLON */ yytestcase(yyruleno==48);
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  flow_parserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
#line 41 "flow-parser.y"

    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
#line 1079 "flow-parser.c"
  flow_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  flow_parserARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 35 "flow-parser.y"

    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
#line 1097 "flow-parser.c"
  flow_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  flow_parserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
#line 38 "flow-parser.y"

    //std::cerr << "parsed just fine, ast size: " << ast->store.size() << " root: " << ast->store.back().children[0] << "\n";
#line 1119 "flow-parser.c"
  flow_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "flow_parserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void flow_parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  flow_parserTOKENTYPE yyminor       /* The value for the token */
  flow_parserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  flow_parserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
