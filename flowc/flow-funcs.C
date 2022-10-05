#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include "flow-compiler.H"
#include "stru1.H"
#include "grpc-helpers.H"
#include "flow-ast.H"
#include "massert.H"

enum function_arg_type {
    A_NONE = 0,
    A_ARG = 128,    // argument's type flag
    A_REP = 64,     // repeated bit flag
    A_ANY = 1,      // any type
    A_STR,          // string
    A_INT,          // integer 
    A_FLT,          // float
    A_NUM,          // either integer or float
    A_PRI,          // primitive type: integer, float or string
    A_MSG,          // any message type

    A_RANY = A_ANY+A_REP,  
    A_RSTR = A_STR+A_REP,
    A_RINT = A_INT+A_REP,
    A_RFLT = A_FLT+A_REP,
    A_RNUM = A_NUM+A_REP,
    A_RPRI = A_PRI+A_REP,
    A_RMSG = A_MSG+A_REP
};
struct function_info {
    int return_type;
    std::vector<int> arg_type;
    unsigned required_argc;
    int dim;
    bool enable;
    char const *help;
};
/** 
 * function signature table:
 *      return_type: Either a basic grpc compiler type, or A_NONE for a type that is deduced from the argument types. A negative value denotes a repeated field.
 *      arg_type: Type for each argument.
 *      required_argc: The number of required arguments. The number of accepted arguments is the size of arg_type.
 *      dim: The change in dimension when compared to arguments' dimension.
 */
static const std::map<std::string, function_info> function_table = {
    // string substr(string s, int begin, int end)
    { "strslice",    { A_STR,  { A_STR, A_INT, A_INT }, 2, 0, true, 
      "Returns the substring indiacted by the byte indices in the second and tird arguents.\n"}},
    { "substr",      { A_STR,  { A_STR, A_INT, A_INT }, 2, 0, true, 
      "Returns the substring indicated by the utf-8 character indices in the second an third arguments.\n"}},
    // int length(string s)
    { "length",      { A_INT, { A_STR }, 1, 0, true, 
      "Returns the number of utf-8 characters in the argument string.\n"}},
    { "size",        { A_INT, { A_STR }, 1, 0, true,
      "Returns the size of the string argument in bytes.\n"}},
    // string *trim(string s, string strip_chars)
    { "trim",        { A_STR,  { A_STR, A_STR }, 1, 0, true, 
      "Deletes all the characters in the second argument string from both ends of the first argument string.\n"
      "If no second argument is given, white-space is deleted.\n"}},
    { "ltrim",       { A_STR,  { A_STR, A_STR }, 1, 0, true, 
      "Deletes all the characters in the second argument string from the beginning of the first argument string.\n"
      "If no second argument is given, white-space is deleted.\n"}},
    { "rtrim",       { A_STR,  { A_STR, A_STR }, 1, 0, true, 
      "Deletes all the characters in the second argument string from the end of the first argument string.\n"
      "If no second argument is given, white-space is deleted.\n"}},
    // string to*(string s)
    { "toupper",     { A_STR,  { A_STR }, 1, 0, true,
      "Converts all characters in the argument string to upper case (ASCII only).\n"}},
    { "tolower",     { A_STR,  { A_STR }, 1, 0, true, 
      "Converts all characters in the argument string to lower case (ASCII only).\n"}},
    { "toid",        { A_STR,  { A_STR }, 1, 0, true, 
      "Converts the string argument to a string that can be used as an identifier, by replacing all illegal character runs with an underscore('_').\n"}},
    { "tocname",     { A_STR,  { A_STR }, 1, 0, true, 
      "Converts the string in the argument to a cannonical name by replacing all underscore('_') and space(' ') character runs with a dash('-').\n"}},
    { "camelize",    { A_STR,  { A_STR }, 1, 0, true, 
      "Converts the string in the argument to a camel-cased identifier string.\n"}},
    { "decamelize",  { A_STR,  { A_STR }, 1, 0, true, 
      "Converts a camel-cased string to a readable string.\n"}},
    // string tr(string s, string match_list, string replace_list)
    { "tr",          { A_STR,  { A_STR, A_STR, A_STR }, 2, 0, true, 
      "Returns the transformation of the string given as first argument by replacing all the characters in the second argument\n"
      "with the corresponding character in the third argument. If there is no corresponding character, the character is deleted.\n"}},
    // string getenv(name, default_value)
    { "getenv",      { A_STR,  { A_STR, A_STR }, 1, 0, true, 
      "Returns the value of the environment variable named by the first argument. If the environment variable is missing,\n"
      "the value of the second argument is returned.\n"}},
    // string join(elements, separator, last_separator)
    { "join",        { A_STR,  { A_RPRI, A_STR, A_STR }, 1, -1, true, 
      "Returns the concatenation of the elements of the repeated field, after converstion to string.\n"
      "The second argument is used as separator, and the third, if given as the last separator.\n"}},
    { "split",       { A_RSTR,  { A_STR, A_STR }, 1, 1, false,
      "Splits the first argument by any character in the second argument. By default it splits on ASCII whitespace.\n"}},
    { "slice",       { A_ARG+1, { A_RANY, A_INT, A_INT }, 2, 0, true,
      "Returns a subsequence of the repeated field. Both begin and end indices can be negative.\n"}},
    { "partition",   { A_ARG+1, { A_RANY, A_INT }, 2, 1, false,
      "Returns a sequence of subsequences of the repeated field. The second argument is the number of sequences to divide into.\n"}},
    { "batch",       { A_ARG+1, { A_RANY, A_INT }, 2, 1, false, 
      "Returns a sequence of subsequences of the repeated field, no longer than the argument.\n"}},
    { "cat",         { A_ARG+1, { A_RANY, A_ARG+1 }, 2, 0, false,
      "Returns the concatenation of arguments.\n"}},
    { "sum",         { A_ARG+1, { A_RNUM }, 1, -1, true, 
      "Returns the sum of the numeric repeated field, preserving type.\n"}},
    { "min",         { A_ARG+1, { A_RNUM }, 1, -1, true, 
      "Returns the minimum of the numeric repeated field, preserving type. It returns 0 if the field is empty.\n"}},
    { "max",         { A_ARG+1, { A_RNUM }, 1, -1, true, 
      "Returns the maximum of the numeric repeated field, preserving type. It returns 0 if the field is empty.\n"}},
    { "uniq",        { A_ARG+1, { A_RPRI }, 1, 0, true, 
      "Removes duplicates from a repeated field.\n"}},
    { "sort",        { A_ARG+1, { A_RPRI, A_NUM }, 1, 0, true, 
      "Sort the repeated field. Sorts in reverse if the second paramer is non-zero.\n"}},
};

