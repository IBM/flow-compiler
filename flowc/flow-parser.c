/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 21 "flow-parser.y"

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
#define YYNOCODE 54
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
#define YYNSTATE 109
#define YYNRULE 59
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
 /*     0 */    13,   11,   43,   72,   53,    8,    9,    7,    6,    4,
 /*    10 */     5,   13,   11,   53,   32,   16,    8,    9,    7,    6,
 /*    20 */     4,    5,   12,   24,   18,   89,   28,   31,   94,   93,
 /*    30 */    91,   17,   57,   94,   93,   91,   66,   67,    8,    9,
 /*    40 */     7,    6,    4,    5,   61,  107,   52,   57,   94,   93,
 /*    50 */    91,   19,   77,   18,   10,   21,    3,   80,   65,  105,
 /*    60 */    42,   29,  169,   27,   44,   41,   49,   99,   15,    1,
 /*    70 */    97,   48,   22,   50,   51,   11,   14,   54,   47,    8,
 /*    80 */     9,    7,    6,    4,    5,  104,  108,    1,   82,   52,
 /*    90 */   100,   82,   52,   98,   56,   77,   62,   59,   77,   66,
 /*   100 */    67,   86,   83,   29,   29,   83,   25,   29,   82,   52,
 /*   110 */    38,   82,   52,   37,   79,   77,  107,   52,   77,   82,
 /*   120 */    52,   92,   83,   77,   29,   83,   77,   29,   30,  106,
 /*   130 */   105,   73,   29,   83,   58,   29,   66,   67,   81,   82,
 /*   140 */    52,   36,   82,   52,   34,   71,   77,   46,   23,   77,
 /*   150 */    82,   52,   35,   83,   77,   29,   83,   77,   29,   57,
 /*   160 */    94,   93,   91,   45,   83,   18,   29,   85,   57,   94,
 /*   170 */    93,   91,  107,   52,  108,   94,   93,   91,   30,   77,
 /*   180 */    82,   52,  102,   20,   87,   68,  105,   77,   29,   54,
 /*   190 */    82,   52,   33,  103,   83,   84,   29,   77,   54,   40,
 /*   200 */    72,   82,   52,   63,   83,   69,   29,   75,   77,    7,
 /*   210 */     6,    4,    5,   66,   67,   83,   30,   29,   94,   93,
 /*   220 */    91,   39,   72,  109,   16,  101,   26,   60,   96,   74,
 /*   230 */    95,   90,   88,   78,   53,   55,    2,   64,   70,   76,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    12,   13,   47,   48,    7,   17,   18,   19,   20,   21,
 /*    10 */    22,   12,   13,    7,   26,    1,   17,   18,   19,   20,
 /*    20 */    21,   22,   25,    1,   27,   26,    1,    1,    2,    3,
 /*    30 */     4,   25,    1,    2,    3,    4,   10,   11,   17,   18,
 /*    40 */    19,   20,   21,   22,    1,   36,   37,    1,    2,    3,
 /*    50 */     4,   25,   43,   27,   23,   24,   25,   39,   49,   50,
 /*    60 */    51,   52,   33,   34,   35,   37,   43,   39,    9,    1,
 /*    70 */    42,   43,   44,   45,   46,   13,   44,   31,   41,   17,
 /*    80 */    18,   19,   20,   21,   22,   26,    1,    1,   36,   37,
 /*    90 */    38,   36,   37,   38,   37,   43,   28,    8,   43,   10,
 /*   100 */    11,   50,   50,   52,   52,   50,    1,   52,   36,   37,
 /*   110 */    38,   36,   37,   38,   28,   43,   36,   37,   43,   36,
 /*   120 */    37,   38,   50,   43,   52,   50,   43,   52,    9,   49,
 /*   130 */    50,   26,   52,   50,    8,   52,   10,   11,   48,   36,
 /*   140 */    37,   38,   36,   37,   38,   26,   43,   36,   37,   43,
 /*   150 */    36,   37,   38,   50,   43,   52,   50,   43,   52,    1,
 /*   160 */     2,    3,    4,   35,   50,   27,   52,    6,    1,    2,
 /*   170 */     3,    4,   36,   37,    1,    2,    3,    4,    9,   43,
 /*   180 */    36,   37,   38,   25,   39,   49,   50,   43,   52,   31,
 /*   190 */    36,   37,   38,   26,   50,   26,   52,   43,   31,   47,
 /*   200 */    48,   36,   37,   38,   50,   39,   52,    6,   43,   19,
 /*   210 */    20,   21,   22,   10,   11,   50,    9,   52,    2,    3,
 /*   220 */     4,   47,   48,    0,    1,    6,   40,   41,    6,    1,
 /*   230 */     6,    6,    6,   26,    7,    1,   25,    8,   26,   26,
};
#define YY_SHIFT_USE_DFLT (-13)
#define YY_SHIFT_MAX 61
static const short yy_shift_ofst[] = {
 /*     0 */    14,   26,  167,   31,   31,   31,   31,   31,   31,   31,
 /*    10 */    31,   31,   31,   31,  158,   46,  173,  105,   86,   22,
 /*    20 */    25,   43,  216,   -3,  126,   89,   68,  223,  203,   85,
 /*    30 */    25,  138,  138,   -1,  -12,   62,   21,  190,  190,  169,
 /*    40 */   119,    6,   59,  207,  161,  161,  201,  219,  222,  224,
 /*    50 */   225,  226,  227,  228,  234,  211,  227,  229,  212,  213,
 /*    60 */   219,  229,
};
#define YY_REDUCE_USE_DFLT (-46)
#define YY_REDUCE_MAX 32
static const short yy_reduce_ofst[] = {
 /*     0 */    29,   28,    9,  154,  165,  144,   52,   55,   72,   75,
 /*    10 */    83,  103,  106,  114,  136,   80,  111,  -45,  186,  174,
 /*    20 */   152,   51,   23,   18,   32,   32,   37,  128,   32,   57,
 /*    30 */    90,  145,  166,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   168,  168,  168,  168,  168,  168,  168,  168,  168,  168,
 /*    10 */   168,  168,  168,  168,  168,  168,  168,  168,  168,  168,
 /*    20 */   168,  168,  168,  132,  168,  168,  168,  168,  168,  148,
 /*    30 */   168,  151,  168,  168,  168,  160,  161,  162,  163,  168,
 /*    40 */   168,  168,  168,  168,  110,  111,  168,  119,  168,  168,
 /*    50 */   168,  168,  132,  168,  168,  168,  149,  151,  168,  168,
 /*    60 */   118,  168,  116,  167,  150,  146,  153,  154,  141,  113,
 /*    70 */   134,  140,  138,  135,  152,  112,  136,  131,  137,  117,
 /*    80 */   114,  139,  155,  156,  133,  115,  157,  127,  125,  158,
 /*    90 */   124,  130,  159,  129,  128,  123,  122,  121,  164,  120,
 /*   100 */   165,  126,  166,  144,  145,  143,  147,  142,  151,
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
  "OR",            "AND",           "BAR",           "CARET",       
  "AMP",           "NE",            "EQ",            "GT",          
  "LT",            "GE",            "LE",            "BANG",        
  "HASH",          "OPENPAR",       "CLOSEPAR",      "OPENBRA",     
  "CLOSEBRA",      "OPENSQB",       "CLOSESQB",      "TILDA",       
  "error",         "main",          "flow",          "stmt",        
  "vali",          "dtid",          "bexp",          "blck",        
  "list",          "elem",          "lblk",          "valx",        
  "eqc",           "oexp",          "rexp",          "fldm",        
  "fldd",          "fldr",          "fldx",          "flda",        
  "fldn",        
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= flow",
 /*   1 */ "flow ::= stmt",
 /*   2 */ "flow ::= flow stmt",
 /*   3 */ "stmt ::= ID vali SEMICOLON",
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
 /*  19 */ "valx ::= STRING",
 /*  20 */ "valx ::= INTEGER",
 /*  21 */ "valx ::= FLOAT",
 /*  22 */ "vali ::= valx",
 /*  23 */ "vali ::= dtid",
 /*  24 */ "rexp ::= OPENPAR fldm CLOSEPAR",
 /*  25 */ "rexp ::= OPENPAR ID AT CLOSEPAR",
 /*  26 */ "oexp ::= dtid OPENPAR CLOSEPAR",
 /*  27 */ "oexp ::= dtid OPENPAR ID AT CLOSEPAR",
 /*  28 */ "oexp ::= dtid OPENPAR fldm CLOSEPAR",
 /*  29 */ "fldm ::= fldd",
 /*  30 */ "fldm ::= fldm COMMA fldd",
 /*  31 */ "fldd ::= ID eqc OPENPAR fldm CLOSEPAR",
 /*  32 */ "fldd ::= ID eqc fldr",
 /*  33 */ "fldr ::= vali",
 /*  34 */ "fldr ::= fldx",
 /*  35 */ "fldr ::= TILDA ID OPENPAR CLOSEPAR",
 /*  36 */ "fldr ::= TILDA ID OPENPAR flda CLOSEPAR",
 /*  37 */ "flda ::= fldr",
 /*  38 */ "flda ::= flda COMMA fldr",
 /*  39 */ "fldx ::= fldn",
 /*  40 */ "fldx ::= fldn dtid",
 /*  41 */ "fldn ::= ID AT",
 /*  42 */ "dtid ::= ID",
 /*  43 */ "dtid ::= dtid DOT ID",
 /*  44 */ "eqc ::= EQUALS",
 /*  45 */ "eqc ::= COLON",
 /*  46 */ "bexp ::= vali",
 /*  47 */ "bexp ::= fldx",
 /*  48 */ "bexp ::= HASH fldx",
 /*  49 */ "bexp ::= OPENPAR bexp CLOSEPAR",
 /*  50 */ "bexp ::= BANG bexp",
 /*  51 */ "bexp ::= bexp OR bexp",
 /*  52 */ "bexp ::= bexp AND bexp",
 /*  53 */ "bexp ::= bexp EQ bexp",
 /*  54 */ "bexp ::= bexp NE bexp",
 /*  55 */ "bexp ::= bexp GT bexp",
 /*  56 */ "bexp ::= bexp LT bexp",
 /*  57 */ "bexp ::= bexp LE bexp",
 /*  58 */ "bexp ::= bexp GE bexp",
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
  { 33, 1 },
  { 34, 1 },
  { 34, 2 },
  { 35, 3 },
  { 35, 6 },
  { 35, 3 },
  { 35, 2 },
  { 39, 3 },
  { 39, 2 },
  { 40, 1 },
  { 40, 2 },
  { 41, 2 },
  { 41, 2 },
  { 41, 3 },
  { 41, 4 },
  { 41, 3 },
  { 41, 3 },
  { 41, 2 },
  { 42, 2 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 36, 1 },
  { 36, 1 },
  { 46, 3 },
  { 46, 4 },
  { 45, 3 },
  { 45, 5 },
  { 45, 4 },
  { 47, 1 },
  { 47, 3 },
  { 48, 5 },
  { 48, 3 },
  { 49, 1 },
  { 49, 1 },
  { 49, 4 },
  { 49, 5 },
  { 51, 1 },
  { 51, 3 },
  { 50, 1 },
  { 50, 2 },
  { 52, 2 },
  { 37, 1 },
  { 37, 3 },
  { 44, 1 },
  { 44, 1 },
  { 38, 1 },
  { 38, 1 },
  { 38, 2 },
  { 38, 3 },
  { 38, 2 },
  { 38, 3 },
  { 38, 3 },
  { 38, 3 },
  { 38, 3 },
  { 38, 3 },
  { 38, 3 },
  { 38, 3 },
  { 38, 3 },
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
#line 39 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_ACCEPT, yymsp[0].minor.yy0); }
#line 824 "flow-parser.c"
        break;
      case 1: /* flow ::= stmt */
#line 41 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_flow, yymsp[0].minor.yy0); }
#line 829 "flow-parser.c"
        break;
      case 2: /* flow ::= flow stmt */
      case 10: /* list ::= list elem */ yytestcase(yyruleno==10);
#line 42 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 835 "flow-parser.c"
        break;
      case 3: /* stmt ::= ID vali SEMICOLON */
#line 44 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_stmt, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); }
#line 840 "flow-parser.c"
        break;
      case 4: /* stmt ::= ID dtid OPENPAR bexp CLOSEPAR blck */
#line 45 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_stmt, yymsp[-5].minor.yy0, yymsp[-4].minor.yy0, yymsp[0].minor.yy0, yymsp[-2].minor.yy0); }
#line 845 "flow-parser.c"
        break;
      case 5: /* stmt ::= ID dtid blck */
#line 46 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_stmt, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 850 "flow-parser.c"
        break;
      case 6: /* stmt ::= stmt SEMICOLON */
      case 7: /* blck ::= OPENBRA list CLOSEBRA */ yytestcase(yyruleno==7);
      case 17: /* elem ::= elem SEMICOLON */ yytestcase(yyruleno==17);
      case 41: /* fldn ::= ID AT */ yytestcase(yyruleno==41);
      case 49: /* bexp ::= OPENPAR bexp CLOSEPAR */ yytestcase(yyruleno==49);
#line 47 "flow-parser.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 859 "flow-parser.c"
        break;
      case 8: /* blck ::= OPENBRA CLOSEBRA */
#line 50 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(yymsp[-1].minor.yy0, FTK_blck); }
#line 864 "flow-parser.c"
        break;
      case 9: /* list ::= elem */
#line 52 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_blck, yymsp[0].minor.yy0); }
#line 869 "flow-parser.c"
        break;
      case 11: /* elem ::= ID blck */
      case 12: /* elem ::= ID lblk */ yytestcase(yyruleno==12);
#line 55 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 875 "flow-parser.c"
        break;
      case 13: /* elem ::= ID valx SEMICOLON */
      case 15: /* elem ::= ID oexp SEMICOLON */ yytestcase(yyruleno==15);
      case 16: /* elem ::= ID rexp SEMICOLON */ yytestcase(yyruleno==16);
#line 57 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); }
#line 882 "flow-parser.c"
        break;
      case 14: /* elem ::= ID eqc valx SEMICOLON */
#line 58 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 887 "flow-parser.c"
        break;
      case 18: /* lblk ::= ID blck */
#line 63 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_lblk, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 892 "flow-parser.c"
        break;
      case 19: /* valx ::= STRING */
      case 20: /* valx ::= INTEGER */ yytestcase(yyruleno==20);
      case 21: /* valx ::= FLOAT */ yytestcase(yyruleno==21);
      case 22: /* vali ::= valx */ yytestcase(yyruleno==22);
      case 23: /* vali ::= dtid */ yytestcase(yyruleno==23);
      case 33: /* fldr ::= vali */ yytestcase(yyruleno==33);
      case 34: /* fldr ::= fldx */ yytestcase(yyruleno==34);
      case 39: /* fldx ::= fldn */ yytestcase(yyruleno==39);
#line 65 "flow-parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 904 "flow-parser.c"
        break;
      case 24: /* rexp ::= OPENPAR fldm CLOSEPAR */
#line 72 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_rexp, yymsp[-1].minor.yy0); }
#line 909 "flow-parser.c"
        break;
      case 25: /* rexp ::= OPENPAR ID AT CLOSEPAR */
#line 73 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_rexp, yymsp[-2].minor.yy0); }
#line 914 "flow-parser.c"
        break;
      case 26: /* oexp ::= dtid OPENPAR CLOSEPAR */
#line 74 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_oexp, yymsp[-2].minor.yy0); }
#line 919 "flow-parser.c"
        break;
      case 27: /* oexp ::= dtid OPENPAR ID AT CLOSEPAR */
#line 75 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_oexp, yymsp[-4].minor.yy0, yymsp[-2].minor.yy0); }
#line 924 "flow-parser.c"
        break;
      case 28: /* oexp ::= dtid OPENPAR fldm CLOSEPAR */
#line 76 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_oexp, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 929 "flow-parser.c"
        break;
      case 29: /* fldm ::= fldd */
#line 78 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldm, yymsp[0].minor.yy0); }
#line 934 "flow-parser.c"
        break;
      case 30: /* fldm ::= fldm COMMA fldd */
      case 38: /* flda ::= flda COMMA fldr */ yytestcase(yyruleno==38);
      case 43: /* dtid ::= dtid DOT ID */ yytestcase(yyruleno==43);
#line 79 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 941 "flow-parser.c"
        break;
      case 31: /* fldd ::= ID eqc OPENPAR fldm CLOSEPAR */
#line 81 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-4].minor.yy0, yymsp[-1].minor.yy0); }
#line 946 "flow-parser.c"
        break;
      case 32: /* fldd ::= ID eqc fldr */
#line 82 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 951 "flow-parser.c"
        break;
      case 35: /* fldr ::= TILDA ID OPENPAR CLOSEPAR */
#line 86 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-2].minor.yy0); }
#line 956 "flow-parser.c"
        break;
      case 36: /* fldr ::= TILDA ID OPENPAR flda CLOSEPAR */
#line 87 "flow-parser.y"
{ yygotominor.yy0 = ast->nprepend(yymsp[-1].minor.yy0, yymsp[-3].minor.yy0); }
#line 961 "flow-parser.c"
        break;
      case 37: /* flda ::= fldr */
#line 89 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[0].minor.yy0); }
#line 966 "flow-parser.c"
        break;
      case 40: /* fldx ::= fldn dtid */
#line 93 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(ast->nprepend(yymsp[0].minor.yy0, yymsp[-1].minor.yy0), FTK_fldx); }
#line 971 "flow-parser.c"
        break;
      case 42: /* dtid ::= ID */
#line 97 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_dtid, yymsp[0].minor.yy0); }
#line 976 "flow-parser.c"
        break;
      case 46: /* bexp ::= vali */
      case 47: /* bexp ::= fldx */ yytestcase(yyruleno==47);
#line 104 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_bexp, yymsp[0].minor.yy0); }
#line 982 "flow-parser.c"
        break;
      case 48: /* bexp ::= HASH fldx */
      case 50: /* bexp ::= BANG bexp */ yytestcase(yyruleno==50);
#line 106 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_bexp, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 988 "flow-parser.c"
        break;
      case 51: /* bexp ::= bexp OR bexp */
      case 52: /* bexp ::= bexp AND bexp */ yytestcase(yyruleno==52);
      case 53: /* bexp ::= bexp EQ bexp */ yytestcase(yyruleno==53);
      case 54: /* bexp ::= bexp NE bexp */ yytestcase(yyruleno==54);
      case 55: /* bexp ::= bexp GT bexp */ yytestcase(yyruleno==55);
      case 56: /* bexp ::= bexp LT bexp */ yytestcase(yyruleno==56);
      case 57: /* bexp ::= bexp LE bexp */ yytestcase(yyruleno==57);
      case 58: /* bexp ::= bexp GE bexp */ yytestcase(yyruleno==58);
#line 109 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_bexp, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1000 "flow-parser.c"
        break;
      default:
      /* (44) eqc ::= EQUALS */ yytestcase(yyruleno==44);
      /* (45) eqc ::= COLON */ yytestcase(yyruleno==45);
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
#line 35 "flow-parser.y"

    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
#line 1053 "flow-parser.c"
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
#line 29 "flow-parser.y"

    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
#line 1071 "flow-parser.c"
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
#line 32 "flow-parser.y"

    //std::cerr << "parsed just fine, ast size: " << ast->store.size() << " root: " << ast->store.back().children[0] << "\n";
#line 1093 "flow-parser.c"
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
