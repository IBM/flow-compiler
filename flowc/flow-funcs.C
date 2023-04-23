#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include "flow-compiler.H"
#include "stru.H"
#include "grpc-helpers.H"
#include "flow-ast.H"
#include "massert.H"

enum func_type {
    A_NONE = 0,
    A_ARG = 128,        // argument's type flag
    A_RPR = 64,         // repeated return bit flag
    A_REP = 32,         // repeated flag
    A_TYPM = (A_REP-1), // mask for getting the base type

    A_ANY = 1,          // any type
    A_STR,              // string
    A_INT,              // integer 
    A_FLT,              // float
    A_NUM,              // either integer or float
    A_PRI,              // primitive type: integer, float or string
    A_MSG,              // any message type

    A_RARG = A_ARG+A_REP,
    A_RANY = A_ANY+A_REP,
    A_RSTR = A_STR+A_REP,
    A_RINT = A_INT+A_REP,
    A_RFLT = A_FLT+A_REP,
    A_RNUM = A_NUM+A_REP,
    A_RPRI = A_PRI+A_REP,
    A_RMSG = A_MSG+A_REP,

    A_RRARG = A_ARG+A_REP+A_RPR,
    A_RRANY = A_ANY+A_REP+A_RPR,
    A_RRSTR = A_STR+A_REP+A_RPR,
    A_RRINT = A_INT+A_REP+A_RPR,
    A_RRFLT = A_FLT+A_REP+A_RPR,
    A_RRNUM = A_NUM+A_REP+A_RPR,
    A_RRPRI = A_PRI+A_REP+A_RPR,
    A_RRMSG = A_MSG+A_REP+A_RPR,

    A_ARG1 = A_ARG+1,
    A_ARG2 = A_ARG+2,
    A_ARG3 = A_ARG+3,
    A_ARG4 = A_ARG+4,
    A_ARG5 = A_ARG+5,
    A_ARG6 = A_ARG+6,
    A_ARG7 = A_ARG+7,
    A_ARG8 = A_ARG+8,

    A_RRARG1 = A_RRARG+1,
    A_RRARG2 = A_RRARG+2,
    A_RRARG3 = A_RRARG+3,
    A_RRARG4 = A_RRARG+4,
    A_RRARG5 = A_RRARG+5,
    A_RRARG6 = A_RRARG+6,
    A_RRARG7 = A_RRARG+7,
    A_RRARG8 = A_RRARG+8,
};
struct function_info {
    func_type return_type;
    std::vector<std::pair<func_type, std::string>> arg_tyna;
    int required_argc;
    int dim;
    bool enable;
    bool inl;
    char const *help;
    int arg_count() const {
        return int(arg_tyna.size());
    }
    func_type base_type(func_type type) const {
        return func_type(((type & A_ARG)? arg_tyna[(type & A_TYPM)-1].first: type) & A_TYPM);
    }
    func_type return_base_type() const {
        return base_type(func_type(return_type));
    }
    func_type arg_base_type(int i) const {
        return func_type(arg_tyna[i].first & A_TYPM);
    }
    bool return_repeated() const {
        return (return_type & A_RPR) != 0;
    }
    bool arg_repeated(int i) const {
        return (((arg_tyna[i].first & A_ARG)? arg_tyna[(arg_tyna[i].first & A_TYPM)-1].first: arg_tyna[i].first) & A_REP) != 0;
    }
    func_type return_full_type() const {
        return func_type(return_base_type() | (return_repeated()? A_REP: 0));
    }
    func_type arg_full_type(int i) const {
        return func_type(arg_base_type(i) | (arg_repeated(i)? A_REP: 0));
    }
    int return_arg_index() const {
        return -1 + ((return_type & A_ARG)? (return_type & A_TYPM): 0);
    }
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
    { "strslice",    {A_STR,   { {A_STR, ""}, {A_INT, "begin"}, {A_INT, "end"} },  2, 0, true, true, 
      "Returns the substring between the byte indices in the second and tird arguents.\n"}},
    { "substr",      {A_STR,   { {A_STR, ""}, {A_INT, "begin"}, {A_INT, "end"} },  2, 0, true, true,
      "Returns the substring between the utf-8 character indices in the second an third arguments.\n"}},
    // int length(string s)
    { "length",      {A_INT,  { {A_STR, ""} },  1, 0, true, true, 
      "Returns the number of utf-8 characters in the argument string.\n"}},
    { "size",        {A_INT,  { {A_STR, ""} },  1, 0, true, true,
      "Returns the size of the string argument in bytes.\n"}},
    // string *trim(string s, string strip_chars)
    { "trim",        {A_STR,   { {A_STR, ""}, {A_STR, "chars"} },   1, 0, true, true, 
      "Deletes all the characters in the second argument string from both ends of the first argument string.\n"
      "If no second argument is given, white-space is deleted.\n"}},
    { "ltrim",       {A_STR,   { {A_STR, ""}, {A_STR, "chars"} },  1, 0, true, true, 
      "Deletes all the characters in the second argument string from the beginning of the first argument string.\n"
      "If no second argument is given, white-space is deleted.\n"}},
    { "rtrim",       {A_STR,   { {A_STR, ""}, {A_STR, "chars"} },  1, 0, true, true,
      "Deletes all the characters in the second argument string from the end of the first argument string.\n"
      "If no second argument is given, white-space is deleted.\n"}},
    // string to*(string s)
    { "toupper",     {A_STR,   { {A_STR, ""} },  1, 0, true, true,
      "Converts all characters in the argument string to upper case (ASCII only).\n"}},
    { "tolower",     {A_STR,   { {A_STR, ""} },  1, 0, true, true,
      "Converts all characters in the argument string to lower case (ASCII only).\n"}},
    { "toid",        {A_STR,   { {A_STR, ""} },  1, 0, true, true,
      "Converts the string argument to a string that can be used as an identifier, by replacing all illegal character runs with an underscore('_').\n"}},
    { "tocname",     {A_STR,   { {A_STR, ""} },  1, 0, true, true,
      "Converts the string in the argument to a cannonical name by replacing all underscore('_') and space(' ') character runs with a dash('-').\n"}},
    { "camelize",    {A_STR,   { {A_STR, ""} },  1, 0, true, true,
      "Converts the string in the argument to a camel-cased identifier string.\n"}},
    { "decamelize",  {A_STR,   { {A_STR, ""} },  1, 0, true, true,
      "Converts a camel-cased string to a readable string.\n"}},
    // string tr(string s, string match_list, string replace_list)
    { "tr",          {A_STR,   { {A_STR, ""}, {A_STR, "set1"}, {A_STR, "set2"} },  2, 0, true, true,
      "Returns the transformation of the string given as first argument by replacing all the characters in the second argument\n"
      "with the corresponding character in the third argument. If there is no corresponding character, the character is deleted.\n"}},