static std::string at2s(int t) {
    std::string ts;
    if((t & A_REP) != 0)  {
        ts = "repeated ";
        t = t & (A_REP-1);
    }
    switch(t) {
        case A_STR: ts += "string"; break;
        case A_INT: ts += "int"; break;
        case A_FLT: ts += "float"; break;
        case A_NUM: ts += "numeric"; break;
        case A_PRI: ts += "primitive type"; break;
        case A_ANY: ts += "type"; break;
        case A_MSG: ts += "message type"; break;
        default: ts += "?"; break;
    }
    return ts;
}
static std::string at2sq(int t) {
    return "'" + at2s(t) + "'";
}
static std::string func_proto(std::string fname, function_info const &fe, bool quote=false) {
    std::ostringstream out;
    auto rtype = fe.return_type;
    if(rtype & A_ARG) {
        rtype = fe.arg_type[ (rtype & (A_ARG-1)) -1 ];
        if((rtype & A_REP) && fe.dim < 0)
            rtype = rtype & (A_REP-1);
    }
    out << (quote?at2sq(rtype):at2s(rtype)) << " \"~" << fname << "\"(";
    if(fe.arg_type.size() == fe.required_argc) {
        out << stru1::joint(fe.arg_type, quote?at2sq:at2s, ", ");
    } else {
        out << stru1::joint(fe.arg_type.begin(), fe.arg_type.begin()+fe.required_argc, quote?at2sq:at2s, ", ");
        out << "[";
        if(fe.required_argc > 0) out << ", ";
        out << stru1::joint(fe.arg_type.begin()+fe.required_argc, fe.arg_type.end(), quote?at2sq:at2s, ", ");
        out << "]";
    }
    out << ");";
    return out.str();
}
void show_builtin_help(std::ostream &out) {
    for(auto const &fe: function_table) if(fe.second.enable) {
        out << func_proto(fe.first, fe.second, true) << "\n";
        out << fe.second.help << "\n";
    }
}
static int at2ftk(function_arg_type t) {
    switch(t & (A_REP-1)) {
        case A_STR: 
            return FTK_STRING;
        case A_INT: 
            return FTK_INTEGER;
        case A_FLT: 
            return FTK_FLOAT;
        case A_ANY:
            return 0;
        default:
            break;
    }
    return 0;
}
static bool compare_atype(function_arg_type fat, int ftk) {
    return fat == A_ANY ||
        fat == A_PRI && (ftk == FTK_INTEGER || ftk == FTK_FLOAT || ftk == FTK_STRING) ||
        fat == A_NUM && (ftk == FTK_INTEGER || ftk == FTK_FLOAT) ||
        fat == ftk;
}
int flow_compiler::check_function(std::string fname, int funcnode, int errnode) {
    std::string errmsg;
    auto const &args = at(funcnode).children;
    int argc = args.size()-1;   
    int rc = -1;
    auto funp = function_table.find(fname);
    // check the function name
    if(funp == function_table.end() || !funp->second.enable) {
        errmsg = stru1::sfmt() << "unknown function \"~" << fname << "\"";
        funp = function_table.end();
    } else {
        // check the number of arguments
        if(funp->second.required_argc == funp->second.arg_type.size() && funp->second.required_argc != argc) 
            errmsg = stru1::sfmt() << "function \"~" << fname << "\" takes " << funp->second.required_argc << " arguments but " << argc << " were given";
        else if(funp->second.required_argc != funp->second.arg_type.size() && funp->second.required_argc > argc || funp->second.arg_type.size() < argc) 
            errmsg = stru1::sfmt() << "function \"~" << fname << "\" takes at least " << funp->second.required_argc 
                << " and at most " << funp->second.arg_type.size() << " arguments but " << argc << " were given";
        else 
            rc = 0;
    }
    if(rc == 0) {
        // check the argument types
        for(int i = 0; i < argc; ++i) {
            auto atype = function_arg_type(funp->second.arg_type[i]);
            if(atype & A_ARG)
                atype = function_arg_type(funp->second.arg_type[(atype & (A_ARG-1))-1]);
            auto ntype = value_type(args[i+1]);

            bool repeated = atype & A_REP;
            atype = function_arg_type(atype & (A_REP-1));
             
            if(!compare_atype(atype, ntype)) {
                //print_ast(std::cout, funcnode);
                if(errnode != 0)
                    errnode = args[i+1];
                errmsg = stru1::sfmt() << "argument " << (i+1) << " for \"~" << fname << "\"  must be of type \"" << at2s(atype) << "\"";
                rc = -1;
            }
        }
    }
    if(rc != 0) {
        if(errnode != 0) {
            pcerr.AddError(main_file, at(errnode), errmsg);
            if(funp != function_table.end())
                pcerr.AddNote(main_file, at(funcnode), stru1::sfmt() << func_proto(funp->first, funp->second));
        }
        return rc;
    }
    // The return type can be the type of one of the arguments
    if((funp->second.return_type & A_ARG) == A_ARG) {
        auto type = value_type(args[funp->second.return_type & (A_ARG-1)]);
        return type;
    }
    int type = at2ftk(function_arg_type(funp->second.return_type));
    return type;
}
int get_fdimchange(std::string fname) {
    auto funp = function_table.find(fname);
    return funp->second.dim;
}
bool get_isinline(std::string fname) {
    auto funp = function_table.find(fname);
    int type = ((funp->second.return_type & A_ARG) == A_ARG)?
        funp->second.arg_type[(funp->second.return_type & (A_ARG-1))-1]:
        funp->second.return_type;
    return (type & A_REP) == 0 || funp->second.dim < 0;
}
std::vector<std::pair<int, bool>> get_fargtypes(std::string fname) {
    std::vector<std::pair<int, bool>> argtypes;
    auto funp = function_table.find(fname);
    for(auto at: funp->second.arg_type) {
        if(at & A_ARG)
            at = funp->second.arg_type[(at & (A_ARG-1))-1];
        int ftk = at2ftk(function_arg_type(at));
        argtypes.push_back(std::make_pair(ftk, (at & A_REP) != 0));
    }
    return argtypes;
}
