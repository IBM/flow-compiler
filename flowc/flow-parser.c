/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 28 "flow-parser.y"

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
#define YYNOCODE 79
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
#define YYNSTATE 136
#define YYNRULE 69
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
 /*     0 */    11,   12,   14,   87,   27,   35,    7,   10,   18,   23,
 /*    10 */    16,   17,    8,  128,   89,    4,   19,   22,   21,    9,
 /*    20 */    69,   11,   12,   14,  128,   81,  110,    7,   10,   18,
 /*    30 */    23,   16,   17,    8,   40,  128,    4,   19,   22,   21,
 /*    40 */     9,    6,   11,   12,   14,  111,   79,   45,    7,   10,
 /*    50 */    18,   23,   16,   17,    8,   44,  128,    4,   19,   22,
 /*    60 */    21,    9,    5,   11,   12,   14,   22,   21,    9,    7,
 /*    70 */    10,   18,   23,   16,   17,    8,   24,  116,    4,   19,
 /*    80 */    22,   21,    9,   14,  102,   39,   82,    7,   10,   18,
 /*    90 */    23,   16,   17,    8,  105,  106,    4,   19,   22,   21,
 /*   100 */     9,   74,  129,  133,  132,    7,   10,   18,   23,   16,
 /*   110 */    17,    8,   67,  101,    4,   19,   22,   21,    9,   74,
 /*   120 */   129,  133,  132,   18,   23,   16,   17,    8,  103,   95,
 /*   130 */     4,   19,   22,   21,    9,    4,   19,   22,   21,    9,
 /*   140 */   100,   33,   34,  133,  132,   80,   15,   72,   77,   31,
 /*   150 */   135,    3,  112,   74,  129,  133,  132,   66,  101,   33,
 /*   160 */    34,  206,   41,   86,   15,   72,   77,   31,  115,    3,
 /*   170 */    43,   38,  129,  133,  132,   20,   98,   29,   71,   46,
 /*   180 */    13,   33,   34,   80,   24,  134,  128,  107,   68,  101,
 /*   190 */   108,   44,   43,   33,   34,  136,   25,   44,   15,   72,
 /*   200 */    77,   31,  125,    1,  130,   42,  129,  133,  132,  113,
 /*   210 */    80,   33,   34,   30,  117,   25,   15,   72,   77,   31,
 /*   220 */    97,    3,   88,  120,  105,  106,   94,  109,   28,   26,
 /*   230 */   117,  129,  133,  132,   84,  131,   65,  126,  124,   92,
 /*   240 */    90,  122,   78,   85,  128,   33,   34,   73,   98,   91,
 /*   250 */    71,   49,  114,  105,  106,   32,  104,   29,  128,  107,
 /*   260 */    93,   36,  108,   64,   43,   70,   75,  105,  106,   29,
 /*   270 */    33,   34,   76,   98,   37,   71,   51,  104,    2,  105,
 /*   280 */   106,   83,  127,  128,  107,   96,  118,  108,  207,   43,
 /*   290 */   207,   98,    8,   71,   63,    4,   19,   22,   21,    9,
 /*   300 */   207,  128,  107,  207,  207,  108,  207,   43,   98,  207,
 /*   310 */    71,   53,  207,  129,  133,  132,  207,  207,  128,  107,
 /*   320 */   207,  207,  108,  207,   43,   98,  207,   71,   48,  207,
 /*   330 */   207,  207,  207,  207,  207,  128,  107,  207,  207,  108,
 /*   340 */   207,   43,  207,  207,  207,   98,  207,   71,   47,  207,
 /*   350 */   207,  207,   33,   34,  207,  128,  107,  207,  207,  108,
 /*   360 */   207,   43,  207,   98,  207,   71,  121,  207,  207,  207,
 /*   370 */   207,  207,  207,  128,  107,  207,  207,  108,  207,   43,
 /*   380 */    98,  207,   71,   55,  207,  207,  207,  207,  207,  207,
 /*   390 */   128,  107,  207,  207,  108,  207,   43,   98,  207,   71,
 /*   400 */   123,  207,  207,  207,  207,  207,  207,  128,  107,  207,
 /*   410 */   207,  108,  207,   43,  207,  207,  207,   98,  207,   71,
 /*   420 */    99,  207,  207,  207,  207,  207,  207,  128,  107,  207,
 /*   430 */   207,  108,  207,   43,  207,   98,  207,   71,   50,  207,
 /*   440 */   207,  207,  207,  207,  207,  128,  107,  207,  207,  108,
 /*   450 */   207,   43,   98,  207,   71,   62,  207,  207,  207,  207,
 /*   460 */   207,  207,  128,  107,  207,  207,  108,  207,   43,   98,
 /*   470 */   207,   71,  119,  207,  207,  207,  207,  207,  207,  128,
 /*   480 */   107,  207,  207,  108,  207,   43,  207,  207,  207,   98,
 /*   490 */   207,   71,   57,  207,  207,  207,  207,  207,  207,  128,
 /*   500 */   107,  207,  207,  108,  207,   43,  207,   98,  207,   71,
 /*   510 */    60,  207,  207,  207,  207,  207,  207,  128,  107,  207,
 /*   520 */   207,  108,  207,   43,   98,  207,   71,   52,  207,  207,
 /*   530 */   207,  207,  207,  207,  128,  107,  207,  207,  108,  207,
 /*   540 */    43,   98,  207,   71,   61,  207,  207,  207,  207,  207,
 /*   550 */   207,  128,  107,  207,  207,  108,  207,   43,  207,  207,
 /*   560 */   207,   98,  207,   71,   46,  207,  207,  207,  207,  207,
 /*   570 */   207,  128,  107,  207,  207,  108,  207,   43,  207,   98,
 /*   580 */   207,   71,   56,  207,  207,  207,  207,  207,  207,  128,
 /*   590 */   107,  207,  207,  108,  207,   43,   98,  207,   71,   59,
 /*   600 */   207,  207,  207,  207,  207,  207,  128,  107,  207,  207,
 /*   610 */   108,  207,   43,   98,  207,   71,   54,  207,  207,  207,
 /*   620 */   207,  207,  207,  128,  107,  207,  207,  108,  207,   43,
 /*   630 */   207,  207,  207,   98,  207,   71,   58,  207,  207,  207,
 /*   640 */   207,  207,  207,  128,  107,  207,  207,  108,  207,   43,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    26,   27,   28,   61,   62,   63,   32,   33,   34,   35,
 /*    10 */    36,   37,   38,   71,   61,   41,   42,   43,   44,   45,
 /*    20 */    60,   26,   27,   28,   71,   61,   52,   32,   33,   34,
 /*    30 */    35,   36,   37,   38,    1,   71,   41,   42,   43,   44,
 /*    40 */    45,   25,   26,   27,   28,   20,   61,   52,   32,   33,
 /*    50 */    34,   35,   36,   37,   38,   23,   71,   41,   42,   43,
 /*    60 */    44,   45,   62,   26,   27,   28,   43,   44,   45,   32,
 /*    70 */    33,   34,   35,   36,   37,   38,    1,   20,   41,   42,
 /*    80 */    43,   44,   45,   28,   52,   66,   67,   32,   33,   34,
 /*    90 */    35,   36,   37,   38,   24,   25,   41,   42,   43,   44,
 /*   100 */    45,    1,    2,    3,    4,   32,   33,   34,   35,   36,
 /*   110 */    37,   38,   73,   74,   41,   42,   43,   44,   45,    1,
 /*   120 */     2,    3,    4,   34,   35,   36,   37,   38,   65,   54,
 /*   130 */    41,   42,   43,   44,   45,   41,   42,   43,   44,   45,
 /*   140 */     1,   41,   42,    3,    4,   21,   46,   47,   48,   49,
 /*   150 */    65,   51,   52,    1,    2,    3,    4,   73,   74,   41,
 /*   160 */    42,   58,   59,   60,   46,   47,   48,   49,   75,   51,
 /*   170 */    77,    1,    2,    3,    4,   51,   61,   53,   63,   64,
 /*   180 */    23,   41,   42,   21,    1,   20,   71,   72,   73,   74,
 /*   190 */    75,   23,   77,   41,   42,    0,    1,   23,   46,   47,
 /*   200 */    48,   49,   20,   51,   71,    1,    2,    3,    4,   52,
 /*   210 */    21,   41,   42,   51,    1,    1,   46,   47,   48,   49,
 /*   220 */    52,   51,   22,   20,   24,   25,   52,   20,   24,   25,
 /*   230 */     1,    2,    3,    4,   61,   71,   63,   54,   65,   20,
 /*   240 */    20,   68,   69,   70,   71,   41,   42,   67,   61,   52,
 /*   250 */    63,   64,   20,   24,   25,   51,   22,   53,   71,   72,
 /*   260 */    52,    1,   75,   76,   77,   22,    1,   24,   25,   53,
 /*   270 */    41,   42,    1,   61,    1,   63,   64,   22,   51,   24,
 /*   280 */    25,   63,   65,   71,   72,   74,    1,   75,   78,   77,
 /*   290 */    78,   61,   38,   63,   64,   41,   42,   43,   44,   45,
 /*   300 */    78,   71,   72,   78,   78,   75,   78,   77,   61,   78,
 /*   310 */    63,   64,   78,    2,    3,    4,   78,   78,   71,   72,
 /*   320 */    78,   78,   75,   78,   77,   61,   78,   63,   64,   78,
 /*   330 */    78,   78,   78,   78,   78,   71,   72,   78,   78,   75,
 /*   340 */    78,   77,   78,   78,   78,   61,   78,   63,   64,   78,
 /*   350 */    78,   78,   41,   42,   78,   71,   72,   78,   78,   75,
 /*   360 */    78,   77,   78,   61,   78,   63,   64,   78,   78,   78,
 /*   370 */    78,   78,   78,   71,   72,   78,   78,   75,   78,   77,
 /*   380 */    61,   78,   63,   64,   78,   78,   78,   78,   78,   78,
 /*   390 */    71,   72,   78,   78,   75,   78,   77,   61,   78,   63,
 /*   400 */    64,   78,   78,   78,   78,   78,   78,   71,   72,   78,
 /*   410 */    78,   75,   78,   77,   78,   78,   78,   61,   78,   63,
 /*   420 */    64,   78,   78,   78,   78,   78,   78,   71,   72,   78,
 /*   430 */    78,   75,   78,   77,   78,   61,   78,   63,   64,   78,
 /*   440 */    78,   78,   78,   78,   78,   71,   72,   78,   78,   75,
 /*   450 */    78,   77,   61,   78,   63,   64,   78,   78,   78,   78,
 /*   460 */    78,   78,   71,   72,   78,   78,   75,   78,   77,   61,
 /*   470 */    78,   63,   64,   78,   78,   78,   78,   78,   78,   71,
 /*   480 */    72,   78,   78,   75,   78,   77,   78,   78,   78,   61,
 /*   490 */    78,   63,   64,   78,   78,   78,   78,   78,   78,   71,
 /*   500 */    72,   78,   78,   75,   78,   77,   78,   61,   78,   63,
 /*   510 */    64,   78,   78,   78,   78,   78,   78,   71,   72,   78,
 /*   520 */    78,   75,   78,   77,   61,   78,   63,   64,   78,   78,
 /*   530 */    78,   78,   78,   78,   71,   72,   78,   78,   75,   78,
 /*   540 */    77,   61,   78,   63,   64,   78,   78,   78,   78,   78,
 /*   550 */    78,   71,   72,   78,   78,   75,   78,   77,   78,   78,
 /*   560 */    78,   61,   78,   63,   64,   78,   78,   78,   78,   78,
 /*   570 */    78,   71,   72,   78,   78,   75,   78,   77,   78,   61,
 /*   580 */    78,   63,   64,   78,   78,   78,   78,   78,   78,   71,
 /*   590 */    72,   78,   78,   75,   78,   77,   61,   78,   63,   64,
 /*   600 */    78,   78,   78,   78,   78,   78,   71,   72,   78,   78,
 /*   610 */    75,   78,   77,   61,   78,   63,   64,   78,   78,   78,
 /*   620 */    78,   78,   78,   71,   72,   78,   78,   75,   78,   77,
 /*   630 */    78,   78,   78,   61,   78,   63,   64,   78,   78,   78,
 /*   640 */    78,   78,   78,   71,   72,   78,   78,   75,   78,   77,
};
#define YY_SHIFT_USE_DFLT (-27)
#define YY_SHIFT_MAX 89
static const short yy_shift_ofst[] = {
 /*     0 */   214,  170,  100,  118,  118,  152,  118,  118,  118,  118,
 /*    10 */   118,  118,  118,  118,  118,  118,  118,  118,  118,  118,
 /*    20 */   118,  118,  118,  118,  204,  229,  311,  311,  311,   75,
 /*    30 */   273,  271,  260,  140,  140,  124,  200,  243,  255,  183,
 /*    40 */    70,  195,  216,  213,   33,  216,  -26,   16,   -5,   37,
 /*    50 */    37,   37,   37,   55,   73,   89,   89,  254,  254,  254,
 /*    60 */   254,   94,   23,   23,  157,  162,  168,  174,   32,  219,
 /*    70 */   208,  189,  265,  182,  234,  227,  234,  285,   25,   57,
 /*    80 */   139,  165,  182,  189,  203,  207,  219,  220,  197,  232,
};
#define YY_REDUCE_USE_DFLT (-59)
#define YY_REDUCE_MAX 45
static const short yy_reduce_ofst[] = {
 /*     0 */   103,  115,  187,  500,  391,  463,  374,  518,  480,  356,
 /*    10 */   319,  284,  247,  212,  552,  408,  428,  446,  572,  230,
 /*    20 */   264,  302,  336,  535,  173,  -58,  -47,  -36,  -15,   19,
 /*    30 */    39,   93,   84,  164,  133,   85,    0,    0,    0,  180,
 /*    40 */     0,  -40,   63,  218,  211,  217,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
 /*    10 */   205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
 /*    20 */   205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
 /*    30 */   205,  205,  205,  205,  205,  205,  205,  205,  201,  205,
 /*    40 */   205,  205,  201,  198,  205,  205,  205,  205,  205,  196,
 /*    50 */   195,  197,  172,  194,  193,  187,  188,  192,  190,  189,
 /*    60 */   191,  186,  181,  182,  205,  205,  205,  205,  205,  138,
 /*    70 */   205,  164,  205,  147,  201,  205,  205,  205,  205,  205,
 /*    80 */   205,  205,  146,  199,  205,  205,  137,  205,  205,  205,
 /*    90 */   139,  166,  143,  167,  168,  145,  170,  165,  163,  185,
 /*   100 */   202,  169,  171,  156,  200,  203,  204,  173,  174,  154,
 /*   110 */   175,  153,  176,  177,  152,  178,  151,  201,  179,  180,
 /*   120 */   150,  183,  149,  184,  148,  155,  144,  141,  162,  161,
 /*   130 */   160,  159,  158,  157,  140,  142,
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
  "FLOAT",         "SYMBOL",        "NODE",          "CONTAINER",   
  "ENTRY",         "IMPORT",        "DEFINE",        "OUTPUT",      
  "RETURN",        "ERROR",         "ENDPOINT",      "IMAGE",       
  "MOUNT",         "ENVIRONMENT",   "INPUT",         "HEADERS",     
  "SEMICOLON",     "DOT",           "AT",            "COMMA",       
  "EQUALS",        "COLON",         "QUESTION",      "OR",          
  "AND",           "BAR",           "CARET",         "AMP",         
  "NE",            "EQ",            "GT",            "LT",          
  "GE",            "LE",            "COMP",          "SHR",         
  "SHL",           "PLUS",          "MINUS",         "STAR",        
  "SLASH",         "PERCENT",       "BANG",          "TILDA",       
  "DOLLAR",        "HASH",          "UMINUS",        "OPENPAR",     
  "CLOSEPAR",      "OPENBRA",       "CLOSEBRA",      "OPENSQB",     
  "CLOSESQB",      "error",         "main",          "flow",        
  "stmt",          "valx",          "eqsc",          "dtid",        
  "fldr",          "blck",          "list",          "elem",        
  "lblk",          "oexp",          "rexp",          "valn",        
  "vall",          "fldm",          "fldd",          "fldx",        
  "fldra",         "fldn",        
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= flow",
 /*   1 */ "flow ::= stmt",
 /*   2 */ "flow ::= flow stmt",
 /*   3 */ "stmt ::= ID valx SEMICOLON",
 /*   4 */ "stmt ::= ID eqsc valx SEMICOLON",
 /*   5 */ "stmt ::= ID dtid OPENPAR fldr CLOSEPAR blck",
 /*   6 */ "stmt ::= ID dtid blck",
 /*   7 */ "stmt ::= stmt SEMICOLON",
 /*   8 */ "blck ::= OPENBRA list CLOSEBRA",
 /*   9 */ "blck ::= OPENBRA CLOSEBRA",
 /*  10 */ "list ::= elem",
 /*  11 */ "list ::= list elem",
 /*  12 */ "elem ::= ID blck",
 /*  13 */ "elem ::= ID lblk",
 /*  14 */ "elem ::= ID valx SEMICOLON",
 /*  15 */ "elem ::= ID EQUALS valx SEMICOLON",
 /*  16 */ "elem ::= ID COLON valx SEMICOLON",
 /*  17 */ "elem ::= ID oexp SEMICOLON",
 /*  18 */ "elem ::= ID rexp SEMICOLON",
 /*  19 */ "elem ::= elem SEMICOLON",
 /*  20 */ "lblk ::= ID blck",
 /*  21 */ "valn ::= INTEGER",
 /*  22 */ "valn ::= FLOAT",
 /*  23 */ "valn ::= PLUS valn",
 /*  24 */ "valn ::= MINUS valn",
 /*  25 */ "valx ::= STRING",
 /*  26 */ "valx ::= valn",
 /*  27 */ "vall ::= valx",
 /*  28 */ "vall ::= dtid",
 /*  29 */ "rexp ::= OPENPAR fldm CLOSEPAR",
 /*  30 */ "rexp ::= OPENPAR ID AT CLOSEPAR",
 /*  31 */ "oexp ::= dtid OPENPAR ID AT CLOSEPAR",
 /*  32 */ "oexp ::= dtid OPENPAR fldm CLOSEPAR",
 /*  33 */ "fldm ::= fldd",
 /*  34 */ "fldm ::= fldm COMMA fldd",
 /*  35 */ "fldd ::= ID eqsc OPENPAR fldm CLOSEPAR",
 /*  36 */ "fldd ::= ID eqsc fldr",
 /*  37 */ "fldr ::= vall",
 /*  38 */ "fldr ::= fldx",
 /*  39 */ "fldr ::= OPENPAR fldr CLOSEPAR",
 /*  40 */ "fldr ::= TILDA ID OPENPAR CLOSEPAR",
 /*  41 */ "fldr ::= TILDA ID OPENPAR fldra CLOSEPAR",
 /*  42 */ "fldr ::= HASH fldx",
 /*  43 */ "fldr ::= DOLLAR ID",
 /*  44 */ "fldr ::= BANG fldr",
 /*  45 */ "fldr ::= fldr PLUS fldr",
 /*  46 */ "fldr ::= fldr MINUS fldr",
 /*  47 */ "fldr ::= fldr SLASH fldr",
 /*  48 */ "fldr ::= fldr STAR fldr",
 /*  49 */ "fldr ::= fldr PERCENT fldr",
 /*  50 */ "fldr ::= fldr COMP fldr",
 /*  51 */ "fldr ::= fldr EQ fldr",
 /*  52 */ "fldr ::= fldr NE fldr",
 /*  53 */ "fldr ::= fldr LT fldr",
 /*  54 */ "fldr ::= fldr GT fldr",
 /*  55 */ "fldr ::= fldr LE fldr",
 /*  56 */ "fldr ::= fldr GE fldr",
 /*  57 */ "fldr ::= fldr AND fldr",
 /*  58 */ "fldr ::= fldr OR fldr",
 /*  59 */ "fldr ::= fldr QUESTION fldr COLON fldr",
 /*  60 */ "fldra ::= fldr",
 /*  61 */ "fldra ::= fldra COMMA fldr",
 /*  62 */ "fldx ::= fldn",
 /*  63 */ "fldx ::= fldn dtid",
 /*  64 */ "fldn ::= ID AT",
 /*  65 */ "dtid ::= ID",
 /*  66 */ "dtid ::= dtid DOT ID",
 /*  67 */ "eqsc ::= EQUALS",
 /*  68 */ "eqsc ::= COLON",
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
  { 58, 1 },
  { 59, 1 },
  { 59, 2 },
  { 60, 3 },
  { 60, 4 },
  { 60, 6 },
  { 60, 3 },
  { 60, 2 },
  { 65, 3 },
  { 65, 2 },
  { 66, 1 },
  { 66, 2 },
  { 67, 2 },
  { 67, 2 },
  { 67, 3 },
  { 67, 4 },
  { 67, 4 },
  { 67, 3 },
  { 67, 3 },
  { 67, 2 },
  { 68, 2 },
  { 71, 1 },
  { 71, 1 },
  { 71, 2 },
  { 71, 2 },
  { 61, 1 },
  { 61, 1 },
  { 72, 1 },
  { 72, 1 },
  { 70, 3 },
  { 70, 4 },
  { 69, 5 },
  { 69, 4 },
  { 73, 1 },
  { 73, 3 },
  { 74, 5 },
  { 74, 3 },
  { 64, 1 },
  { 64, 1 },
  { 64, 3 },
  { 64, 4 },
  { 64, 5 },
  { 64, 2 },
  { 64, 2 },
  { 64, 2 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 5 },
  { 76, 1 },
  { 76, 3 },
  { 75, 1 },
  { 75, 2 },
  { 77, 2 },
  { 63, 1 },
  { 63, 3 },
  { 62, 1 },
  { 62, 1 },
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
#line 46 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_ACCEPT, yymsp[0].minor.yy0); }
#line 938 "flow-parser.c"
        break;
      case 1: /* flow ::= stmt */
#line 48 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_flow, yymsp[0].minor.yy0); }
#line 943 "flow-parser.c"
        break;
      case 2: /* flow ::= flow stmt */
      case 11: /* list ::= list elem */ yytestcase(yyruleno==11);
#line 49 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 949 "flow-parser.c"
        break;
      case 3: /* stmt ::= ID valx SEMICOLON */
#line 51 "flow-parser.y"
{ yygotominor.yy0 = ast->stmt_keyw(yymsp[-2].minor.yy0) == FTK_IMPORT? ast->node(FTK_IMPORT, yymsp[-1].minor.yy0): ast->node(ast->stmt_keyw(yymsp[-2].minor.yy0), yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); 
                                                                 ast->expect(yygotominor.yy0, {FTK_DEFINE, FTK_IMPORT}, "import directive or variable definition expected"); }
#line 955 "flow-parser.c"
        break;
      case 4: /* stmt ::= ID eqsc valx SEMICOLON */
#line 53 "flow-parser.y"
{ yygotominor.yy0 = ast->node(ast->stmt_keyw(yymsp[-3].minor.yy0), yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); ast->expect(yygotominor.yy0, FTK_DEFINE, "keyword used as variable name"); }
#line 960 "flow-parser.c"
        break;
      case 5: /* stmt ::= ID dtid OPENPAR fldr CLOSEPAR blck */
#line 54 "flow-parser.y"
{ yygotominor.yy0 = ast->node(ast->stmt_keyw(yymsp[-5].minor.yy0), yymsp[-5].minor.yy0, yymsp[-4].minor.yy0, yymsp[0].minor.yy0, yymsp[-2].minor.yy0); ast->expect(yygotominor.yy0, FTK_NODE, "expected \"node\" keyword"); }
#line 965 "flow-parser.c"
        break;
      case 6: /* stmt ::= ID dtid blck */
#line 55 "flow-parser.y"
{ yygotominor.yy0 = ast->node(ast->stmt_keyw(yymsp[-2].minor.yy0), yymsp[-2].minor.yy0, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); 
                                                                 ast->expect(yygotominor.yy0, {FTK_CONTAINER, FTK_NODE, FTK_ENTRY}, "expected \"node\", \"entry\" or \"container\" here"); }
#line 971 "flow-parser.c"
        break;
      case 7: /* stmt ::= stmt SEMICOLON */
      case 8: /* blck ::= OPENBRA list CLOSEBRA */ yytestcase(yyruleno==8);
      case 19: /* elem ::= elem SEMICOLON */ yytestcase(yyruleno==19);
      case 39: /* fldr ::= OPENPAR fldr CLOSEPAR */ yytestcase(yyruleno==39);
      case 64: /* fldn ::= ID AT */ yytestcase(yyruleno==64);
#line 57 "flow-parser.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 980 "flow-parser.c"
        break;
      case 9: /* blck ::= OPENBRA CLOSEBRA */
#line 60 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(yymsp[-1].minor.yy0, FTK_blck); }
#line 985 "flow-parser.c"
        break;
      case 10: /* list ::= elem */
#line 64 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_blck, yymsp[0].minor.yy0); }
#line 990 "flow-parser.c"
        break;
      case 12: /* elem ::= ID blck */
      case 13: /* elem ::= ID lblk */ yytestcase(yyruleno==13);
#line 70 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 996 "flow-parser.c"
        break;
      case 14: /* elem ::= ID valx SEMICOLON */
      case 17: /* elem ::= ID oexp SEMICOLON */ yytestcase(yyruleno==17);
      case 18: /* elem ::= ID rexp SEMICOLON */ yytestcase(yyruleno==18);
#line 72 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); }
#line 1003 "flow-parser.c"
        break;
      case 15: /* elem ::= ID EQUALS valx SEMICOLON */
      case 16: /* elem ::= ID COLON valx SEMICOLON */ yytestcase(yyruleno==16);
#line 73 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 1009 "flow-parser.c"
        break;
      case 20: /* lblk ::= ID blck */
#line 79 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_lblk, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1014 "flow-parser.c"
        break;
      case 21: /* valn ::= INTEGER */
      case 22: /* valn ::= FLOAT */ yytestcase(yyruleno==22);
      case 23: /* valn ::= PLUS valn */ yytestcase(yyruleno==23);
      case 25: /* valx ::= STRING */ yytestcase(yyruleno==25);
      case 26: /* valx ::= valn */ yytestcase(yyruleno==26);
      case 27: /* vall ::= valx */ yytestcase(yyruleno==27);
      case 37: /* fldr ::= vall */ yytestcase(yyruleno==37);
      case 38: /* fldr ::= fldx */ yytestcase(yyruleno==38);
      case 62: /* fldx ::= fldn */ yytestcase(yyruleno==62);
#line 83 "flow-parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 1027 "flow-parser.c"
        break;
      case 24: /* valn ::= MINUS valn */
#line 86 "flow-parser.y"
{ yygotominor.yy0 = ast->negate(yymsp[0].minor.yy0); }
#line 1032 "flow-parser.c"
        break;
      case 28: /* vall ::= dtid */
#line 96 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(yymsp[0].minor.yy0, FTK_enum); }
#line 1037 "flow-parser.c"
        break;
      case 29: /* rexp ::= OPENPAR fldm CLOSEPAR */
#line 100 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_RETURN, yymsp[-1].minor.yy0); }
#line 1042 "flow-parser.c"
        break;
      case 30: /* rexp ::= OPENPAR ID AT CLOSEPAR */
#line 101 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_RETURN, ast->node(FTK_fldx, yymsp[-2].minor.yy0)); }
#line 1047 "flow-parser.c"
        break;
      case 31: /* oexp ::= dtid OPENPAR ID AT CLOSEPAR */
#line 105 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_OUTPUT, yymsp[-4].minor.yy0, ast->node(FTK_fldx, yymsp[-2].minor.yy0)); }
#line 1052 "flow-parser.c"
        break;
      case 32: /* oexp ::= dtid OPENPAR fldm CLOSEPAR */
#line 106 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_OUTPUT, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 1057 "flow-parser.c"
        break;
      case 33: /* fldm ::= fldd */
#line 108 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldm, yymsp[0].minor.yy0); }
#line 1062 "flow-parser.c"
        break;
      case 34: /* fldm ::= fldm COMMA fldd */
      case 61: /* fldra ::= fldra COMMA fldr */ yytestcase(yyruleno==61);
      case 66: /* dtid ::= dtid DOT ID */ yytestcase(yyruleno==66);
#line 109 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 1069 "flow-parser.c"
        break;
      case 35: /* fldd ::= ID eqsc OPENPAR fldm CLOSEPAR */
#line 113 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-4].minor.yy0, yymsp[-1].minor.yy0); ast->chinteger(yygotominor.yy0, ast->at(yymsp[-3].minor.yy0).token.integer_value); }
#line 1074 "flow-parser.c"
        break;
      case 36: /* fldd ::= ID eqsc fldr */
#line 114 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); ast->chinteger(yygotominor.yy0, ast->at(yymsp[-1].minor.yy0).token.integer_value); }
#line 1079 "flow-parser.c"
        break;
      case 40: /* fldr ::= TILDA ID OPENPAR CLOSEPAR */
#line 121 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-2].minor.yy0); }
#line 1084 "flow-parser.c"
        break;
      case 41: /* fldr ::= TILDA ID OPENPAR fldra CLOSEPAR */
#line 122 "flow-parser.y"
{ yygotominor.yy0 = ast->nprepend(yymsp[-1].minor.yy0, yymsp[-3].minor.yy0); }
#line 1089 "flow-parser.c"
        break;
      case 42: /* fldr ::= HASH fldx */
      case 43: /* fldr ::= DOLLAR ID */ yytestcase(yyruleno==43);
      case 44: /* fldr ::= BANG fldr */ yytestcase(yyruleno==44);
#line 123 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1096 "flow-parser.c"
        break;
      case 45: /* fldr ::= fldr PLUS fldr */
      case 46: /* fldr ::= fldr MINUS fldr */ yytestcase(yyruleno==46);
      case 47: /* fldr ::= fldr SLASH fldr */ yytestcase(yyruleno==47);
      case 48: /* fldr ::= fldr STAR fldr */ yytestcase(yyruleno==48);
      case 49: /* fldr ::= fldr PERCENT fldr */ yytestcase(yyruleno==49);
      case 50: /* fldr ::= fldr COMP fldr */ yytestcase(yyruleno==50);
      case 51: /* fldr ::= fldr EQ fldr */ yytestcase(yyruleno==51);
      case 52: /* fldr ::= fldr NE fldr */ yytestcase(yyruleno==52);
      case 53: /* fldr ::= fldr LT fldr */ yytestcase(yyruleno==53);
      case 54: /* fldr ::= fldr GT fldr */ yytestcase(yyruleno==54);
      case 55: /* fldr ::= fldr LE fldr */ yytestcase(yyruleno==55);
      case 56: /* fldr ::= fldr GE fldr */ yytestcase(yyruleno==56);
      case 57: /* fldr ::= fldr AND fldr */ yytestcase(yyruleno==57);
      case 58: /* fldr ::= fldr OR fldr */ yytestcase(yyruleno==58);
#line 126 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-1].minor.yy0, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 1114 "flow-parser.c"
        break;
      case 59: /* fldr ::= fldr QUESTION fldr COLON fldr */
#line 140 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-3].minor.yy0, yymsp[-4].minor.yy0, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 1119 "flow-parser.c"
        break;
      case 60: /* fldra ::= fldr */
#line 142 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[0].minor.yy0); }
#line 1124 "flow-parser.c"
        break;
      case 63: /* fldx ::= fldn dtid */
#line 148 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(ast->nprepend(yymsp[0].minor.yy0, yymsp[-1].minor.yy0), FTK_fldx); }
#line 1129 "flow-parser.c"
        break;
      case 65: /* dtid ::= ID */
#line 156 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_dtid, yymsp[0].minor.yy0); }
#line 1134 "flow-parser.c"
        break;
      case 67: /* eqsc ::= EQUALS */
      case 68: /* eqsc ::= COLON */ yytestcase(yyruleno==68);
#line 161 "flow-parser.y"
{ ast->chinteger(yygotominor.yy0, 0); }
#line 1140 "flow-parser.c"
        break;
      default:
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
#line 42 "flow-parser.y"

    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
#line 1191 "flow-parser.c"
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
#line 36 "flow-parser.y"

    ast->node(FTK_SYNTAX_ERROR, (int) ast->store.size());
#line 1209 "flow-parser.c"
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
#line 39 "flow-parser.y"

    //std::cerr << "parsed just fine, ast size: " << ast->store.size() << " root: " << ast->store.back().children[0] << "\n";
#line 1231 "flow-parser.c"
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