    // string getenv(name, default_value)
    { "getenv",      {A_STR,   { {A_STR, "variable_name"}, {A_STR, "default_value"} },  1, 0, true, true,
      "Returns the value of the environment variable named by the first argument. If the environment variable is missing,\n"
      "the value of the second argument is returned.\n"}},
    // string join(elements, separator, last_separator)
    { "join",        {A_STR,   { {A_RPRI, ""}, {A_STR, "separator"}, {A_STR, "last_separator"} },  1, -1, true, true,
      "Returns the concatenation of the elements of the repeated field, after converstion to string.\n"
      "The second argument is used as separator, and the third, if given as the last separator.\n"}},
    { "split",       {A_RRSTR,   { {A_STR, "begin"}, {A_STR, "end"} },  1, 1, false, false,
      "Splits the first argument by any character in the second argument. By default it splits on ASCII whitespace.\n"}},
    { "slice",       {A_RRARG1,  { {A_RANY, ""}, {A_INT, "begin"}, {A_INT, "end"} },  2, 0, true, false,
      "Returns a subsequence of the repeated field. Both begin and end indices can be negative.\n"}},
    { "partition",   {A_ARG1,  { {A_RANY, ""}, {A_INT, ""} },  2, 1, false, false,
      "Returns a sequence of subsequences of the repeated field. The second argument is the number of sequences to divide into.\n"}},
    { "batch",       {A_ARG1,  { {A_RANY, ""}, {A_INT, ""} },  2, 1, false, false,
      "Returns a sequence of subsequences of the repeated field, no longer than the argument.\n"}},
    { "cat",         {A_RRARG1,  { {A_RANY, ""}, {A_ARG1, ""} },  2, 0, false, false,
      "Returns the concatenation of arguments.\n"}},
    { "flatten",     {A_RRARG1,  { {A_RANY, ""} },  1, -1, true, false,
      "Retruns a one dimensional repeated field from a field of any dimension\n" }},
    { "sum",         {A_ARG1,  { {A_RNUM, ""} },  1, -1, true, true,
      "Returns the sum of the numeric repeated field, preserving type.\n"}},
    { "min",         {A_ARG1,  { {A_RNUM, ""} },  1, -1, true, true,
      "Returns the minimum of the numeric repeated field, preserving type. It returns 0 if the field is empty.\n"}},
    { "max",         {A_ARG1,  { {A_RNUM, ""} },  1, -1, true, true,
      "Returns the maximum of the numeric repeated field, preserving type. It returns 0 if the field is empty.\n"}},
    { "uniq",        {A_RRARG1,  { {A_RPRI, ""} },  1, 0, true, false,
      "Removes duplicates from a repeated field.\n"}},
    { "sort",        {A_RRARG1,  { {A_RPRI, ""}, {A_NUM, "reverse"} },  1, 0, true, false,
      "Sort the repeated field. Sorts in reverse if the second paramer is non-zero.\n"}},
};
/**
 * Convert a primitive type to FTK
 */
