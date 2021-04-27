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
 /*     0 */     8,   10,   11,   13,   86,   26,   37,   19,   24,   17,
 /*    10 */    18,   15,   16,    7,  103,   29,    4,   14,   23,   20,
 /*    20 */     5,   10,   11,   13,  206,   39,   85,   19,   24,   17,
 /*    30 */    18,   15,   16,    7,   42,    6,    4,   14,   23,   20,
 /*    40 */     5,   71,   22,   10,   11,   13,   82,   98,   42,   19,
 /*    50 */    24,   17,   18,   15,   16,    7,  103,   77,    4,   14,
 /*    60 */    23,   20,    5,  121,   10,   11,   13,  103,   76,   44,
 /*    70 */    19,   24,   17,   18,   15,   16,    7,  125,  103,    4,
 /*    80 */    14,   23,   20,    5,   13,   40,   83,  127,   19,   24,
 /*    90 */    17,   18,   15,   16,    7,  122,  101,    4,   14,   23,
 /*   100 */    20,    5,   84,  105,  113,  111,   19,   24,   17,   18,
 /*   110 */    15,   16,    7,   64,  120,    4,   14,   23,   20,    5,
 /*   120 */    84,  105,  113,  111,   17,   18,   15,   16,    7,  136,
 /*   130 */    25,    4,   14,   23,   20,    5,    4,   14,   23,   20,
 /*   140 */     5,   71,   34,   33,  113,  111,   71,   12,   74,   81,
 /*   150 */    31,   91,    3,  134,   84,  105,  113,  111,   68,  120,
 /*   160 */    34,   33,  126,   22,   43,   12,   74,   81,   31,    9,
 /*   170 */     3,   32,   36,  105,  113,  111,   21,  115,   29,   70,
 /*   180 */    46,   42,   34,   33,  108,  106,   99,  103,  104,   67,
 /*   190 */   120,  102,  114,   43,   34,   33,  129,   80,  123,   12,
 /*   200 */    74,   81,   31,  109,    1,  110,   45,  105,  113,  111,
 /*   210 */   112,   25,   34,   33,   93,  107,   95,   12,   74,   81,
 /*   220 */    31,  128,    3,   89,  119,  108,  106,   88,  116,   27,
 /*   230 */    28,  129,  105,  113,  111,   79,   35,   65,  117,   90,
 /*   240 */    41,  131,  133,   73,   72,  103,   34,   33,   69,  115,
 /*   250 */    75,   70,   49,   81,  108,  106,   30,   38,   29,  103,
 /*   260 */   104,  100,    2,  102,   66,   43,   23,   20,    5,   78,
 /*   270 */    96,   34,   33,  118,  115,  207,   70,   48,   81,  110,
 /*   280 */   130,  108,  106,   97,  103,  104,  124,  207,  102,  207,
 /*   290 */    43,  207,  115,    7,   70,   60,    4,   14,   23,   20,
 /*   300 */     5,  207,  103,  104,  207,  207,  102,  207,   43,  115,
 /*   310 */   207,   70,   59,  207,  207,  105,  113,  111,  207,  103,
 /*   320 */   104,  207,  207,  102,  207,   43,  115,  207,   70,   58,
 /*   330 */   207,   87,  207,  108,  106,  207,  103,  104,  207,  207,
 /*   340 */   102,  207,   43,  207,  207,  207,  115,  207,   70,   55,
 /*   350 */   207,  207,  207,  207,   34,   33,  103,  104,  207,  207,
 /*   360 */   102,   81,   43,  207,  115,  207,   70,  135,  207,  207,
 /*   370 */   207,  207,  207,  207,  103,  104,  207,  207,  102,  207,
 /*   380 */    43,  115,  207,   70,   92,  207,  207,  207,  207,  207,
 /*   390 */   207,  103,  104,  207,  207,  102,  207,   43,  115,  207,
 /*   400 */    70,   56,  207,  207,  207,  207,  207,  207,  103,  104,
 /*   410 */   207,  207,  102,  207,   43,  207,  207,  207,  115,  207,
 /*   420 */    70,   52,  207,  207,  207,  207,  207,  207,  103,  104,
 /*   430 */   207,  207,  102,  207,   43,  207,  115,  207,   70,   94,
 /*   440 */   207,  207,  207,  207,  207,  207,  103,  104,  207,  207,
 /*   450 */   102,  207,   43,  115,  207,   70,   61,  207,  207,  207,
 /*   460 */   207,  207,  207,  103,  104,  207,  207,  102,  207,   43,
 /*   470 */   115,  207,   70,   50,  207,  207,  207,  207,  207,  207,
 /*   480 */   103,  104,  207,  207,  102,  207,   43,  207,  207,  207,
 /*   490 */   115,  207,   70,   62,  207,  207,  207,  207,  207,  207,
 /*   500 */   103,  104,  207,  207,  102,  207,   43,  207,  115,  207,
 /*   510 */    70,   51,  207,  207,  207,  207,  207,  207,  103,  104,
 /*   520 */   207,  207,  102,  207,   43,  115,  207,   70,   47,  207,
 /*   530 */   207,  207,  207,  207,  207,  103,  104,  207,  207,  102,
 /*   540 */   207,   43,  115,  207,   70,   53,  207,  207,  207,  207,
 /*   550 */   207,  207,  103,  104,  207,  207,  102,  207,   43,  207,
 /*   560 */   207,  207,  115,  207,   70,  132,  207,  207,  207,  207,
 /*   570 */   207,  207,  103,  104,  207,  207,  102,  207,   43,  207,
 /*   580 */   115,  207,   70,   46,  207,  207,  207,  207,  207,  207,
 /*   590 */   103,  104,  207,  207,  102,  207,   43,  115,  207,   70,
 /*   600 */    54,  207,  207,  207,  207,  207,  207,  103,  104,  207,
 /*   610 */   207,  102,  207,   43,  115,  207,   70,   63,  207,  207,
 /*   620 */   207,  207,  207,  207,  103,  104,  207,  207,  102,  207,
 /*   630 */    43,  207,  207,  207,  115,  207,   70,   57,  207,  207,
 /*   640 */   207,  207,  207,  207,  103,  104,  207,  207,  102,  207,
 /*   650 */    43,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    25,   26,   27,   28,   61,   62,   63,   32,   33,   34,
 /*    10 */    35,   36,   37,   38,   71,   53,   41,   42,   43,   44,
 /*    20 */    45,   26,   27,   28,   58,   59,   60,   32,   33,   34,
 /*    30 */    35,   36,   37,   38,   23,   62,   41,   42,   43,   44,
 /*    40 */    45,   21,    1,   26,   27,   28,   61,   52,   23,   32,
 /*    50 */    33,   34,   35,   36,   37,   38,   71,   61,   41,   42,
 /*    60 */    43,   44,   45,   52,   26,   27,   28,   71,   61,   52,
 /*    70 */    32,   33,   34,   35,   36,   37,   38,   52,   71,   41,
 /*    80 */    42,   43,   44,   45,   28,   66,   67,   20,   32,   33,
 /*    90 */    34,   35,   36,   37,   38,   54,   74,   41,   42,   43,
 /*   100 */    44,   45,    1,    2,    3,    4,   32,   33,   34,   35,
 /*   110 */    36,   37,   38,   73,   74,   41,   42,   43,   44,   45,
 /*   120 */     1,    2,    3,    4,   34,   35,   36,   37,   38,    0,
 /*   130 */     1,   41,   42,   43,   44,   45,   41,   42,   43,   44,
 /*   140 */    45,   21,   41,   42,    3,    4,   21,   46,   47,   48,
 /*   150 */    49,   20,   51,   52,    1,    2,    3,    4,   73,   74,
 /*   160 */    41,   42,   75,    1,   77,   46,   47,   48,   49,   23,
 /*   170 */    51,   51,    1,    2,    3,    4,   51,   61,   53,   63,
 /*   180 */    64,   23,   41,   42,   24,   25,    1,   71,   72,   73,
 /*   190 */    74,   75,   20,   77,   41,   42,    1,   67,   52,   46,
 /*   200 */    47,   48,   49,   71,   51,   22,    1,    2,    3,    4,
 /*   210 */    52,    1,   41,   42,   20,   71,   54,   46,   47,   48,
 /*   220 */    49,   52,   51,   22,   20,   24,   25,   60,   52,   24,
 /*   230 */    25,    1,    2,    3,    4,   61,    1,   63,    1,   65,
 /*   240 */     1,   20,   68,   69,   70,   71,   41,   42,    1,   61,
 /*   250 */     1,   63,   64,   48,   24,   25,   51,    1,   53,   71,
 /*   260 */    72,   20,   51,   75,   76,   77,   43,   44,   45,   63,
 /*   270 */    20,   41,   42,   65,   61,   78,   63,   64,   48,   22,
 /*   280 */    65,   24,   25,   65,   71,   72,   20,   78,   75,   78,
 /*   290 */    77,   78,   61,   38,   63,   64,   41,   42,   43,   44,
 /*   300 */    45,   78,   71,   72,   78,   78,   75,   78,   77,   61,
 /*   310 */    78,   63,   64,   78,   78,    2,    3,    4,   78,   71,
 /*   320 */    72,   78,   78,   75,   78,   77,   61,   78,   63,   64,
 /*   330 */    78,   22,   78,   24,   25,   78,   71,   72,   78,   78,
 /*   340 */    75,   78,   77,   78,   78,   78,   61,   78,   63,   64,
 /*   350 */    78,   78,   78,   78,   41,   42,   71,   72,   78,   78,
 /*   360 */    75,   48,   77,   78,   61,   78,   63,   64,   78,   78,
 /*   370 */    78,   78,   78,   78,   71,   72,   78,   78,   75,   78,
 /*   380 */    77,   61,   78,   63,   64,   78,   78,   78,   78,   78,
 /*   390 */    78,   71,   72,   78,   78,   75,   78,   77,   61,   78,
 /*   400 */    63,   64,   78,   78,   78,   78,   78,   78,   71,   72,
 /*   410 */    78,   78,   75,   78,   77,   78,   78,   78,   61,   78,
 /*   420 */    63,   64,   78,   78,   78,   78,   78,   78,   71,   72,
 /*   430 */    78,   78,   75,   78,   77,   78,   61,   78,   63,   64,
 /*   440 */    78,   78,   78,   78,   78,   78,   71,   72,   78,   78,
 /*   450 */    75,   78,   77,   61,   78,   63,   64,   78,   78,   78,
 /*   460 */    78,   78,   78,   71,   72,   78,   78,   75,   78,   77,
 /*   470 */    61,   78,   63,   64,   78,   78,   78,   78,   78,   78,
 /*   480 */    71,   72,   78,   78,   75,   78,   77,   78,   78,   78,
 /*   490 */    61,   78,   63,   64,   78,   78,   78,   78,   78,   78,
 /*   500 */    71,   72,   78,   78,   75,   78,   77,   78,   61,   78,
 /*   510 */    63,   64,   78,   78,   78,   78,   78,   78,   71,   72,
 /*   520 */    78,   78,   75,   78,   77,   61,   78,   63,   64,   78,
 /*   530 */    78,   78,   78,   78,   78,   71,   72,   78,   78,   75,
 /*   540 */    78,   77,   61,   78,   63,   64,   78,   78,   78,   78,
 /*   550 */    78,   78,   71,   72,   78,   78,   75,   78,   77,   78,
 /*   560 */    78,   78,   61,   78,   63,   64,   78,   78,   78,   78,
 /*   570 */    78,   78,   71,   72,   78,   78,   75,   78,   77,   78,
 /*   580 */    61,   78,   63,   64,   78,   78,   78,   78,   78,   78,
 /*   590 */    71,   72,   78,   78,   75,   78,   77,   61,   78,   63,
 /*   600 */    64,   78,   78,   78,   78,   78,   78,   71,   72,   78,
 /*   610 */    78,   75,   78,   77,   61,   78,   63,   64,   78,   78,
 /*   620 */    78,   78,   78,   78,   71,   72,   78,   78,   75,   78,
 /*   630 */    77,   78,   78,   78,   61,   78,   63,   64,   78,   78,
 /*   640 */    78,   78,   78,   78,   71,   72,   78,   78,   75,   78,
 /*   650 */    77,
};
#define YY_SHIFT_USE_DFLT (-39)
#define YY_SHIFT_MAX 89
static const short yy_shift_ofst[] = {
 /*     0 */   210,  171,  101,  119,  119,  119,  153,  119,  119,  119,
 /*    10 */   119,  119,  119,  119,  119,  119,  119,  119,  119,  119,
 /*    20 */   119,  119,  205,  119,  119,  230,  313,  313,  313,   41,
 /*    30 */   256,  247,  235,  141,  141,  201,  257,  125,  309,  129,
 /*    40 */   162,  160,  239,  195,  -38,  -38,   -5,  -25,   17,   38,
 /*    50 */    38,   38,   38,   56,   74,   90,   90,  255,  255,  255,
 /*    60 */   255,   95,  223,  223,   25,  120,  146,  158,   11,  183,
 /*    70 */    20,  237,  221,  241,  249,  211,  250,  266,   20,   67,
 /*    80 */   131,  185,  172,  131,  183,  194,  204,  169,  194,  176,
};
#define YY_REDUCE_USE_DFLT (-58)
#define YY_REDUCE_MAX 45
static const short yy_reduce_ofst[] = {
 /*     0 */   -34,  116,  188,  519,  429,  375,  357,  392,  409,  447,
 /*    10 */   464,  481,  501,  536,  553,  573,  231,  248,  265,  285,
 /*    20 */   303,  213,  174,  320,  337,  -57,  -15,   -4,    7,   19,
 /*    30 */    40,   87,   85,  144,  132,  -27,  -27,  208,  -27,  167,
 /*    40 */   130,  -27,   22,  206,  218,  215,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
 /*    10 */   205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
 /*    20 */   205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
 /*    30 */   205,  205,  205,  205,  205,  205,  201,  205,  205,  205,
 /*    40 */   205,  205,  205,  198,  205,  201,  205,  205,  205,  196,
 /*    50 */   195,  197,  173,  194,  193,  188,  187,  192,  189,  190,
 /*    60 */   191,  186,  181,  182,  205,  205,  205,  205,  205,  205,
 /*    70 */   165,  205,  205,  205,  205,  205,  205,  205,  199,  205,
 /*    80 */   147,  205,  205,  146,  201,  137,  205,  205,  138,  205,
 /*    90 */   148,  155,  184,  143,  185,  144,  152,  141,  176,  163,
 /*   100 */   153,  171,  175,  162,  174,  161,  204,  160,  203,  159,
 /*   110 */   200,  158,  172,  157,  140,  164,  168,  202,  142,  139,
 /*   120 */   170,  169,  145,  178,  151,  166,  179,  150,  167,  201,
 /*   130 */   156,  154,  180,  149,  177,  183,
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
 /*  27 */ "valx ::= DOLLAR ID",
 /*  28 */ "vall ::= valx",
 /*  29 */ "vall ::= dtid",
 /*  30 */ "rexp ::= OPENPAR fldm CLOSEPAR",
 /*  31 */ "rexp ::= OPENPAR ID AT CLOSEPAR",
 /*  32 */ "oexp ::= dtid OPENPAR ID AT CLOSEPAR",
 /*  33 */ "oexp ::= dtid OPENPAR fldm CLOSEPAR",
 /*  34 */ "fldm ::= fldd",
 /*  35 */ "fldm ::= fldm COMMA fldd",
 /*  36 */ "fldd ::= ID eqsc OPENPAR fldm CLOSEPAR",
 /*  37 */ "fldd ::= ID eqsc fldr",
 /*  38 */ "fldr ::= vall",
 /*  39 */ "fldr ::= fldx",
 /*  40 */ "fldr ::= OPENPAR fldr CLOSEPAR",
 /*  41 */ "fldr ::= TILDA ID OPENPAR CLOSEPAR",
 /*  42 */ "fldr ::= TILDA ID OPENPAR fldra CLOSEPAR",
 /*  43 */ "fldr ::= HASH fldx",
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
  { 61, 2 },
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
#line 940 "flow-parser.c"
        break;
      case 1: /* flow ::= stmt */
#line 48 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_flow, yymsp[0].minor.yy0); }
#line 945 "flow-parser.c"
        break;
      case 2: /* flow ::= flow stmt */
      case 11: /* list ::= list elem */ yytestcase(yyruleno==11);
#line 49 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 951 "flow-parser.c"
        break;
      case 3: /* stmt ::= ID valx SEMICOLON */
#line 51 "flow-parser.y"
{ yygotominor.yy0 = ast->stmt_keyw(yymsp[-2].minor.yy0) == FTK_IMPORT? ast->node(FTK_IMPORT, yymsp[-1].minor.yy0): ast->node(ast->stmt_keyw(yymsp[-2].minor.yy0), yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); 
                                                                 ast->expect(yygotominor.yy0, {FTK_DEFINE, FTK_IMPORT}, "import directive or variable definition expected"); 
                                                                 if(ast->stmt_keyw(yymsp[-2].minor.yy0) == FTK_DEFINE) ast->define_var(ast->get_id(yymsp[-2].minor.yy0), yymsp[-1].minor.yy0);
                                                               }
#line 959 "flow-parser.c"
        break;
      case 4: /* stmt ::= ID eqsc valx SEMICOLON */
#line 55 "flow-parser.y"
{ yygotominor.yy0 = ast->node(ast->stmt_keyw(yymsp[-3].minor.yy0), yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); ast->expect(yygotominor.yy0, FTK_DEFINE, "keyword used as variable name");
                                                                 ast->define_var(ast->get_id(yymsp[-3].minor.yy0), yymsp[-1].minor.yy0);
                                                               }
#line 966 "flow-parser.c"
        break;
      case 5: /* stmt ::= ID dtid OPENPAR fldr CLOSEPAR blck */
#line 58 "flow-parser.y"
{ yygotominor.yy0 = ast->node(ast->stmt_keyw(yymsp[-5].minor.yy0), yymsp[-5].minor.yy0, yymsp[-4].minor.yy0, yymsp[0].minor.yy0, yymsp[-2].minor.yy0); ast->expect(yygotominor.yy0, FTK_NODE, "expected \"node\" keyword"); }
#line 971 "flow-parser.c"
        break;
      case 6: /* stmt ::= ID dtid blck */
#line 59 "flow-parser.y"
{ yygotominor.yy0 = ast->node(ast->stmt_keyw(yymsp[-2].minor.yy0), yymsp[-2].minor.yy0, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); 
                                                                 ast->expect(yygotominor.yy0, {FTK_CONTAINER, FTK_NODE, FTK_ENTRY}, "expected \"node\", \"entry\" or \"container\" here"); }
#line 977 "flow-parser.c"
        break;
      case 7: /* stmt ::= stmt SEMICOLON */
      case 8: /* blck ::= OPENBRA list CLOSEBRA */ yytestcase(yyruleno==8);
      case 19: /* elem ::= elem SEMICOLON */ yytestcase(yyruleno==19);
      case 40: /* fldr ::= OPENPAR fldr CLOSEPAR */ yytestcase(yyruleno==40);
      case 64: /* fldn ::= ID AT */ yytestcase(yyruleno==64);
#line 61 "flow-parser.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 986 "flow-parser.c"
        break;
      case 9: /* blck ::= OPENBRA CLOSEBRA */
#line 64 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(yymsp[-1].minor.yy0, FTK_blck); }
#line 991 "flow-parser.c"
        break;
      case 10: /* list ::= elem */
#line 68 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_blck, yymsp[0].minor.yy0); }
#line 996 "flow-parser.c"
        break;
      case 12: /* elem ::= ID blck */
      case 13: /* elem ::= ID lblk */ yytestcase(yyruleno==13);
#line 74 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1002 "flow-parser.c"
        break;
      case 14: /* elem ::= ID valx SEMICOLON */
      case 17: /* elem ::= ID oexp SEMICOLON */ yytestcase(yyruleno==17);
      case 18: /* elem ::= ID rexp SEMICOLON */ yytestcase(yyruleno==18);
#line 76 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-2].minor.yy0, yymsp[-1].minor.yy0); }
#line 1009 "flow-parser.c"
        break;
      case 15: /* elem ::= ID EQUALS valx SEMICOLON */
      case 16: /* elem ::= ID COLON valx SEMICOLON */ yytestcase(yyruleno==16);
#line 77 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_elem, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 1015 "flow-parser.c"
        break;
      case 20: /* lblk ::= ID blck */
#line 83 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_lblk, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1020 "flow-parser.c"
        break;
      case 21: /* valn ::= INTEGER */
      case 22: /* valn ::= FLOAT */ yytestcase(yyruleno==22);
      case 23: /* valn ::= PLUS valn */ yytestcase(yyruleno==23);
      case 25: /* valx ::= STRING */ yytestcase(yyruleno==25);
      case 26: /* valx ::= valn */ yytestcase(yyruleno==26);
      case 28: /* vall ::= valx */ yytestcase(yyruleno==28);
      case 38: /* fldr ::= vall */ yytestcase(yyruleno==38);
      case 39: /* fldr ::= fldx */ yytestcase(yyruleno==39);
      case 62: /* fldx ::= fldn */ yytestcase(yyruleno==62);
#line 87 "flow-parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 1033 "flow-parser.c"
        break;
      case 24: /* valn ::= MINUS valn */
#line 90 "flow-parser.y"
{ yygotominor.yy0 = ast->negate(yymsp[0].minor.yy0); }
#line 1038 "flow-parser.c"
        break;
      case 27: /* valx ::= DOLLAR ID */
#line 96 "flow-parser.y"
{ yygotominor.yy0 = ast->lookup_var(ast->get_id(yymsp[0].minor.yy0)); if(yygotominor.yy0 == 0) ast->error(yymsp[0].minor.yy0, stru1::sfmt() << "reference to undefined symbol \"" << ast->get_id(yygotominor.yy0=yymsp[0].minor.yy0) << "\""); }
#line 1043 "flow-parser.c"
        break;
      case 29: /* vall ::= dtid */
#line 101 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(yymsp[0].minor.yy0, FTK_enum); }
#line 1048 "flow-parser.c"
        break;
      case 30: /* rexp ::= OPENPAR fldm CLOSEPAR */
#line 105 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_RETURN, yymsp[-1].minor.yy0); }
#line 1053 "flow-parser.c"
        break;
      case 31: /* rexp ::= OPENPAR ID AT CLOSEPAR */
#line 106 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_RETURN, ast->node(FTK_fldx, yymsp[-2].minor.yy0)); }
#line 1058 "flow-parser.c"
        break;
      case 32: /* oexp ::= dtid OPENPAR ID AT CLOSEPAR */
#line 110 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_OUTPUT, yymsp[-4].minor.yy0, ast->node(FTK_fldx, yymsp[-2].minor.yy0)); }
#line 1063 "flow-parser.c"
        break;
      case 33: /* oexp ::= dtid OPENPAR fldm CLOSEPAR */
#line 111 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_OUTPUT, yymsp[-3].minor.yy0, yymsp[-1].minor.yy0); }
#line 1068 "flow-parser.c"
        break;
      case 34: /* fldm ::= fldd */
#line 113 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldm, yymsp[0].minor.yy0); }
#line 1073 "flow-parser.c"
        break;
      case 35: /* fldm ::= fldm COMMA fldd */
      case 61: /* fldra ::= fldra COMMA fldr */ yytestcase(yyruleno==61);
      case 66: /* dtid ::= dtid DOT ID */ yytestcase(yyruleno==66);
#line 114 "flow-parser.y"
{ yygotominor.yy0 = ast->nappend(yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 1080 "flow-parser.c"
        break;
      case 36: /* fldd ::= ID eqsc OPENPAR fldm CLOSEPAR */
#line 118 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-4].minor.yy0, yymsp[-1].minor.yy0); ast->chinteger(yygotominor.yy0, ast->at(yymsp[-3].minor.yy0).token.integer_value); }
#line 1085 "flow-parser.c"
        break;
      case 37: /* fldd ::= ID eqsc fldr */
#line 119 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldd, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); ast->chinteger(yygotominor.yy0, ast->at(yymsp[-1].minor.yy0).token.integer_value); }
#line 1090 "flow-parser.c"
        break;
      case 41: /* fldr ::= TILDA ID OPENPAR CLOSEPAR */
#line 126 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-2].minor.yy0); }
#line 1095 "flow-parser.c"
        break;
      case 42: /* fldr ::= TILDA ID OPENPAR fldra CLOSEPAR */
#line 127 "flow-parser.y"
{ yygotominor.yy0 = ast->nprepend(yymsp[-1].minor.yy0, yymsp[-3].minor.yy0); }
#line 1100 "flow-parser.c"
        break;
      case 43: /* fldr ::= HASH fldx */
      case 44: /* fldr ::= BANG fldr */ yytestcase(yyruleno==44);
#line 128 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-1].minor.yy0, yymsp[0].minor.yy0); }
#line 1106 "flow-parser.c"
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
#line 130 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-1].minor.yy0, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 1124 "flow-parser.c"
        break;
      case 59: /* fldr ::= fldr QUESTION fldr COLON fldr */
#line 144 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[-3].minor.yy0, yymsp[-4].minor.yy0, yymsp[-2].minor.yy0, yymsp[0].minor.yy0); }
#line 1129 "flow-parser.c"
        break;
      case 60: /* fldra ::= fldr */
#line 146 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_fldr, yymsp[0].minor.yy0); }
#line 1134 "flow-parser.c"
        break;
      case 63: /* fldx ::= fldn dtid */
#line 152 "flow-parser.y"
{ yygotominor.yy0 = ast->chtype(ast->nprepend(yymsp[0].minor.yy0, yymsp[-1].minor.yy0), FTK_fldx); }
#line 1139 "flow-parser.c"
        break;
      case 65: /* dtid ::= ID */
#line 160 "flow-parser.y"
{ yygotominor.yy0 = ast->node(FTK_dtid, yymsp[0].minor.yy0); }
#line 1144 "flow-parser.c"
        break;
      case 67: /* eqsc ::= EQUALS */
      case 68: /* eqsc ::= COLON */ yytestcase(yyruleno==68);
#line 165 "flow-parser.y"
{ ast->chinteger(yygotominor.yy0, 0); }
#line 1150 "flow-parser.c"
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
#line 1201 "flow-parser.c"
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
#line 1219 "flow-parser.c"
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
#line 1241 "flow-parser.c"
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