static int at2ftk(func_type t) {
    switch(t & A_TYPM) {
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
static std::string at2s(func_type t) {
    std::string ts;
    if((t & (A_REP|A_RPR)) != 0)  {
        ts = "repeated ";
        t = func_type(t & A_TYPM);
    }
    switch(t) {
        case A_STR: ts += "string"; break;
        case A_INT: ts += "int"; break;
        case A_FLT: ts += "float"; break;
        case A_NUM: ts += "numeric"; break;
        case A_PRI: ts += "primitive type"; break;
        case A_ANY: ts += "type"; break;
        case A_MSG: ts += "message type"; break;
        default: ts += std::to_string(t); break;
    }
    return ts;
}
static std::string at2sq(func_type t) {
    return "'" + at2s(t) + "'";
}
static std::string func_proto(std::string fname, function_info const &fe, bool quote=false) {
    std::ostringstream out;
    auto rtype = fe.return_full_type();
    out << (quote?at2sq(rtype):at2s(rtype)) << " \"~" << fname << "\"(";
    auto tyna2str = [quote] (decltype(fe.arg_tyna)::value_type const &tyna) { 
        std::string type = quote? at2sq(tyna.first):at2s(tyna.first);
        if(!tyna.second.empty())
            type = type + " " + tyna.second; 
        return type;
    };
    out << stru::joint(fe.arg_tyna.begin(), fe.arg_tyna.begin()+fe.required_argc, tyna2str, ", ");
    if(fe.arg_count() != fe.required_argc) {
        out << "[";
        if(fe.required_argc > 0) out << ", ";
        out << stru::joint(fe.arg_tyna.begin()+fe.required_argc, fe.arg_tyna.end(), tyna2str, ", ");
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
static bool compare_atype(func_type fat, int ftk) {
    return fat == A_ANY ||
        fat == A_PRI && (ftk == FTK_INTEGER || ftk == FTK_FLOAT || ftk == FTK_STRING) ||
        fat == A_NUM && (ftk == FTK_INTEGER || ftk == FTK_FLOAT) ||
        fat == ftk;
}
int flow_compiler::check_function(int *rvt, std::string fname, int funcnode, int errnode) {
    int error_count = 0;
    DEBUG_ENTERA(funcnode << ", " << fname);
    std::string errmsg;
    auto const &args = at(funcnode).children;
    int argc = args.size()-1;   
    auto funp = function_table.find(fname);
    // check the function name
    if(funp == function_table.end() || !funp->second.enable) {
        errmsg = stru::sfmt() << "unknown function \"~" << fname << "\"";
        funp = function_table.end();
        ++error_count;
    } else {
        // check the number of arguments
        if(funp->second.required_argc == funp->second.arg_count() && funp->second.required_argc != argc) {
            errmsg = stru::sfmt() << "function \"~" << fname << "\" takes " << funp->second.required_argc << " arguments but " << argc << " were given";
            ++error_count;
        } else if(funp->second.required_argc != funp->second.arg_count() && funp->second.required_argc > argc || funp->second.arg_count() < argc) {
            errmsg = stru::sfmt() << "function \"~" << fname << "\" takes at least " << funp->second.required_argc 
                << " and at most " << funp->second.arg_count() << " arguments but " << argc << " were given";
            ++error_count;
        }
    }
    if(error_count == 0) {
        // check the argument types
        for(int i = 0; i < argc; ++i) {
            auto atype = funp->second.arg_base_type(i);
            auto ntype = value_type(args[i+1]);

            if(!compare_atype(atype, ntype)) {
                //print_ast(std::cout, funcnode);
                if(errnode != 0)
                    errnode = args[i+1];
                errmsg = stru::sfmt() << "argument " << (i+1) << " for \"~" << fname << "\"  must be of type \"" << at2s(atype) << "\"";
                ++error_count;
            }
        }
    }
    if(error_count != 0) {
        if(errnode != 0) {
            pcerr.AddError(main_file, at(errnode), errmsg);
            if(funp != function_table.end())
                pcerr.AddNote(main_file, at(funcnode), stru::sfmt() << func_proto(funp->first, funp->second));
        }
    } else {
        // The return type can be the type of one of the arguments
        int vx = funp->second.return_arg_index();
        int vt =  vx < 0?  at2ftk(funp->second.return_base_type()): value_type(args[vx+1]);
        if(rvt != nullptr) *rvt = vt;
    }
    DEBUG_LEAVE;
    return error_count;
}
int get_fdimchange(std::string fname) {
    auto funp = function_table.find(fname);
    return funp->second.dim;
}
bool get_isinline(std::string fname) {
    auto const &fni = function_table.find(fname)->second;
    //return !fni.return_repeated() || fni.dim < 0;
    return fni.inl;
}
std::vector<std::pair<int, bool>> get_fargtypes(std::string fname) {
    std::vector<std::pair<int, bool>> argtypes;
    auto const &fni = function_table.find(fname)->second;
    for(int i = 0, c = fni.arg_count(); i < c; ++i)
        argtypes.push_back(std::make_pair(fni.arg_base_type(i), fni.arg_repeated(i)));
    return argtypes;
}
