#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wlogical-op-parentheses"
#endif

#include "flow-compiler.H"
#include "stru1.H"
#include "grpc-helpers.H"
#include "flow-ast.H"
#include "flow-parser.c"
#include "massert.H"

using namespace stru1;

static void get_enums(std::set<EnumValueDescriptor const *> &edset, Descriptor const *dd) {
    for(int e = 0, ec = dd->enum_type_count(); e != ec; ++e) {
        auto ed = dd->enum_type(e);
        for(int ev = 0, evc = ed->value_count(); ev != evc; ++ev) {
            auto evd = ed->value(ev);
            edset.insert(evd);
        }
    }
    for(int nd = 0, ndc = dd->nested_type_count(); nd != ndc; ++nd) 
        get_enums(edset, dd->nested_type(nd));
    
}
static void get_enums(std::set<EnumValueDescriptor const *> &edset, FileDescriptor const *fid) {
    // Get all enums at global scope
    for(int e = 0, ec = fid->enum_type_count(); e != ec; ++e) {
        auto ed = fid->enum_type(e);
        for(int ev = 0, evc = ed->value_count(); ev != evc; ++ev) {
            auto evd = ed->value(ev);
            edset.insert(evd);
        }
    }
    for(int d = 0, dc = fid->message_type_count(); d != dc; ++d) {
        auto dd = fid->message_type(d);
        get_enums(edset, dd);
    }
}

int flow_compiler::compile_proto(std::string const &file) {
    auto fdp = importer.Import(file);
    if(fdp == nullptr) {
        pcerr.AddError(file, -1, 0, "import failed");
        return 1;
    }
    if(fdp->syntax() != FileDescriptor::Syntax::SYNTAX_PROTO3) {
        pcerr.AddError(file, -1, 0, "syntax must be proto3");
        return 1;
    }
    std::set<FileDescriptor const *> included(fdps.begin(), fdps.end());
    if(contains(included, fdp)) return 0;
    fdps.push_back(fdp);
    included.insert(fdp);
    get_enums(enum_value_set, fdp);
    // Add the dependents to our set
    for(int i = 0, e = fdp->dependency_count(); i != e; ++i) {
        auto fdq = fdp->dependency(i);
        if(!contains(included, fdq)) {
            fdps.push_back(fdq);
            included.insert(fdq);
            get_enums(enum_value_set, fdq);
        }
    }
    return 0;
}
/**
 * The method can be spcified as [package.][service.]method_name
 * The package is either the label defined with the package directive or the basename of the .proto file
 */
MethodDescriptor const *flow_compiler::find_service_method(std::string const &method_name, std::set<std::string> *matches) const {
    MethodDescriptor const *last_match = nullptr;
    for(auto fdp: fdps) {
        std::string package = get_package(fdp);
        for(int sc = fdp->service_count(), s = 0; s < sc; ++s) {
            auto sdp = fdp->service(s);
            for(int mc = sdp->method_count(), m = 0; m < mc; ++m) {
                auto mdp = sdp->method(m);
                std::string full_name = mdp->service()->name() + "." + mdp->name();
                if(method_name == mdp->name() || 
                        method_name == full_name || 
                        method_name == package + "." + mdp->name() || 
                        method_name == package + "." + full_name) {

                    if(matches != nullptr) matches->insert(package+"."+full_name);
                    last_match = mdp;
                }
            }
        }
    }
    return last_match;
}
/**
 * The message can be spcified as [package.]message_type
 * The package is either the label defined with the package directive or the basename of the .proto file
 */
Descriptor const *flow_compiler::find_message(std::string const &dotted_name, std::set<std::string> *matches) const {
    Descriptor const *last_match = nullptr;
    for(auto fdp: fdps) {
        std::string package = get_package(fdp);
        for(int mc = fdp->message_type_count(), m = 0; m < mc; ++m) {
            auto mdp = fdp->message_type(m);
            if(dotted_name == mdp->name() || dotted_name == package + "." + mdp->name()) {
                if(matches != nullptr) matches->insert(package+"."+mdp->name());
                last_match = mdp;
            }
        }
    }
    return last_match;
}
/**
 * Return formatted list of all messages for message display purposes
 */
std::string flow_compiler::format_message_names(std::string const &sep, std::string const &last, std::string const &begin, std::string const &prefix, std::string const &suffix) const {
    std::set<std::string> l_names;
    for(auto fdp: fdps) 
        for(int mc = fdp->message_type_count(), m = 0; m < mc; ++m) {
             auto mdp = fdp->message_type(m);
             l_names.insert(mdp->full_name());
        }

    return join(l_names, sep, last, begin, prefix, suffix);
}
/**
 * Return formatted list of all methods for message display purposes
 */
std::string flow_compiler::format_full_name_methods(std::string const &sep, std::string const &last, std::string const &begin, std::string const &prefix, std::string const &suffix) const {
    std::set<std::string> l_names;
    for(auto fdp: fdps) 
        for(int sc = fdp->service_count(), s = 0; s < sc; ++s) {
            auto sdp = fdp->service(s);
            for(int mc = sdp->method_count(), m = 0; m < mc; ++m) {
                auto mdp = sdp->method(m);
                l_names.insert(mdp->service()->name() + "." + mdp->name());
            }

        }
    return join(l_names, sep, last, begin, prefix, suffix);
}
/**
 * Preserve all comments that have @text referneces in them
 */
void flow_compiler::add_comments(std::string const &comment, int token) {
    auto atp = comment.find_first_of("@");
    if(atp != std::string::npos) {
        auto eid = atp+1;
        for(; eid < comment.length(); ++eid) 
            if(comment[eid] != '_' && !isalnum(comment[eid]))
                break;
        if(eid != comment.length()) {
            std::string text = strip(comment.substr(eid), " \t\r\b\v\f\n");
            if(!text.empty())
                comments[comment.substr(atp+1, eid-atp-1)].push_back(text);
        }
    }
    std::string text = strip(comment, " \t\r\b\v\f\n");
    if(token > 0 && !text.empty() && isalnum(text[0])) {
        token_comment.push_back(std::make_pair(token, text));
    }
}
static std::map<std::string, int> keywords = {
    { "input", FTK_INPUT },
    { "output", FTK_OUTPUT },
    { "node", FTK_NODE },
    { "container", FTK_CONTAINER },
    { "entry", FTK_ENTRY },
    { "return", FTK_RETURN },
    { "headers", FTK_HEADERS },
    { "environment", FTK_ENVIRONMENT },
    { "mount", FTK_MOUNT }
};
int flow_compiler::parse() {
    io::ZeroCopyInputStream *zi = source_tree.Open(main_file);
    if(zi == nullptr) {
        pcerr.AddError(main_file, -1, 0, "can't read file");
        return 1;
    }
    int error_count = 0;
    ErrorPrinter ep(pcerr, main_file);
    io::Tokenizer tokenizer(zi, &ep);
    yyParser *fpp = (yyParser *) flow_parserAlloc(malloc);
    if(trace_on)
        flow_parserTrace(stderr, (char *) "flow: ");
    flow_token ftok;
    ftok.type = 0; // In case tokenizer fails

    std::string prev_trailing;
    std::vector<std::string> detached;
    std::string next_leading;
    bool get_previous = false;
    bool keep_parsing = true;

    while(get_previous || keep_parsing) {
        if(!get_previous) keep_parsing = tokenizer.NextWithComments(&prev_trailing, &detached, &next_leading);
        auto const &token = tokenizer.current();
        get_previous = false;
        ftok.line = token.line;
        ftok.column = token.column;
        ftok.end_column = token.end_column;
        switch(token.type) {
            case io::Tokenizer::TokenType::TYPE_START:
                ftok.type = 0;
                ++error_count;
                pcerr.AddError(main_file, ftok, "tokenizer internal error");
                break;
            case io::Tokenizer::TokenType::TYPE_END:
                ftok.type = 0;
                break;
            case io::Tokenizer::TokenType::TYPE_IDENTIFIER:
                ftok.type = FTK_ID;
                ftok.text = token.text;
                break;
            case io::Tokenizer::TokenType::TYPE_INTEGER:
                ftok.type = FTK_INTEGER;
                ftok.text = token.text;
                if(!io::Tokenizer::ParseInteger(token.text, UINT64_MAX, (uint64_t *) &ftok.integer_value)) {
                    // Integer overflow
                    ++error_count;
                    pcerr.AddError(main_file, ftok, "integer value overflow");
                }
                break;
            case io::Tokenizer::TokenType::TYPE_FLOAT:
                ftok.type = FTK_FLOAT;
                ftok.text = token.text;
                ftok.float_value = io::Tokenizer::ParseFloat(token.text);
                break;
            case io::Tokenizer::TokenType::TYPE_STRING:
                ftok.type = FTK_STRING;
                ftok.text.clear();
                io::Tokenizer::ParseString(token.text, &ftok.text);
                break;
            case io::Tokenizer::TokenType::TYPE_SYMBOL:
                ftok.type = FTK_SYMBOL;
                ftok.text = token.text;
                if(token.text.length() == 1) {
                    bool look_ahead = false;
                    switch(token.text[0]) {
                        case ';': ftok.type = FTK_SEMICOLON;  break;
                        case ',': ftok.type = FTK_COMMA; break;
                        case '.': ftok.type = FTK_DOT; break;
                        case ':': ftok.type = FTK_COLON; break;
                        case '#': ftok.type = FTK_HASH; break;
                        case '@': ftok.type = FTK_AT; break;
                        case '$': ftok.type = FTK_DOLLAR; break;
                        case '%': ftok.type = FTK_PERCENT; break;
                        case '?': ftok.type = FTK_QUESTION; break;
                        case '{': ftok.type = FTK_OPENBRA; break;
                        case '}': ftok.type = FTK_CLOSEBRA; break;
                        case '(': ftok.type = FTK_OPENPAR; break;
                        case ')': ftok.type = FTK_CLOSEPAR; break;
                        case '[': ftok.type = FTK_OPENSQB; break;
                        case ']': ftok.type = FTK_CLOSESQB; break;

                        case '^': ftok.type = FTK_CARET; break;
                        case '~': ftok.type = FTK_TILDA; break;
                        case '-': ftok.type = FTK_MINUS; break;
                        case '+': ftok.type = FTK_PLUS; break;
                        case '*': ftok.type = FTK_STAR; break;
                        case '/': ftok.type = FTK_SLASH; break;
                                  
                        case '=': ftok.type = FTK_EQUALS; look_ahead = true; break;
                        case '>': ftok.type = FTK_GT; look_ahead = true;  break;
                        case '<': ftok.type = FTK_LT; look_ahead = true; break;
                        case '&': ftok.type = FTK_AMP; look_ahead = true; break;
                        case '|': ftok.type = FTK_BAR; look_ahead = true; break;
                        case '!': ftok.type = FTK_BANG; look_ahead = true; break;
                        default: break;
                    }
                    
                    while(look_ahead) {
                        look_ahead = false;
                        keep_parsing = tokenizer.NextWithComments(&prev_trailing, &detached, &next_leading);
                        if(keep_parsing) {
                            get_previous = true;
                            auto const &next = tokenizer.current();
                            if(next.type == io::Tokenizer::TokenType::TYPE_SYMBOL && next.text.length() == 1) switch(next.text[0]) {
                                case '=': 
                                    get_previous = false;
                                    switch(ftok.type) {
                                        case FTK_EQUALS: ftok.type = FTK_EQ; break;
                                        case FTK_BANG: ftok.type = FTK_NE; break;
                                        case FTK_LT: ftok.type = FTK_LE;  break;
                                        case FTK_GT: ftok.type = FTK_GE; break;
                                        default: get_previous = true; break;
                                    }
                                    break;
                                case '&':
                                    if(ftok.type == FTK_AMP) { ftok.type = FTK_AND; get_previous = false; }
                                    break;
                                case '|': 
                                    if(ftok.type == FTK_BAR) { ftok.type = FTK_OR; get_previous = false; }
                                    break;
                                case '>': 
                                    get_previous = false;
                                    switch(ftok.type) {
                                        case FTK_GT: ftok.type = FTK_SHR; break;
                                        case FTK_LE: ftok.type = FTK_COMP; break;
                                        default: get_previous = true; break;
                                    }
                                    break;
                                case '<': 
                                    get_previous = false;
                                    switch(ftok.type) {
                                        case FTK_LT: ftok.type = FTK_SHL; break;
                                        default: get_previous = true; break;
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                break;
            default:
                ftok.type = 0;
                ++error_count;
                pcerr.AddError(main_file, ftok, "unknown token type");
        }
        if(error_count > 0) 
            break;
        int ntok = mk_node(ftok);
        std::string clean_prev_trailing = strip(prev_trailing);
        std::string clean_next_leading = strip(next_leading); 
        if(!clean_prev_trailing.empty() || !clean_next_leading.empty()) {
            std::string join;
            if(!clean_prev_trailing.empty() && !clean_next_leading.empty()) join = "\n";
            description.put(ntok, clean_prev_trailing + join + clean_next_leading);
        }

        add_comments(prev_trailing, ntok);
        for(auto const &s: detached) add_comments(s, ntok);
        add_comments(next_leading, ntok);
        flow_parser(fpp, ftok.type, ntok, this);
        auto top = store.back().type;
        if(top  == FTK_ACCEPT) 
            break;
        if(top == FTK_SYNTAX_ERROR) {
            pcerr.AddError(main_file, ftok, "syntax error");
            ++error_count;
            break;
        }
    }
    if(error_count == 0 && ftok.type != 0) 
        flow_parser(fpp, 0, -1, this);

    flow_parserFree(fpp, free);
    return error_count;
}
int flow_compiler::compile_string(std::string &str, int node, int node_type) {
    auto const &s = at(node);
    str = s.token.text;
    return s.type == node_type? 0: 1;
}
int flow_compiler::compile_id(std::string &str, int id_node) {
    return compile_string(str, id_node, FTK_ID);
}
int flow_compiler::compile_method(std::string &method, int mthd_node, int max_components) {
    auto const &mthd = at(mthd_node);
    bool error = mthd.type != FTK_dtid || mthd.children.size() == 0 || max_components > 0 && mthd.children.size() > max_components;
    if(error) return 1;
    for(int c: mthd.children) {
        auto const &id = at(c);
        assert(id.type == FTK_ID);          // syntax checked 
        if(!method.empty()) method += '.';
        method += id.token.text;
    }
    return 0;
}

static const std::map<int, int> operator_precedence = {
    {FTK_ID, 0},
    {FTK_fldx, 0},
    {FTK_HASH, 0},
    {FTK_DOLLAR, 0},
    {FTK_BANG, 0},
    {FTK_PLUS, 6},
    {FTK_MINUS, 6},
    {FTK_SLASH, 5},
    {FTK_STAR, 5},
    {FTK_PERCENT, 5},
    {FTK_COMP, 8},
    {FTK_EQ, 10},
    {FTK_NE, 10},
    {FTK_LE, 9},
    {FTK_GE, 9},
    {FTK_LT, 9},
    {FTK_GT, 9},
    {FTK_AND, 14}, 
    {FTK_OR, 15}, 
    {FTK_QUESTION, 15}, 
};
int get_operator_precedence(int node_type) {
    MASSERT(operator_precedence.find(node_type) != operator_precedence.end()) << "precedence not defined for node type " << node_type << "\n";
    return operator_precedence.find(node_type)->second;
}
int flow_compiler::fixup_vt(int node) {
    for(auto c: at(node).children)
        fixup_vt(c);
    if(at(node).type == FTK_fldr && !value_type.has(node) && at(node).children.size() > 1 && value_type.has(at(node).children[1]))
        value_type.put(node, value_type(at(node).children[1]));
    return 0;

}
struct function_info {
    int return_type;
    std::vector<int> arg_type;
    unsigned required_argc;
    int dim;        // Number of dimensions added or removed
    char const *help;
};
/** 
 * function signature table:
 *      return_type: Either a basic grpc compiler type or the negative of the index into the argument list.
 *      arg_type: Either a basic grpc compiler type or FTK_ACCEPT for any type. Negative values means the argument must be arepeated field.
 *      required_argc: The number of required arguments. The number of accepted arguments is the size of arg_type
 */
static const std::map<std::string, function_info> function_table = {
    // string substr(string s, int begin, int end)
    { "strslice",    { FTK_STRING,  { FTK_STRING, FTK_INTEGER, FTK_INTEGER }, 2, 0, "Returns the substring indiacted by the byte indices in the second and tird arguents.\n"}},
    { "substr",   { FTK_STRING,  { FTK_STRING, FTK_INTEGER, FTK_INTEGER }, 2, 0, "Returns the substring indicated by the utf-8 character indices in the second an third arguments.\n"}},
    // int length(string s)
    { "length",   { FTK_INTEGER, { FTK_STRING }, 1, 0, "Returns the number of utf-8 characters in the argument string.\n"}},
    { "size",  { FTK_INTEGER, { FTK_STRING }, 1, 0, "Returns the size of the string argument in bytes.\n"}},
    // string *trim(string s, string strip_chars)
    { "trim",     { FTK_STRING,  { FTK_STRING, FTK_STRING }, 1, 0, "Deletes all the characters in the second argument string from both ends of the first argument string.\nIf no second argument is given, white-space is deleted.\n"}},
    { "ltrim",    { FTK_STRING,  { FTK_STRING, FTK_STRING }, 1, 0, "Deletes all the characters in the second argument string from the beginning of the first argument string.\nIf no second argument is given, white-space is deleted.\n"}},
    { "rtrim",    { FTK_STRING,  { FTK_STRING, FTK_STRING }, 1, 0, "Deletes all the characters in the second argument string from the end of the first argument string.\nIf no second argument is given, white-space is deleted.\n"}},
    // string to*(string s)
    { "toupper",    { FTK_STRING,  { FTK_STRING }, 1, 0, "Converts all characters in the argument string to upper case (ASCII only).\n"}},
    { "tolower",    { FTK_STRING,  { FTK_STRING }, 1, 0, "Converts all characters in the argument string to lower case (ASCII only).\n"}},
    { "toid",       { FTK_STRING,  { FTK_STRING }, 1, 0, "Converts the string argument to a string that can be used as an identifier, by replacing all illegal character runs with an underscore('_').\n"}},
    { "tocname",    { FTK_STRING,  { FTK_STRING }, 1, 0, "Converts the string in the argument to a cannonical name by replacing all underscore('_') and space(' ') character runs with a dash('-').\n"}},
    { "camelize",   { FTK_STRING,  { FTK_STRING }, 1, 0, "Converts the string in the argument to a camel-cased identifier string.\n"}},
    { "decamelize", { FTK_STRING,  { FTK_STRING }, 1, 0, "Converts a camel-cased string to a readable string.\n"}},
    // string tr(string s, string match_list, string replace_list)
    { "tr",       { FTK_STRING,  { FTK_STRING, FTK_STRING, FTK_STRING }, 2, 0, "Returns the transformation of the string given as first argument by replacing all the characters in the second argument with the corresponding character in the third argument.\nIf there is no corresponding character, the character is deleted.\n"}},
    // string getenv(name, default_value)
    { "getenv",   { FTK_STRING,  { FTK_STRING, FTK_STRING }, 1, 0, "Returns the value of the environment variable named by the first argument. If the environment variable is missing, the value of the second argument is returned.\n"}},
    // string join(elements, separator, last_separator)
    { "join",     { FTK_STRING,  { -FTK_ACCEPT, FTK_STRING, FTK_STRING }, 1, -1, "Returns the concatenation of the elements of the repeated field, after converstion to string.\nThe second argument is used as separator, and the third, if given as the last separator.\n"}},
    { "split",    { -FTK_STRING, { FTK_STRING, FTK_STRING }, 1, 1, "Splits the first argument by any characters in the second argument\n"}},
    { "slice",    { -FTK_ACCEPT, { -FTK_ACCEPT, FTK_INTEGER, FTK_INTEGER }, 2, 0, "Returns a subsequence of the repeated field. Both begin and end indices can be negative.\n"}},
};

static std::string ftk2s(int ftk) {
    switch(ftk) {
        case FTK_STRING: return "string";
        case FTK_INTEGER: return "int";
        case -FTK_ACCEPT: return "repeated ANY";
        case -1: return "ANY";
        default: break;
    }
    return "?";
}

void show_builtin_help(std::ostream &out) {
    for(auto const &fe: function_table) {
        out << ftk2s(fe.second.return_type) << " \"~" << fe.first << "\"(";
        if(fe.second.arg_type.size() == fe.second.required_argc) {
            out << stru1::joint(fe.second.arg_type, ftk2s, ", ");
        } else {
            out << stru1::joint(fe.second.arg_type.begin(), fe.second.arg_type.begin()+fe.second.required_argc, ftk2s, ", ");
            out << "[";
            if(fe.second.required_argc > 0) out << ", ";
            out << stru1::joint(fe.second.arg_type.begin()+fe.second.required_argc, fe.second.arg_type.end(), ftk2s, ", ");
            out << "]";
        }
        out << ");\n";
        out << fe.second.help << "\n";
    }
}

int flow_compiler::compile_fldr(int fldr_node) {
    int error_count = 0;
    auto const &fldr = at(fldr_node);
    assert(fldr.type == FTK_fldr && fldr.children.size() > 0);

    for(unsigned n = 1, e = fldr.children.size(); n < e; ++n) {
        switch(at(fldr.children[n]).type) {
            case FTK_fldr:
                error_count += compile_fldr(fldr.children[n]);
                break;
            case FTK_INTEGER: case FTK_STRING: case FTK_FLOAT:
                value_type.put(fldr.children[n], at(fldr.children[n]).type);
                break;
            case FTK_fldx: {
                // this will be fixed up in the second pass
            } break;
            case FTK_enum: 

            break;

            default:
                MASSERT(false) << "did not set value type for: " << fldr.children[n] << ":" << at(fldr.children[n]).type << "\n";
        }
    }

    auto const &ff = at(fldr.children[0]);
    switch(ff.type) {
        case FTK_ID: {  // function call
            std::string fname(get_id(fldr.children[0])); 
            auto funp = function_table.find(fname);
            // check the function name
            if(funp == function_table.end()) {
                ++error_count;
                pcerr.AddError(main_file, at(fldr_node), sfmt() << "unknown function \"" << fname << "\"");
            } else if(funp->second.required_argc == funp->second.arg_type.size() && funp->second.required_argc + 1 != fldr.children.size()) {
                // check the number of arguments
                ++error_count;
                pcerr.AddError(main_file, at(fldr_node), sfmt() << "function \"" << fname << "\" takes " << funp->second.required_argc << " arguments but " << (fldr.children.size() - 1) << " were given");
            } else if(funp->second.required_argc != funp->second.arg_type.size() && funp->second.required_argc + 1 > fldr.children.size() || funp->second.arg_type.size() + 1 < fldr.children.size()) {
                ++error_count;
                pcerr.AddError(main_file, at(fldr_node), sfmt() << "function \"" << fname << "\" takes at least " << funp->second.required_argc << " and at most " << funp->second.arg_type.size() << " arguments but " << (fldr.children.size() - 1) << " were given");
            }
            if(funp->second.return_type < 0) 
                value_type.put(fldr_node, abs(value_type(fldr.children[abs(funp->second.return_type)])));
            else
                value_type.put(fldr_node, funp->second.return_type);
        } break;
        case FTK_HASH:
            assert(fldr.children.size() == 2);
            value_type.put(fldr_node, FTK_INTEGER);
            break;
        case FTK_BANG:
            assert(fldr.children.size() == 2);
            value_type.put(fldr_node, FTK_INTEGER);
            break;
            /*
        case FTK_DOLLAR:
            assert(fldr.children.size() == 2);
            value_type.put(fldr_node, FTK_STRING);
            break;
            */
        case FTK_PLUS: case FTK_MINUS: case FTK_SLASH: case FTK_STAR: case FTK_PERCENT: 
            assert(fldr.children.size() == 3);
            if(value_type.has(fldr.children[1])) 
                value_type.put(fldr_node, value_type(fldr.children[1]));
            if(field_descriptor.has(fldr.children[1]))
                field_descriptor.put(fldr_node, field_descriptor(fldr.children[1]));
            if(enum_descriptor.has(fldr.children[1]))
                enum_descriptor.put(fldr_node, enum_descriptor(fldr.children[1]));
            break;
        case FTK_COMP:
        case FTK_EQ: case FTK_NE: case FTK_LE: case FTK_GE: case FTK_LT: case FTK_GT:
        case FTK_AND: case FTK_OR:
            assert(fldr.children.size() == 3);
            value_type.put(fldr_node, FTK_INTEGER);
            break;
        case FTK_QUESTION:
            assert(fldr.children.size() == 4);
            if(value_type.has(fldr.children[2])) 
                value_type.put(fldr_node, value_type(fldr.children[2]));
            if(field_descriptor.has(fldr.children[2]))
                field_descriptor.put(fldr_node, field_descriptor(fldr.children[2]));
            if(enum_descriptor.has(fldr.children[2]))
                enum_descriptor.put(fldr_node, enum_descriptor(fldr.children[2]));
            break;
        default: 
            assert(false);
    }
    return error_count;
}
int flow_compiler::compile_fldm(int fldm_node, Descriptor const *dp) {
    int error_count = 0;
    auto const &fldm = at(fldm_node);
    assert(fldm.type == FTK_fldm);
    for(auto d: fldm.children) {
        auto const &fldd = at(d);
        assert(fldd.type == FTK_fldd && fldd.children.size() == 2);
        std::string id(get_id(fldd.children[0]));
        name.put(d, id);

        // Check that the descriptor actually has this field
        FieldDescriptor const *fidp = dp->FindFieldByName(id);
        if(fidp == nullptr) {
            ++error_count;
            pcerr.AddError(main_file, at(d), sfmt() << "\"" << dp->name() << "\" does not have a field called \"" << id << "\"");
            continue;
        }
        field_descriptor.put(d, fidp);
        auto left_type = grpc_type_to_ftk(fidp->type());

        switch(at(fldd.children[1]).type) {
            case FTK_fldm:
                if(left_type == FTK_fldm) {
                    Descriptor const *ldp = fidp->message_type();
                    message_descriptor.put(fldd.children[1], ldp); 
                    name.put(fldd.children[1], get_name(ldp));
                    error_count += compile_fldm(fldd.children[1], ldp);
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(d), sfmt() << "the field \"" << id << "\" in \"" << get_name(dp) << "\" is not of message type");
                }
            break;
            case FTK_fldx:
                // Postpone visiting the fldx field until the next time around
            break;
            case FTK_fldr:
                // Check for the right number of arguments, for valid identifiers and proper return type
                error_count += compile_fldr(fldd.children[1]);
            break;
            case FTK_INTEGER:
            case FTK_STRING:
            case FTK_FLOAT:
            case FTK_enum:
                if(left_type == FTK_fldm) {
                    ++error_count;
                    pcerr.AddError(main_file, at(d), sfmt() << "field \"" << id << "\" is of type message");
                } else if(left_type == FTK_fldm && at(fldd.children[1]).type == FTK_fldm) {

                } else if(left_type != at(fldd.children[1]).type) {
                    //pcerr.AddWarning(main_file, at(d), sfmt() << "value will be converted to \"" << node_name(left_type) << "\" before assignment to field \"" << get_name(dp) << "\"");
                }
            break;    
            case FTK_ID:
                if(left_type != FTK_fldm) {
                    ++error_count;
                    pcerr.AddError(main_file, at(d), sfmt() << "the field \"" << id << "\" in \"" << get_name(dp) << "\" is not of message type");
                } else {
                    pcerr.AddWarning(main_file, at(d), sfmt() << "got this \"" << id << "\" ...");
                }
            break;
            default:
                print_ast(std::cerr, fldd.children[1]);
                assert(false);
        }
    }
    return error_count;
}
/*
 * 2nd pass, this should be called after all nodes are compiled
 * Resolve message_descriptor attributes for node references
 */
int flow_compiler::compile_node_ref(int node) {
    std::string node_name(get_id(node));
    Descriptor const *dp = nullptr;
    if(node_name == input_label) {
        dp = input_dp;
    } else {
        auto nnp = named_blocks.find(node_name);
        if(nnp == named_blocks.end()) {
            pcerr.AddError(main_file, at(node), sfmt() << "unknown node \"" << node_name << "\"");
            return 1;
        }
        int blk = nnp->second.second;
        if(message_descriptor(blk) != nullptr)
            dp = message_descriptor(blk);
        else if(method_descriptor(blk) != nullptr) 
            dp = method_descriptor(blk)->output_type();
    }
    message_descriptor.put(node, dp);
    return 0;
}

int flow_compiler::update_noderef_dimension(int node) {
    if(dimension.has(node)) 
        return 0;

    std::string label = get_text(node);
    if(label == input_label) {
        dimension.put(node, 0);
        return 0;
    } 

    int error_count = 0;
    int nodes_dimension = -3;
    DEBUG_ENTER;
    std::set<int> node_set;
    for(int n: *this) if(type(n) == "node" && name(n) == label) {
        node_set.insert(n);
        if(!dimension.has(n)) {
            update_dimensions(n);
        }
        DEBUG_CHECK(" >>>> from " << node << " computed dimension for " << n << " this [" << label << "] is " << dimension(n));
        if(nodes_dimension < 0)
            nodes_dimension = dimension(n);
        if(nodes_dimension >= 0 && dimension(n) < 0) 
            dimension.update(n, nodes_dimension);
        if(nodes_dimension != dimension(n)) {
            pcerr.AddError(main_file, at(n), sfmt() << "dimension computed as \"" << dimension(n) << "\",  expected \"" << nodes_dimension << "\"");
            ++error_count;
        }
    }
    if(!dimension.has(node)) 
        dimension.put(node, nodes_dimension);

    if(nodes_dimension >= 0) {
        // If a node's dimension could not be computed, update it from the other nodes
        for(auto n: node_set) if(dimension(n) < 0) 
            dimension.update(n, nodes_dimension);
    } else {
        // If a node set's dimension could not be computed, generate errors
        for(auto n: node_set) {
            pcerr.AddError(main_file, at(n), sfmt() << "output size can not be determined for node");
            ++error_count;
        }
    }
    DEBUG_LEAVE;
    return error_count; 
}
/*
 * 2nd pass, this should be called after all the message types are set
 * Resolve dimensions for each data referencing node
 */
int flow_compiler::update_dimensions(int node) {
    if(dimension.has(node))
        return 0;
    int error_count = 0;
    auto const &children = at(node).children;

    switch(at(node).type) {
        case FTK_fldx: {
                error_count += update_noderef_dimension(children[0]);
                int xc = dimension(children[0]);
                for(int u = 1, e = children.size(); u < e; ++u) 
                    if(field_descriptor(children[u])->is_repeated())
                        ++xc;
                dimension.put(node, xc);
            }
            break;

        default:
            break;
    }

    for(int n: children) 
        error_count += update_dimensions(n);

    switch(at(node).type) {
        case FTK_blck:
            if(type(node) == "node" || type(node) == "entry") {
                int fonode = find_first(node, [this](int n) -> bool {
                    return contains(std::set<int>({FTK_OUTPUT, FTK_RETURN}), at(n).type);
                });

                if(fonode != 0) {
                    dimension.put(node, dimension(fonode));
                } else {
                    dimension.put(node, -2);
                }
                // FIXME this should look for the node's condition expression's dimension and 
                // that of all previous 
                
                int conode = find_first(node, [this](int n) ->  bool { return condition.has(n); });
                if(conode != 0 && dimension(node) >= 0 && dimension(condition(conode)) > dimension(node)) {
                    pcerr.AddError(main_file, at(node), sfmt() << "condition and node dimension mismatch");
                    ++error_count;
                }
                
            } 
            break;
        case FTK_fldd:
            dimension.put(node, dimension(children[1])+(field_descriptor(node)->is_repeated()? -1: 0));
            break;
        case FTK_fldm: {
                int xc = 0;
                for(auto c: children)
                    xc = std::max(xc, dimension(c));
                dimension.put(node, xc);
            }
            break;
        case FTK_OUTPUT: 
            dimension.put(node, dimension(children[1]));
            break;
        case FTK_RETURN: 
            dimension.put(node, dimension(children[0]));
            break;
        case FTK_fldr:
            switch(at(children[0]).type) {
                case FTK_ID: {  // function call
                    int dim = 0;
                    for(unsigned a = 0; a+1 < children.size(); ++a) 
                        dim = std::max(dim, dimension(children[a+1]));
                    auto funp = function_table.find(get_id(children[0]));
                    dimension.put(node, dim + funp->second.dim);
                } break;
                case FTK_HASH:
                    dimension.put(node, std::max(0, dimension(children[1])-1));
                    break;
                case FTK_BANG:
                    dimension.put(node, std::max(0, dimension(children[1])));
                    break;
                    /*
                case FTK_DOLLAR:
                    dimension.put(node, 0);
                    break;
                    */
                case FTK_COMP:
                case FTK_EQ: case FTK_NE: case FTK_LE: case FTK_GE: case FTK_LT: case FTK_GT:
                case FTK_AND: case FTK_OR:
                case FTK_PLUS: case FTK_MINUS: case FTK_SLASH: case FTK_STAR: case FTK_PERCENT: 
                    dimension.put(node, std::max(dimension(children[2]), dimension(children[1])));
                    break;
                case FTK_QUESTION:
                    dimension.put(node, std::max(dimension(children[2]), dimension(children[3])));
                    break;
                default: 
                    break;
            }
            break;
        default: 
            break;
    }
    return error_count;
}
/*
 * 2nd pass, this should be called after all nodes are compiled
 * Resolve fldx attributes
 */
int flow_compiler::compile_fldx(int node) {
    int error_count = 0;
    int fldx_node = node;
    switch(at(node).type) {
        // Fldx are expected at the right side of an fldd
        case FTK_fldd:
            assert(at(node).children.size() == 2);
            fldx_node = at(node).children[1];
            break;
        case FTK_fldx:
            break;
        default:
            for(int n: at(node).children) 
                error_count += compile_fldx(n);
            return error_count;
    }
    auto const &fldx = at(fldx_node);
    // Not a fldx node? Then keep visiting
    if(fldx.type != FTK_fldx)
        return compile_fldx(fldx_node);

    assert(fldx.children.size() > 0);
    // First field must be a node name
    error_count += compile_node_ref(fldx.children[0]);
    if(error_count > 0) return error_count;

    Descriptor const *dp = message_descriptor(fldx.children[0]);
    for(unsigned i = 1; i < fldx.children.size(); ++i) {
        int f= fldx.children[i];
        std::string id(get_id(f));
        FieldDescriptor const *fidp = dp->FindFieldByName(id);
        if(fidp == nullptr) {
            pcerr.AddError(main_file, at(f), sfmt() << "unknown field name \"" << id << "\"");
            return error_count + 1;
        }
        field_descriptor.put(f, fidp);
        if(is_message(fidp)) {
            dp = fidp->message_type(); 
        } else if(i + 1 <  fldx.children.size()) {
            pcerr.AddError(main_file, at(f), sfmt() << "the \"" << id << "\" field in \"" << get_name(dp) << "\" is not of message type");
            return error_count + 1;
        }
    }
    auto fdp = field_descriptor(fldx.children[fldx.children.size()-1]);
    if(fdp != nullptr) {
        int ftkt = grpc_type_to_ftk(fdp->type());
        if(ftkt == FTK_STRING || ftkt == FTK_INTEGER || ftkt == FTK_FLOAT)
            value_type.put(node, ftkt);
    }
    return error_count;
}
/*
 * 2nd pass, this should be called after all nodes are compiled 
 * Reslove attributes for rexp and oexp single node references
 */
int flow_compiler::compile_exp_id(int node) {
    int error_count = 0;
    switch(at(node).type) {
        case FTK_OUTPUT:
            if(at(node).children.size() == 2 && atc(node, 1).type == FTK_ID)
                error_count += compile_node_ref(at(node).children[1]);
            break;
        case FTK_RETURN:
            if(at(node).children.size() == 1 && atc(node, 0).type == FTK_ID)
                error_count += compile_node_ref(at(node).children[0]);
            break;
        default:
            for(auto c: at(node).children)
                error_count += compile_exp_id(c);
    }
    return error_count;
}
/*
 * 2nd pass, this should be called after all nodes are compiled 
 * Reslove enum references
 */
int flow_compiler::compile_id_ref(int node) {
    int error_count = 0;
    if(at(node).type != FTK_enum) {
        for(auto c: at(node).children) 
            error_count += compile_id_ref(c);
        return error_count;
    }
    std::string id_label(get_dotted_id(node));
    // Find all enums that match this id
    std::set<EnumValueDescriptor const *> matches;

    for(auto evd: enum_value_set) {
        if(evd->name() == id_label || evd->full_name() == id_label || stru1::ends_with(evd->full_name(), std::string(".")+id_label))  {
            if(matches.size() == 0) {
                enum_descriptor.put(node, evd);
            }
            matches.insert(evd);
        }
    }
    if(matches.size() == 0) {
        pcerr.AddError(main_file, at(node), sfmt() << "unknown enum label \"" << id_label << "\"");
        ++error_count;
    } else if(matches.size() > 1) {
        pcerr.AddError(main_file, at(node), sfmt() << "ambiguous enum label \"" << id_label << "\"");
        ++error_count;
    }
    return error_count;
}
// If exp_node is a valid pointer, the block is expected to contain one output (oexp) or return (rexp) definition 
int flow_compiler::compile_block(int blck_node, std::set<std::string> const &output_nvn, int *exp_node) {
    auto const &blck = at(blck_node);
    assert(blck.type == FTK_blck);
    int error_count = 0;
    int exp_node_count = 0;
    // Node values are uniqe so this will allocate a new block:
    block_data_t &block = block_store[blck_node];

    for(int elem_node: blck.children) {
        // syntax checked: 
        //      blck is a list of elem 
        //      each elem has 2 children
        //      the first child is always ID
        //      the second child is either a value (INTEGER, FLOAT or STRING), another [blck/lblk], or an [oexp/rexp] node
        auto const &elem = at(elem_node);
        assert(elem.type == FTK_elem && elem.children.size() == 2); 
        std::string elem_id = get_id(elem.children[0]);

        int value_node = elem.children[1];
        auto value_node_type = at(value_node).type;
        switch(value_node_type) {
            case FTK_blck:
                block.push_back(std::make_pair(elem_id, value_node));
                type.put(value_node, elem_id);
                error_count += compile_block(value_node);
                break;
            case FTK_STRING:
                if(elem_id == "error" && contains(output_nvn, elem_id)) {
                    if(exp_node != nullptr) {
                        *exp_node = value_node;
                        type.put(*exp_node, elem_id);
                    }
                }
            case FTK_INTEGER:
            case FTK_FLOAT:
                block.push_back(std::make_pair(elem_id, value_node));
                break;
            case FTK_lblk:
                // TODO allow anonymous mounts
                if(elem_id == "mount") {
                    block.push_back(std::make_pair(elem_id, value_node));
                    type.put(at(value_node).children[1], elem_id);
                    this->name.put(at(value_node).children[1], get_id(at(value_node).children[0]));

                    error_count += compile_block(at(value_node).children[1]);
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(value_node), sfmt() << "labeled block can't be of type \"" << elem_id << "\"");
                }
                break;
            case FTK_OUTPUT:
            case FTK_RETURN:
                // keep count of how many return/output definitions we have seen so far in exp_node_count
                if(exp_node_count == 0 && exp_node != nullptr && contains(output_nvn, elem_id)) {
                    *exp_node = elem.children[1]; ++exp_node_count;
                    type.put(*exp_node, elem_id);
                    // Store, but defer compilation of output/return expression until all the blocks are compiled
                    block.push_back(std::make_pair(elem_id, elem.children[1]));
                    if(value_node_type == FTK_OUTPUT) {
                        auto const &oexp = at(*exp_node);
                        std::string dotted_id = get_dotted_id(oexp.children[0]);
                       
                        if(elem_id == "output") {
                            auto md = check_method(dotted_id, oexp.children[0]);
                            method_descriptor.put(*exp_node, md);
                            if(md != nullptr) {
                                message_descriptor.put(*exp_node, md->output_type());
                                input_descriptor.put(*exp_node, md->input_type());
                            } else {
                                ++error_count;
                            }
                        } else if(elem_id == "return") {
                            auto md = check_message(dotted_id, oexp.children[0]);
                            method_descriptor.put(*exp_node, nullptr);
                            if(md != nullptr) {
                                message_descriptor.put(*exp_node, md);
                                input_descriptor.put(*exp_node, md);
                            } else {
                                ++error_count;
                            }
                        }
                    } else if(value_node_type == FTK_RETURN && type(blck_node) == "node") {
                        if(!message_descriptor(blck_node)) for(int n: *this) {
                            if(n == blck_node) 
                                break;
                            if(type(n) == "node" && name(n) == name(blck_node)) {
                                input_descriptor.copy(n, blck_node);
                                message_descriptor.copy(n, blck_node);
                                break;
                            }
                        }
                    }
                } else if(exp_node_count > 0) {
                    ++error_count;
                    pcerr.AddError(main_file, at(elem.children[1]), sfmt() << "redefinition of \"" << elem_id << "\" is not allowed");
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(elem.children[1]), sfmt() << "\"" << elem_id << "\" definition is not allowed here");
                }
                break;
            default:
                assert(false);
        }
    }
    return error_count;
}
/**
 * Check if the string refers to a valid method and return the descriptor. 
 * An error associated with 'error_node' is printed if 'error_node' is a valid node and 
 * if 'method' does not unambiguosly refer to a method.
 */
MethodDescriptor const *flow_compiler::check_method(std::string &method, int error_node)  {
    // Check if method is defined in any of the protos
    std::set<std::string> matches;
    MethodDescriptor const *mdp = find_service_method(method, &matches);
    if(matches.size() == 0) {
        if(error_node > 0) {
            pcerr.AddError(main_file, at(error_node), sfmt() << "service method not found: \"" << method << "\"");
            pcerr.AddNote(main_file, at(error_node), sfmt() << "defined service methods: " << format_full_name_methods(", ",  " and ", "", "\"", "\""));
        }
        mdp = nullptr;
    } else if(matches.size() > 1) {
        if(error_node > 0) 
            pcerr.AddError(main_file, at(error_node), sfmt() << "ambiguous method name \"" << method << "\" matches: "+join(matches, ", ", " and ", "", "\"", "\""));
        mdp = nullptr;
    } else if(mdp->client_streaming() || mdp->server_streaming()) {
        if(error_node > 0) 
            pcerr.AddError(main_file, at(error_node), "streaming is not supported at this time");
        mdp = nullptr;
    } else {
        method = *matches.begin();
    }
    return mdp;
}
/**
 * Check if the string refers to a valid message type and return the descriptor. 
 * An error associated with 'error_node' is printed if 'error_node' is a valid node and 
 * if 'dotted_id' does not unambiguosly refer to a message type.
 */
Descriptor const *flow_compiler::check_message(std::string &dotted_id, int error_node)  {
    // Check if method is defined in any of the protos
    std::set<std::string> matches;
    Descriptor const *mdp = find_message(dotted_id, &matches);
    if(matches.size() == 0) {
        if(error_node > 0) {
            pcerr.AddError(main_file, at(error_node), sfmt() << "message type not found: \"" << dotted_id << "\"");
            pcerr.AddNote(main_file, at(error_node), sfmt() << "defined message types: " << format_message_names(", ",  " and ", "", "\"", "\""));
        }
        mdp = nullptr;
    } else if(matches.size() > 1) {
        if(error_node > 0) 
            pcerr.AddError(main_file, at(error_node), sfmt() << "ambiguous message type name \"" << dotted_id << "\" matches: "+join(matches, ", ", " and ", "", "\"", "\""));
        mdp = nullptr;
    } else {
        dotted_id = *matches.begin();
    }
    return mdp;
}
int flow_compiler::compile_if_import(int stmt_node) {
    auto const &stmt = at(stmt_node);
    // Ignore unless an import statement
    if(stmt.type != FTK_IMPORT)
        return 0;

    assert(stmt.children.size() == 1);

    std::string filename;
    if(compile_string(filename, stmt.children[0])) {
        pcerr.AddError(main_file, at(stmt.children[0]), "expected filename string");
        return 1;
    }
    int ec = compile_proto(filename);
    if(ec > 0) 
        pcerr.AddError(main_file, stmt.token, sfmt() << "failed to import \"" << filename << "\"");
    return ec;
}
template <class SET>
static inline SET set_union(SET const &s1, SET const & s2) {
    SET s; 
    s.insert(s1.begin(), s1.end()); 
    s.insert(s2.begin(), s2.end()); 
    return s;
}
int flow_compiler::compile_stmt(int stmt_node) {
    int error_count = 0;
    int exp_node = 0, node_node = 0;
    auto const &stmt = at(stmt_node);
    assert(stmt.children.size() > 0);
    std::string statement;
    compile_id(statement, stmt.children[0]); // sytax checked for ID already

    switch(stmt.type) {
        case FTK_CONTAINER: case FTK_NODE: 
            {
                std::string node_name;
                if(compile_method(node_name, stmt.children[1], 1)) {
                    pcerr.AddError(main_file, at(stmt.children[1]), std::string("expected \"")+statement+"\" name");
                    return 1;
                }
                node_node = stmt.children[2];

                name.put(node_node, node_name);
                type.put(node_node, statement);
                if(description.has(stmt.children[0]))
                    description.update(node_node, description(stmt.children[0]));

                // Look for the action entry in the node block
                if(compile_block(node_node, {"output", "return", "error"}, stmt.type == FTK_NODE? &exp_node: nullptr)) {
                    //pcerr.AddError(main_file, at(stmt.children[2]), sfmt() << "invalid \"" << statement << "\" parameter definition block");
                    return 1;
                }
                if(stmt.type == FTK_NODE) {
                    if(exp_node != 0) {
                        // move attributes to the node 
                        method_descriptor.copy(exp_node, node_node);
                        input_descriptor.copy(exp_node, node_node);
                        message_descriptor.copy(exp_node, node_node);
                        if(stmt.children.size() > 3) {
                            condition.put(node_node, stmt.children[3]);
                            if(at(stmt.children[3]).type == FTK_fldr) 
                                error_count += compile_fldr(stmt.children[3]);
                        }
                    }
                }
                // Check this node against other nodes with the same name
                for(int visited: set_union(node_set, container_set)) if(name(visited) == node_name) {

                    if(type(visited) != type(node_node)) {
                        pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "redefinition of \"" << node_name << "\" with a different type");
                        pcerr.AddNote(main_file, at(visited), sfmt() << "previously defined here as \"" << type(visited) << "\"");
                        return 1;
                    }
                    if(type(node_node) == "container") {
                        pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "redefinition of container \"" << node_name << "\"");
                        pcerr.AddNote(main_file, at(visited), "previously defined here");
                        return 1;
                    }
                    // visited and node_node are both of type node
                    if(!condition.has(visited) && !condition.has(node_node)) {
                        // Can't have more than one default node
                        pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "redefinition of node \"" << node_name << "\"");
                        pcerr.AddNote(main_file, at(visited), "previously defined here");
                        return 1;
                    }
                    if(message_descriptor.has(visited) && message_descriptor.has(node_node) &&
                            message_descriptor(visited) != message_descriptor(node_node)) {
                        pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "node \"" << node_name << "\" redefined with a different output type");
                        pcerr.AddNote(main_file, at(visited), "previously defined here");
                        return 1;
                    }
                } 
                if(stmt.type == FTK_NODE)
                    node_set.insert(node_node);
                else
                    container_set.insert(node_node);

                // find the first node with this name
                auto nep = named_blocks_w.find(node_name);
                if(nep == named_blocks.end()) {
                    // Quick access to the block node id
                    named_blocks_w[node_name] = std::make_pair(statement, node_node);
                } else {
                    // Keep in the table the node without condition but with an output type
                    if((!condition.has(node_node) || !method_descriptor.has(nep->second.second)) && method_descriptor.has(node_node))
                        nep->second.second = node_node;
                }
            } break;
        case FTK_ENTRY: 
            {
                std::string method; 
                if(compile_method(method, stmt.children[1])) {
                    pcerr.AddError(main_file, at(stmt.children[1]), "expected method name");
                    return 1;
                }
                node_node = stmt.children[2];
                type.put(node_node, statement);
                if(stmt.children.size() != 3 || compile_block(node_node, {"return"}, &exp_node)) {
                    pcerr.AddError(main_file, at(node_node), "parameter definition block expected");
                    return 1;
                }
                if(exp_node == 0) {
                    pcerr.AddError(main_file, at(node_node), "entry is missing return definition");
                    return 1;
                }
                // check if method is defined in any of the protos
                MethodDescriptor const *mdp = check_method(method, stmt.children[1]);
                if(method_descriptor.has(exp_node)) {
                    // TODO a redudant message type should be accepted here
                    pcerr.AddError(main_file, at(exp_node), "return definition must omit message type");
                    return 1;
                }
                method_descriptor.put(exp_node, mdp);
                method_descriptor.copy(exp_node, node_node);
                input_descriptor.copy(exp_node, node_node);
                message_descriptor.copy(exp_node, node_node);
                if(description.has(stmt.children[0]))
                    description.update(node_node, description(stmt.children[0]));
                if(mdp == nullptr) 
                    return 1;
                if(contains(named_blocks, method)) {
                    pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "redefinition of \"" << method << "\"");
                    return 1;
                }
                // quick access to the block node id 
                named_blocks_w[method] = std::make_pair(statement, stmt.children[2]);
                entry_set.insert(stmt.children[2]);
                // all entries must have the same input type
                if(input_dp == nullptr) {
                    input_dp = mdp->input_type();
                } else {
                    if(input_dp != mdp->input_type()) { 
                        error_count += 1;
                        pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "input type must be the same for all entries");
                    }
                }
            } break;
        case FTK_DEFINE:
            if(statement == "package") {
                std::string package_name;
                if(compile_id(package_name, stmt.children[1])) {
                    pcerr.AddError(main_file, at(stmt.children[1]), "expected package name id");
                    error_count += 1;
                }
            } else if(statement == "repository") {
                std::string repository;
                if(compile_string(repository, stmt.children[1])) {
                    pcerr.AddError(main_file, at(stmt.children[1]), "expected repository path string");
                    error_count += 1;
                }
                default_repository = repository;
            } else if(statement == "image_pull_secret") {
                std::string secret;
                if(compile_string(secret, stmt.children[1])) {
                    pcerr.AddError(main_file, at(stmt.children[1]), "expected image pull secret name");
                    error_count += 1;
                }
                image_pull_secrets.insert(secret);
            } else if(statement == "port") {
                if(at(stmt.children[1]).type != FTK_STRING && at(stmt.children[1]).type != FTK_INTEGER) {
                    pcerr.AddError(main_file, at(stmt.children[1]), "port value expected");
                    error_count += 1;
                }
                base_port = get_integer(stmt.children[1]);
            }
            break;
        default: 
            break;
    }
    
    // If everything went well, visit the exp node
    if(error_count == 0 && exp_node != 0 && type.has(exp_node) && (type(exp_node) == "return" || type(exp_node) == "output")) {
        auto const *mp = method_descriptor(exp_node);
        auto const *d = message_descriptor(exp_node);
        if(mp != nullptr) 
            d = type(exp_node) == "return"? mp->output_type(): mp->input_type();
        if(d == nullptr) 
            d = message_descriptor(node_node);
        if(d == nullptr) {
            pcerr.AddError(main_file, at(exp_node), "unknown return message type");
            error_count += 1;
        }
        // Set the method descriptor attribute 
        for(auto c: at(exp_node).children) switch(at(c).type) {
            case FTK_fldm:
                message_descriptor.put(c, d); 
                if(d != nullptr) {
                    this->name.put(c, get_name(d));
                    error_count += compile_fldm(c, d);
                }
                break;
            case FTK_ID:
                // This is a node reference so postpone until the second pass
                break;
            default:
                break;
        }
    }
    return error_count;
}
void flow_compiler::get_fldm_node_refs(std::map<int, std::set<std::string>> &noset, int fldm_node) {
    for(int fldd_node: at(fldm_node).children) switch(atc(fldd_node, 1).type) {
        case FTK_fldm:
            get_fldm_node_refs(noset, at(fldd_node).children[1]);
            break;
        case FTK_fldx:
            if(get_id(atc(fldd_node, 1).children[0]) != input_label) 
                noset[named_blocks.find(get_id(atc(fldd_node, 1).children[0]))->second.second].insert(get_dotted_id(at(fldd_node).children[1], 1));
            else
                noset[0].insert(get_dotted_id(at(fldd_node).children[1], 1));
            break;
        default:
            break;
    }
}
/**
 * Return a map node->[field-set] referenced by the conditional expression of node bexp_node
 */
void flow_compiler::get_bexp_node_refs(std::map<int, std::set<std::string>> &noset, int bexp_node) {
    switch(at(bexp_node).type) {
        case FTK_fldx:
            if(get_id(at(bexp_node).children[0]) != input_label) 
                noset[named_blocks.find(get_id(at(bexp_node).children[0]))->second.second].insert(get_dotted_id(bexp_node, 1));
            else 
                noset[0].insert(get_dotted_id(bexp_node, 1));
            break;
        default:
            for(auto c: at(bexp_node).children) 
                get_bexp_node_refs(noset, c);
            break;
    }
}
// Get all the nodes of the same name 
std::vector<int> flow_compiler::all_nodes(std::string const &node_name) const {
    std::vector<int> all;
    for(auto n: node_set) if(name(n) == node_name && condition.has(n))
        all.push_back(n);
    for(auto n: node_set) if(name(n) == node_name && !condition.has(n))
        all.push_back(n);
    MASSERT(all.size() > 0) << "\"" << node_name << "\" is not a node name\n";
    return all;
}
// Get all the nodes of the same name
std::vector<int> flow_compiler::all_nodes(int node) const {
    MASSERT(contains(node_set, node)) << node << " is not a node\n";
    return all_nodes(name(node));
}
// Grab the nodes referenced by <blck_node>. 
// <type> can be either 1 or references in the condition, 2 for references in the node body or 0 for both.
// The input node will also be added.
std::map<int, std::set<std::string>> &flow_compiler::get_node_refs(std::map<int, std::set<std::string>> &noset, int blck_node, int type) {
    if(blck_node == 0) 
        return noset;

    // condition expression references
    if((type == 0 || type == 1) && this->type(blck_node) == "node") {
        // add the conditions of all previous nodes
        for(auto n: all_nodes(blck_node)) if(condition.has(n) && n <= blck_node) 
            get_bexp_node_refs(noset, condition(n));
    }
    // references in the body
    if(type == 0 || type == 2) {
        for(auto p = begin(blck_node), e = end(); p != e; ++p) {
            int n = *p;
            if(at(n).type == FTK_OUTPUT || at(n).type == FTK_RETURN || at(n).type == FTK_HEADERS) 
                get_bexp_node_refs(noset, n);
        }
    }
    return noset;
}
int flow_compiler::build_flow_graph(int blk_node) {
    int error_count = 0;

    // All the nodes reachable from the entry, including the entry itself
    std::set<int> used_nodes; 

    // initialze a processing buffer (stack) with the entry
    // process each node in the buffer until the buffer is empty
    for(std::vector<int> todo(&blk_node, &blk_node+1); todo.size() > 0;) {
        int cur_node = todo.back(); todo.pop_back();
        if(contains(used_nodes, cur_node))
            continue;
        // add the current node...
        used_nodes.insert(cur_node);
        // stack all the aliases unless already processed
        if(cur_node != 0 && name.has(cur_node) && !name(cur_node).empty()) for(auto n: all_nodes(name(cur_node))) {
            if(!contains(used_nodes, n)) 
                todo.push_back(n);
        }
        // also push all the nodes referenced by the current node in the stack
        std::map<int, std::set<std::string>> noset;
        get_node_refs(noset, cur_node, 0);
        for(auto const &ns: noset) 
            if(!contains(used_nodes, ns.first)) 
                todo.push_back(ns.first);
    }

    auto node_count = used_nodes.size();

    // Adjacency matrix for the graph
    std::vector<std::vector<bool>> adjmat(node_count, std::vector<bool>(node_count, false));
    // Map from matrix index to the actual node id
    std::vector<int> xton(used_nodes.begin(), used_nodes.end());

    if(node_count > 0) {
        // Map from node id to adjacency matrix index
        std::map<int, unsigned> ntox;

        for(unsigned x = 0; x != node_count; ++x) ntox[xton[x]] = x;
        for(auto const &nx: ntox) {
            std::map<int, std::set<std::string>> noset;
            get_node_refs(noset, nx.first, 0);
            for(auto const &ns: noset) {
                adjmat[nx.second][ntox[ns.first]] = true;
                if(ns.first != 0 && !name(ns.first).empty()) for(auto n: all_nodes(name(ns.first)))
                    adjmat[nx.second][ntox[n]] = true;
            }
        }
    }

    // The flow graph is represented as list of sets of nodes:
    // Each set is connected only to nodes from the sets previous in the list
    // i.e. all nodes in S(i) are connected to nodes in S(0)|...|S(i-1)
    std::vector<std::set<int>> &graph = flow_graph[blk_node];

    // The set of nodes that we know all the conections for
    std::set<int> solved;

    // The input has no connections:
    solved.insert(0);

    // The maximum number of sets mx-2 is reached when every node is in a set by itself.
    // Each iteration attempts to build a stage (set). If a stage is empty we are either
    // done or the graph is badly connected.
    
    for(unsigned i = 0, mx = node_count; i != mx; ++i) {
        graph.push_back(std::set<int>());

        for(unsigned x = 0; x != mx; ++x) 
            if(!contains(solved, xton[x])) {
                bool s = true;
                for(unsigned y = 0; y != mx; ++y) 
                    if(adjmat[x][y] && !contains(solved, xton[y])) {
                        s = false;
                        break;
                    }
                if(s) graph.back().insert(xton[x]);
            }
        if(graph.back().size() == 0) 
            break;

        if(contains(graph.back(), blk_node)) {
            solved.insert(blk_node);
            graph.pop_back();
            // Successfully finised
            break;
        }
        for(auto n: graph.back()) 
            solved.insert(n);
    }
    if(contains(solved, blk_node)) 
        return 0;
    
    auto a1 = adjmat;
    std::set<int> circular;
    for(unsigned i = 0; i + 3 < node_count; ++i) {
#if 0
        std::cerr << "distance: " << 1+i << "\n";
        std::cerr << "    " << xton << "\n";
        std::cerr << "----------------------------------------\n";
        for(unsigned p = 0; p != node_count; ++p) 
            std::cerr << xton[p] << "  " << adjmat[p] << "\n";
        std::cerr << "----------------------------------------\n";
#endif
        for(unsigned x = 0; x < node_count; ++x) 
            if(adjmat[x][x] && !contains(circular, xton[x])) {
                circular.insert(xton[x]);
                if(i == 0) 
                    pcerr.AddError(main_file, at(xton[x]), sfmt() << "node \"" << name(xton[x]) << "\" references itself");
                else
                    pcerr.AddError(main_file, at(xton[x]), sfmt() << "circular reference of node \"" << name(xton[x]) << "\"");
                ++error_count;
            }
        auto aip = adjmat;
        for(unsigned x = 0; x < node_count; ++x) 
            for(unsigned y = 0; y < node_count; ++y) {
                bool b = false;
                for(unsigned j = 0; j < node_count; ++j)
                    b = b or (aip[x][j] and a1[j][y]);
                adjmat[x][y] = b;
            }

    }
    if(error_count == 0) {
        pcerr.AddError(main_file, at(blk_node), sfmt() << "failed to construct graph for \"" << method_descriptor(blk_node)->full_name() << "\" entry");
        ++error_count;
    }
    return error_count;
}
// This is used only to compare INDX 
int flow_compiler::fop_compare(fop const &left, fop const &right) const {
    int x = left.code - right.code;
    if(x != 0) return x;
    x = left.arg1.compare(right.arg1);
    if(x != 0) return x;
    x = left.arg2.compare(right.arg2);
    if(x != 0) return x;
    x = (int) left.arg.size() - (int) right.arg.size();
    if(left.arg.size() == 0 || x != 0) return x;
    if(message_descriptor(left.arg[0]) != message_descriptor(right.arg[0])) 
        return message_descriptor(left.arg[0]) > message_descriptor(right.arg[0])?1 : -1;
    for(unsigned f = 1; f < left.arg.size(); ++f) {
        if(field_descriptor(left.arg[f]) != field_descriptor(right.arg[f])) 
            return field_descriptor(left.arg[f]) > field_descriptor(right.arg[f])?1 : -1;
    }
    return 0;
}
/*
template <class ITER>
static 
// Look for the first NSET within the current node or entry scope
ITER find_next_xcode(ITER ip, ITER ie) {
    int skip = 1;
    while(ip != ie && ip->code != BNOD && ip->code != BPRP) {
        switch(ip->code) {
            case LOOP:  --skip; break;
            case ELP:   ++skip; break;
            case NSET:  skip = 0; break;
            default: break;
        }
        if(skip == 0) break;
        ++ip;
    }
    return ip;
}
*/
/** Encapsulation of the type information for left and right values
 */
struct lrv_descriptor {
    int value_type;
    Descriptor const *dp;
    FieldDescriptor const *fp;

    lrv_descriptor(int itype): 
        value_type(itype), dp(nullptr), fp(nullptr) {}

    lrv_descriptor(lrv_descriptor const &lvd, FieldDescriptor const *ifp):
        value_type(lvd.value_type), dp(lvd.dp), fp(ifp) {}

    lrv_descriptor(Descriptor const *idp, FieldDescriptor const *ifp=nullptr):
        value_type(0),  dp(idp), fp(ifp) {}

    bool is_field() const { return fp != nullptr; }
    bool is_message() const { return is_field()? ::is_message(fp): (dp != nullptr); }
    bool is_repeated() const { return is_field() && fp->is_repeated(); }
    int type() const {
        if(is_field()) return grpc_type_to_ftk(fp->type());
        if(dp != nullptr) return FTK_fldm;
        return value_type;
    }
    int grpc_type() const {
        if(is_field()) return (int) fp->type();
        if(dp != nullptr) return (int) google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE;
        return value_type;
    }
    int t_size() const {
        if(is_field()) return get_grpc_type_size(fp);
        return 100;
    }
    std::string type_name() const {
        if(is_field()) return get_full_type_name(fp);
        if(dp != nullptr) return dp->full_name();
        return node_name(value_type); 
    }
    std::string grpc_type_name() const {
        if(is_field()) return get_grpc_type_name(fp);
        if(dp != nullptr) return dp->full_name();
        return "undefined"; 
    }
    google::protobuf::EnumDescriptor const *enum_descriptor() const {
        if(!is_field()) return nullptr;
        return fp->enum_type();
    }
};
/**
 * Returns: 
 * 1 if right can be assigned to left without conversion, 
 * 2 if right can be copied onto left
 * 3 if it can be converted without data loss, 
 * 4 if the conversion could cause data loss, 
 * 0 if conversion is not possible.
 */
int flow_compiler::check_assign(int error_node, lrv_descriptor const &left, lrv_descriptor const &right) {
    int check = 0;
    if(left.is_message() || right.is_message()) {
        if(left.type_name() == right.type_name()) 
            check = 2;
        else 
            pcerr.AddError(main_file, at(error_node), sfmt() << "cannot assign \"" << right.type_name() << "\" to left value of type \"" << left.type_name() << "\"");
    } else if(left.type() == right.type()) {
        check = 1; 
    } else if(left.type() == FTK_STRING || left.grpc_type_name() == "bool" || left.t_size() > right.t_size()) {
        check = 3;
    } else if(left.type() == FTK_enum) {
        pcerr.AddError(main_file, at(error_node), sfmt() << "cannont convert from \"" << right.type_name() << "\" to \"" << left.enum_descriptor()->full_name() << "\"");
    } else {
        pcerr.AddWarning(main_file, at(error_node), sfmt() << "conversion from \"" << right.type_name() << "\" to \"" << left.type_name() << "\" could cause data loss");
        check = 4;
    }
    return check;
}
static 
op get_pconv_op(int l_type, int r_type) {
    switch(l_type) {
        case FTK_STRING:
            switch(r_type) {
                case FTK_INTEGER:
                    return COIS;
                case FTK_FLOAT:
                    return COFS;
                default:
                    break;
            }
            break;
        case FTK_INTEGER:
            switch(r_type) {
                case FTK_STRING:
                    return COSI;
                case FTK_FLOAT:
                    return COFI;
                default:
                    break;
            }
            break;
        case FTK_FLOAT:
            switch(r_type) {
                case FTK_STRING:
                    return COSF;
                case FTK_INTEGER:
                    return COIF;
                default:
                    break;
            }
        default:
            break;
    }
    return NOP;
}
static 
std::string type_name(int t) {
    switch(t) {
        case FTK_STRING:
            return "FTK_STRING";
        case FTK_INTEGER:
            return "FTK_INTEGER";
        case FTK_FLOAT:
            return "FTK_INTEGER";
        default:
            return std::to_string(t);
    }
}
/*
static
op get_pconv_op(int l_type, int r_type) {
    auto c = get_pconv_op_i(l_type, r_type);
    std::cerr << "~~~~ get_pconv_op(" <<  type_name(l_type) << ", " << type_name(r_type) << "): "  << op_name(c) << "\n";
    return c;
}
*/
static 
op get_conv_op(int r_type, int l_type, int r_grpc_type, int l_grpc_type) {
    switch(r_type) {
        case FTK_INTEGER:
            switch(l_type) {
                case FTK_INTEGER:
                    if(l_grpc_type == google::protobuf::FieldDescriptor::Type::TYPE_BOOL) 
                        return COIB;
                    if(l_grpc_type != r_grpc_type) return COII;
                    return NOP;
                case FTK_FLOAT:
                    return COIF;
                case FTK_STRING:
                    return COIS;
                default:
                    break;
            } 
            break;
        case FTK_FLOAT:
            switch(l_type) {
                case FTK_INTEGER:
                    if(l_grpc_type == google::protobuf::FieldDescriptor::Type::TYPE_BOOL) 
                        return COFB;
                    return COFI;
                case FTK_FLOAT:
                    if(l_grpc_type != r_grpc_type) return COFF;
                    return NOP;
                case FTK_STRING:
                    return COFS;
                default:
                    break;
            } 
            break;
        case FTK_STRING:
            switch(l_type) {
                case FTK_INTEGER:
                    if(l_grpc_type == google::protobuf::FieldDescriptor::Type::TYPE_BOOL) 
                        return COSB;
                    return COSI;
                case FTK_FLOAT:
                    return COSF;
                case FTK_STRING:
                    return NOP;
                default:
                    break;
            }
            break;
        case FTK_enum:
            switch(l_type) {
                case FTK_INTEGER:
                    if(l_grpc_type == google::protobuf::FieldDescriptor::Type::TYPE_BOOL) 
                        return COEB;
                    return COEI;
                case FTK_FLOAT:
                    return COEF;
                case FTK_STRING:
                    return COES;
                case FTK_enum:
                    return COEE;
                default:
                    break;
            }
            break;
        default: 
            std::cerr << "Asked to convert: " << l_type << " to " << r_type << " (" << l_grpc_type << ", " << r_grpc_type << ") STRING " << FTK_STRING << " enum: " << FTK_enum << " fldx: " << FTK_fldx << " dtid: " << FTK_dtid << "\n";
    }
    return NOP;
}
/**
 * Return all fldx ast nodes that are referenced from expr_node and their final dimension
 */
int flow_compiler::get_field_refs(std::set<std::pair<int, int>> &refs, int expr_node, int lv_dim) const {
    int error_count = 0;
    auto const &node = at(expr_node);
    switch(node.type) {
        case FTK_fldr: {
            switch(at(node.children[0]).type) {
                case FTK_ID: {  // function call
                    if(get_id(node.children[0]) == "join")
                        ++lv_dim;
                } break;
                case FTK_HASH:
                    ++lv_dim;         
                    break;
                default: 
                    break;
            }
            for(unsigned a = 1; a < node.children.size(); ++a) 
                error_count += get_field_refs(refs, node.children[a], lv_dim);
        } break;
        case FTK_fldm: {
            for(int fldd_node: node.children) 
                error_count += get_field_refs(refs, at(fldd_node).children[1], lv_dim + (field_descriptor(fldd_node)->is_repeated()? 1: 0));
            
        } break;
        case FTK_fldx: {
            refs.insert(std::make_pair(expr_node, dimension(expr_node) - lv_dim));
        } break;
        default:
            for(auto n: node.children) 
                error_count += get_field_refs(refs, n, lv_dim);
          break;
    }
    return error_count;
}
/**
 * Return a mangled field name with placehodlers for indices
 */
std::string flow_compiler::fldx_mname(int fldx_node, int context_dim) const {
    auto const &fields = at(fldx_node).children;
    int rvn = get_id(fields[0]) == input_label? 0: named_blocks.find(get_id(fields[0]))->second.second;
    auto const rv_name = get_id(fields[0]) == input_label? cs_name("", 0): cs_name("RS", name(rvn));
    std::string fa_name = rv_name;
    int xc = context_dim;
    for(auto i = 0, e = dimension(fields[0]); i < e; ++i) {
        if(--xc <= 0)
            break;
        fa_name +=  ":";
    }
    if(xc > 0) for(unsigned i = 1; i < fields.size(); ++i) {
        fa_name += ".";
        fa_name += get_id(fields[i]);
        auto fid = field_descriptor(fields[i]);
        if(fid->is_repeated()) {
            if(--xc <= 0)
                break;
            fa_name += "*";
        } else {
            fa_name += "+";
        }
    }
    return fa_name;
}
int flow_compiler::encode_expression(int fldr_node, int expected_type, int dim_change) {
    int error_count = 0;
    auto const &fields = at(fldr_node).children;
    auto coop = get_pconv_op(expected_type, value_type(fldr_node));
    int op_precedence = -1;
    switch(at(fldr_node).type) {
        case FTK_fldr: 
            if(at(fields[0]).type == FTK_ID) {
                auto funp = function_table.find(get_id(fields[0]));
                for(unsigned a = 0; a+1 < fields.size(); ++a) {
                    if(funp->second.arg_type[a] < 0) 
                        icode.push_back(fop(CLLS, a));
                    int evt = abs(funp->second.arg_type[a]);
                    error_count += encode_expression(fields[a+1], (evt == FTK_ACCEPT? 0: evt), funp->second.dim);
                    // FIXME: needs review 
                    if(funp->second.arg_type[a] < 0) 
                        icode.push_back(fop(DACC, funp->second.dim));
                }
            }
            MASSERT(operator_precedence.find(at(fields[0]).type) != operator_precedence.end()) << "precedence not defined for type " << at(fields[0]).type << ", at " << at(fields[0]).type << "\n";
            op_precedence = operator_precedence.find(at(fields[0]).type)->second;
            switch(at(fields[0]).type) {
                case FTK_ID: 
                    icode.push_back(fop(FUNC, get_id(fields[0]), fields.size()-1, op_precedence));
                    break;
                case FTK_HASH:
                    if(dimension(fields[1]) == 0) {
                        icode.push_back(fop(RVC, "1", fldr_node, google::protobuf::FieldDescriptor::Type::TYPE_INT64));
                    } else {
                        std::string name = fldx_mname(fields[1], 1000);
                        icode.push_back(fop(SVF, name.substr(0, name.length()-1), dimension(at(fields[1]).children[0])));
                    }
                    break;
                case FTK_BANG:
                    error_count += encode_expression(fields[1], FTK_INTEGER, dim_change);
                    icode.push_back(fop(IOP, "!", 1, op_precedence));
                    break;
                case FTK_PLUS: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "+", 2, op_precedence));
                    break;
                case FTK_MINUS: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "-", 2, op_precedence));
                    break;
                case FTK_SLASH: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "/", 2, op_precedence));
                    break;
                case FTK_STAR: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "*", 2, op_precedence));
                    break;
                case FTK_PERCENT: 
                    error_count += encode_expression(fields[1], FTK_INTEGER, dim_change);
                    error_count += encode_expression(fields[2], FTK_INTEGER, dim_change);
                    icode.push_back(fop(IOP, "%", 2, op_precedence));
                    break;
                case FTK_COMP:
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "<=>", 2, op_precedence));
                    break;
                case FTK_EQ: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "==", 2, op_precedence));
                    break;
                case FTK_NE: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "!=", 2, op_precedence));
                    break;
                case FTK_LE: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "<=", 2, op_precedence));
                    break;
                case FTK_GE: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, ">=", 2, op_precedence));
                    break;
                case FTK_LT: 
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, "<", 2, op_precedence));
                    break;
                case FTK_GT:
                    error_count += encode_expression(fields[1], 0, dim_change);
                    error_count += encode_expression(fields[2], value_type(fields[1]), dim_change);
                    icode.push_back(fop(IOP, ">", 2, op_precedence));
                    break;
                case FTK_AND: 
                    error_count += encode_expression(fields[1], FTK_INTEGER, dim_change);
                    error_count += encode_expression(fields[2], FTK_INTEGER, dim_change);
                    icode.push_back(fop(IOP, "&&", 2, op_precedence));
                    break;
                case FTK_OR:
                    error_count += encode_expression(fields[1], FTK_INTEGER, dim_change);
                    error_count += encode_expression(fields[2], FTK_INTEGER, dim_change);
                    icode.push_back(fop(IOP, "||", 2, op_precedence));
                    break;
                case FTK_QUESTION:
                    error_count += encode_expression(fields[1], FTK_INTEGER, dim_change);
                    error_count += encode_expression(fields[2], 0, dim_change);
                    error_count += encode_expression(fields[3], value_type(fields[2]), dim_change);
                    icode.push_back(fop(IOP, "?:", 3, op_precedence));
                    break;
                break;
            }
            break;
        case FTK_fldx:
            icode.push_back(fop(RVF, fldx_mname(fldr_node, 1000), dimension(fields[0])));
            if(dim_change < 0 && dimension(fldr_node) > 0)
                icode.push_back(fop(STOL, fldx_mname(fldr_node, dimension(fldr_node)), dimension(fields[0])));
            break;
        case FTK_STRING: 
            if(name.has(fldr_node)) 
                icode.push_back(fop(RVV, name(fldr_node), fldr_node, google::protobuf::FieldDescriptor::Type::TYPE_STRING));
            else
                icode.push_back(fop(RVC, get_value(fldr_node), fldr_node, google::protobuf::FieldDescriptor::Type::TYPE_STRING));
            break;
        case FTK_INTEGER: 
            if(name.has(fldr_node)) 
                icode.push_back(fop(RVV, name(fldr_node), fldr_node, google::protobuf::FieldDescriptor::Type::TYPE_INT64));
            else
                icode.push_back(fop(RVC, get_value(fldr_node), fldr_node, google::protobuf::FieldDescriptor::Type::TYPE_INT64));
            break;
        case FTK_FLOAT: 
            if(name.has(fldr_node)) 
                icode.push_back(fop(RVV, name(fldr_node), fldr_node, google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE));
            else
                icode.push_back(fop(RVC, get_value(fldr_node), fldr_node, google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE));
            break;
        case FTK_enum: 
            icode.push_back(fop(RVC, std::to_string(enum_descriptor(fldr_node)->number() == 0)));
            icode.back().ev1 = enum_descriptor(fldr_node);
            if(expected_type == FTK_STRING) {
                icode.push_back(COES);
            } else {
                icode.push_back(COEI);
            }
            icode.back().er = enum_descriptor(fldr_node)->type();
            coop = NOP;
            break;
    }
    if(coop != NOP) 
        icode.push_back(coop);
    return error_count;
}
/**
 * Generate code to set up the all the fields in the input grpc messsage 
 *
 * lv_name: the left-value 
 *     1 - either the name of a message reference that is the input to a node call
 *     2 - the name of reference to a message that is the return value for a node
 *     3 - one of the above followed by a list of field names
 *     5 - the name of a temporary variable of a basic type (integer, float or string)
 * lvd: descriptor associated with the left value: 
 *      either the input for a node 
 *      or the output for an entry
 * lvfd: descriptor associated with the case number 3 of lv_name
 * arg_node: current node
 * node_ip: node to icode address 
 *
 */
int flow_compiler::populate_message(std::string const &lv_name, lrv_descriptor const &lvd, int arg_node, std::map<int, int> &node_ip, int loop_level) {
    int error_count = 0;
    if(lvd.is_repeated()) {
        std::set<std::pair<int, int>> refs;
        get_field_refs(refs, arg_node, 0);
        std::set<std::pair<std::string, int>> inds;
        if(dimension(arg_node) == 0)
            inds.insert(std::make_pair(std::string("1"), 0));

        for(auto p: refs) {
            auto const &fields = at(p.first).children;
            int rvn = get_id(fields[0]) == input_label? 0: named_blocks.find(get_id(fields[0]))->second.second;
            auto const rv_name = get_id(fields[0]) == input_label? cs_name("", 0): cs_name("RS", name(rvn));

            int xc = dimension(arg_node);
            if(xc > p.second)
                continue;
            std::string fa_name = rv_name;
            for(auto i = 0, e = dimension(fields[0]); i < e; ++i) {
                if(--xc <= 0)
                    break;
                fa_name +=  ":";
            }
            if(xc > 0) for(unsigned i = 1; i < fields.size(); ++i) {
                fa_name += ".";
                fa_name += get_id(fields[i]);
                auto fid = field_descriptor(fields[i]);
                if(fid->is_repeated()) {
                    if(--xc <= 0)
                        break;
                    fa_name += "*";
                } else {
                    fa_name += "+";
                }
            }

            std::string dr_name = std::string(dimension(fields[0]), '*') + rv_name;
            for(unsigned i = 1; i < fields.size(); ++i) {
                dr_name += "+"; dr_name += get_id(fields[i]);
                if(field_descriptor(fields[i])->is_repeated()) dr_name += "*";
            }
            if(p.second != 0)
                inds.insert(std::make_pair(fa_name, dimension(fields[0])));
        }
        icode.push_back(fop(LOOP));
        for(auto ind: inds) {
            icode.push_back(fop(LPC, ind.first, ind.second));
        }
        icode.push_back(fop(BLP, lv_name, lvd.is_message()? 1: 0));
        ++loop_level;
    }

    assert(arg_node != 0);
    switch(at(arg_node).type) {
        case FTK_fldm: {
            for(int fldd_node: at(arg_node).children) {
                auto fidp = field_descriptor(fldd_node);
                std::string lname = lv_name;
                if(!lname.empty()) lname += ".";
                lname += name(fldd_node);
                if(fidp != nullptr && fidp->is_repeated()) lname += "*";
                else lname += "+";
                error_count += populate_message(lname, lrv_descriptor(lvd, fidp), at(fldd_node).children[1], node_ip, loop_level);
            }
        } break;
        case FTK_fldr: {
            error_count += encode_expression(arg_node, lvd.type(), 0);
            /*
            op coop = get_conv_op(value_type(arg_node), lvd.type(), 0, lvd.grpc_type());
            if(coop != NOP)
                icode.push_back(coop);
                */
            icode.push_back(fop(SETF, lv_name));
        } break;
        case FTK_fldx: {
            auto const &fields = at(arg_node).children;
            // TODO improve name generation
            int rvn = get_id(fields[0]) == input_label? 0: named_blocks.find(get_id(fields[0]))->second.second;
            auto const rv_name = get_id(fields[0]) == input_label? cs_name("", 0): cs_name("RS", name(rvn));
            auto const rvd = message_descriptor(rvn);

            std::string fa_name = rv_name+std::string(dimension(fields[0]), ':');
            for(unsigned i = 1; i < fields.size(); ++i) {
                fa_name += ".";
                fa_name += get_id(fields[i]);
                fa_name += field_descriptor(fields[i])->is_repeated()? "*" : "+";
            }
            icode.push_back(fop(RVF, fa_name, dimension(fields[0]), dimension(arg_node)));

            if(fields.size() > 1) {
                auto const rvfd = lrv_descriptor(rvd, field_descriptor(fields.back()));
                int chk = check_assign(arg_node, lvd, rvfd);
                error_count += chk? 0:1;

                // if left and right are descriptors we copy
                if(is_message(field_descriptor(fields.back())) && lvd.dp != nullptr) {
                    icode.push_back(fop(COPY, lv_name));
                } else {
                    //ri = icode.size();
                    //icode.push_back(fop(RVA, rv_name, rvfd.grpc_type_name(), rvd)); 
                    op coop = get_conv_op(rvfd.type(), lvd.type(), rvfd.grpc_type(), lvd.grpc_type());
                    if(coop == CON1) {
                        print_ast(std::cerr, arg_node);
                        coop = NOP;
                    }
                    if(coop == COEE && lvd.enum_descriptor() != rvfd.enum_descriptor()) {
                        // Check if all values in rv can be converted to lv
                        bool can_convert = true;
                        for(int i = 0, vc = rvfd.enum_descriptor()->value_count(); i < vc; ++i) {
                            auto revd = rvfd.enum_descriptor()->value(i);
                            if(lvd.enum_descriptor()->FindValueByNumber(revd->number()) == nullptr) {
                                error_count += 1;
                                pcerr.AddError(main_file, at(arg_node), sfmt() << "not all values in \"" << rvfd.enum_descriptor()->full_name() << "\" can be converted to \"" << lvd.enum_descriptor()->full_name() << "\"");
                                can_convert = false;
                                break;
                            }
                        }
                        if(can_convert) 
                            pcerr.AddWarning(main_file, at(arg_node), sfmt() << "all \"" << rvfd.enum_descriptor()->full_name() << "\" values will be converted to their numerical correspondent values in \"" << lvd.enum_descriptor()->full_name() << "\"");
                    }
                    if(coop != NOP) {
                        icode.push_back(fop(coop, lvd.grpc_type(), rvfd.grpc_type()));
                        if(lvd.type() == FTK_enum) icode.back().el = lvd.enum_descriptor();
                        if(rvfd.type() == FTK_enum) icode.back().er = rvfd.enum_descriptor();
                    }
                    icode.push_back(fop(SETF, lv_name));
                }
            } else {
                error_count += check_assign(arg_node, lvd, lrv_descriptor(rvd)) == 2? 0: 1;
                icode.push_back(fop(COPY, lv_name));
            }
        } break;
        case FTK_ID: {
            // Single id reference this is a particular case of fldx but the left value will always be a message
            int rvn = get_id(arg_node) == input_label? 0: named_blocks.find(get_id(arg_node))->second.second;
            auto const rvd = message_descriptor(rvn);
            error_count += check_assign(arg_node, lvd.dp, lrv_descriptor(rvd))? 0: 1;

            std::string fa_name = std::string(dimension(arg_node), '*')+cs_name("RS", rvn);
            icode.push_back(fop(RVF, fa_name, dimension(arg_node), dimension(arg_node)));
            //icode.push_back(fop(RVF, fa_name, 0, 0));
            icode.push_back(fop(COPY, lv_name));
            assert(false);
        } break;
        case FTK_STRING: 
        case FTK_INTEGER: 
        case FTK_FLOAT: 
        case FTK_enum: 
            switch(lvd.grpc_type()) {
                case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
                    if(enum_descriptor.has(arg_node)) 
                        icode.push_back(fop(RVC, enum_descriptor(arg_node)->name(), arg_node, (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING));
                    else if(name.has(arg_node)) 
                        icode.push_back(fop(RVV, name(arg_node), arg_node, (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING));
                    else
                        icode.push_back(fop(RVC, get_value(arg_node), arg_node, (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING));
                break;
                case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
                case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
                    // Error
                    ++error_count;
                    pcerr.AddError(main_file, at(arg_node), sfmt() << "cannot assign literal value to this field");
                break;
                case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
                    if(enum_descriptor.has(arg_node)) {
                        icode.push_back(fop(RVC, std::to_string(enum_descriptor(arg_node)->number() == 0), arg_node, lvd.grpc_type()));
                        icode.back().ev1 = enum_descriptor(arg_node);
                        if(enum_descriptor(arg_node)->type() != lvd.enum_descriptor()) {
                            icode.back().ev1 = lvd.enum_descriptor()->FindValueByNumber(enum_descriptor(arg_node)->number());
                            // Check if the value can be assigned
                            if(lvd.enum_descriptor()->FindValueByNumber(enum_descriptor(arg_node)->number()) == nullptr) {
                                ++error_count;
                                pcerr.AddError(main_file, at(arg_node), sfmt() << "enum value \"" << enum_descriptor(arg_node)->name() << "\" (\"" << enum_descriptor(arg_node)->number() << "\") not found in \"" << lvd.enum_descriptor()->name() << "\"" );
                            } else if(lvd.enum_descriptor()->FindValueByNumber(enum_descriptor(arg_node)->number())->name() != enum_descriptor(arg_node)->name()) {
                                pcerr.AddWarning(main_file, at(arg_node), sfmt() << "enum value \"" << enum_descriptor(arg_node)->name() << "\" will be assigend as \"" <<  lvd.enum_descriptor()->FindValueByNumber(enum_descriptor(arg_node)->number())->name() <<  "\"" );
                            }
                        }
                    } else if(at(arg_node).type == FTK_STRING) {
                        // TODO - add conversion from variable to enum
                        auto evd = lvd.enum_descriptor()->FindValueByName(get_value(arg_node));
                        if(evd != nullptr) {
                            icode.push_back(fop(RVC, evd->name(), arg_node, lvd.grpc_type()));
                            icode.back().ev1 = evd;
                        } else {
                            ++error_count;
                            pcerr.AddError(main_file, at(arg_node), sfmt() << "enum value not found: '" << get_value(arg_node) << "' in '" << lvd.enum_descriptor()->name() << "'" );
                        }

                    } else {
                        int number = at(arg_node).type == FTK_INTEGER? (int) get_integer(arg_node): (int) get_float(arg_node);
                        // TODO - add conversion from variable to enum
                        auto evd = lvd.enum_descriptor()->FindValueByNumber(number);
                        if(evd != nullptr) {
                            icode.push_back(fop(RVC, evd->name(), arg_node, lvd.grpc_type()));
                            icode.back().ev1 = evd;
                        } else {
                            ++error_count;
                            pcerr.AddError(main_file, at(arg_node), sfmt() << "no enum with value '" << number << "' was found in '"<< lvd.enum_descriptor()->name() << "'");
                        }
                    }
                break;

                case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                    if(name.has(arg_node)) {
                        icode.push_back(fop(RVV, name(arg_node), arg_node, lvd.grpc_type()));
                     } else if(enum_descriptor.has(arg_node)) {
                        icode.push_back(fop(RVC, std::to_string(enum_descriptor(arg_node)->number() != 0), arg_node, lvd.grpc_type()));
                    } else {
                        icode.push_back(fop(RVC, std::to_string(string_to_bool(get_value(arg_node))), arg_node, lvd.grpc_type()));
                    }
                break;

                case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:

                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:

                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:

                case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                    if(name.has(arg_node)) {
                        icode.push_back(fop(RVV, name(arg_node), arg_node, lvd.grpc_type()));
                    } else if(enum_descriptor.has(arg_node)) {
                        icode.push_back(fop(RVC, std::to_string(enum_descriptor(arg_node)->number()), arg_node, lvd.grpc_type()));
                    } else if(at(arg_node).type == FTK_STRING) {
                        pcerr.AddError(main_file, at(arg_node), sfmt() << "numeric value expected here");
                    } else {
                        if(!can_cast(arg_node, lvd.grpc_type()))
                            pcerr.AddWarning(main_file, at(arg_node), sfmt() << "the value \"" << get_value(arg_node) << "\" will be assigned as \"" << get_number(arg_node, lvd.grpc_type()) << "\"");
                        icode.push_back(fop(RVC, get_number(arg_node, lvd.grpc_type()), arg_node, lvd.grpc_type()));
                    }
                break;
            }
            icode.push_back(fop(SETF, lv_name));
        break;

        default:
            print_ast(std::cerr, arg_node);
            assert(false);
    }
    if(lvd.is_repeated()) {
        icode.push_back(fop(ELP));
    }
    return error_count;
}

int flow_compiler::compile_flow_graph(int entry_blck_node, std::vector<std::set<int>> const &fg, std::set<int> const &node_set) {
    int error_count = 0;
    DEBUG_ENTER;

    // pointer to the begining of code for each node
    std::map<int, int> node_ip;
    auto emd = method_descriptor(entry_blck_node);
    std::string return_name = cs_name("RS", entry_blck_node);

    std::vector<std::set<int>> node_stages(fg);

    // MTHD marks the beggining of a method implementation.
    // The list of all nodes that can be visited by this method is stored in the args (in source order).
    icode.push_back(fop(MTHD, cs_name("", 0), return_name, emd, emd->input_type(), emd->output_type()));
    std::set<int> method_set;
    for(auto const &stage_set: node_stages) 
        for(auto n: stage_set) method_set.insert(n);
    for(auto n: method_set) icode.back().arg.push_back(n);      // MTHD arg 1... nodes used by this entry

    DEBUG_CHECK(1);

    // Reorder nodes and label them accordingly
    std::map<std::string, int> node_order;
    for(auto const &stage_set: node_stages) 
        for(int n: stage_set) {
            auto &ni = referenced_nodes.find(n)->second;
            if(condition.has(n)) {
                if(contains(node_order, name(n))) {
                    ++node_order[name(n)];
                } else {
                    node_order[name(n)] = 1;
                }
                ni.order = node_order[name(n)];
            }

            if(ni.id.empty() && ni.order == 0) 
                ni.xname = name(ni.node);
            else if(ni.id.empty()) 
                ni.xname = sfmt() << name(ni.node) << "-" << ni.order;
            else 
                ni.xname = sfmt() << name(ni.node) << "-" << ni.id;
        }

    // Keep track of node sets 
    std::set<std::string> foak;
    // Keep track of nodes with output
    std::set<std::string> foak_wo;
    DEBUG_CHECK("generating " << node_stages.size() << " stages");

    int stage = 0;
    for(auto const &stage_set: node_stages) {
        ++stage;
        int stage_idx = icode.size();
        int stage_dim = 0; 
        int stage_dim2 = 0;
        // BSTG marks the beginning of an execution stage. 
        // The stage set contains all the nodes to be processed in this stage in the order 
        // they were declared in the source file.
        // At this point all the inputs needed by this node set are known and calls to nodes
        // in the same stage can be made in parallel.

        icode.push_back(fop(BSTG, stage));                    // BSTG arg 1: stage number
        icode[stage_idx].arg.push_back(stage_set.size());     // BSTG arg 2: the number of nodes in this stage
        icode[stage_idx].arg.push_back(0);                    // BSTG arg 3: max node dimension (placeholder)
        DEBUG_CHECK("begin stage " << stage);

        std::vector<std::string> stage_set_names;
        std::set<std::string> distinct_names;    

        for(int n: stage_set) {
            icode[stage_idx].arg.push_back(n);   // BSTG arg 4... nodes in this stage
            std::string nn(name(n));             // default name for this node

            auto &mni = referenced_nodes.find(n)->second;
            stage_set_names.push_back(mni.xname);

            distinct_names.insert(nn);
        }
        
        icode[stage_idx].arg1 = join(stage_set_names, ", ");  // label for this node set
        DEBUG_CHECK("generating nodes " << stage_set);
      
        for(int pass = 0; pass < 2; ++pass) for(int node: stage_set) if((pass == 0 && method_descriptor(node) == nullptr && dimension(node) >= 0) || (pass == 1 && method_descriptor(node) != nullptr) || (pass == 1 && dimension(node) < 0)) {
            DEBUG_CHECK("generating node " << node << " pass " << pass);
            auto md = method_descriptor(node);
            auto output_type = message_descriptor(node);
            auto input_type = input_descriptor(node, output_type);

            std::string rs_name;
            if(output_type != nullptr) rs_name = cs_name("RS", name(node));
            std::string rq_name;
            if(input_type != nullptr) rq_name = cs_name("RQ", node);

            int node_idx = icode.size();
            node_ip[node] = node_idx;

            icode.push_back(fop(BNOD, rs_name, rq_name, output_type, input_type));
            icode.back().arg.push_back(dimension(node));
            icode.back().arg.push_back(node);
            icode.back().arg.push_back(stage);

            std::set<std::pair<int, int>> refs;
            get_field_refs(refs, node, 0);

            for(int i = 0, e = dimension(node); e > 0 && i != e; ++i) {
                // Grab all distinct right value references in this node
                std::set<std::pair<std::string, int>> r;
                for(auto p: refs) if(p.second > i) 
                    r.insert(std::make_pair(fldx_mname(p.first, i+1), dimension(at(p.first).children[0])));
            
                icode.push_back(fop(NLP, i+1, r.size())); 
                for(auto nc: r) {
                    icode.push_back(fop(LPC, nc.first, nc.second, i+1));
                }
                icode.push_back(fop(BNL, i+1)); 
            }

            int cc = 0;
            if(condition.has(node)) {
                error_count += encode_expression(condition(node), FTK_INTEGER, 0);
                ++cc;
            }
            // Add all the conditions for the nodes with the same name that haven't been compiled yet 
            // but appear before in source order
            for(auto n: all_nodes(name(node))) {
                if(!contains(node_ip, n)) {
                    if(condition.has(n)) {
                        error_count += encode_expression(condition(n), FTK_INTEGER, 0);
                        icode.push_back(fop(IOP, "!", 1, 0));
                        if(cc > 0) {
                            icode.push_back(fop(IOP, "&&", 2, 14));
                        }
                        ++cc;
                    }
                } 
                if(n == node) // stop at the current node
                    break;
            }
            icode.push_back(fop(IFNC, name(node), node, cc)); 

            if(output_type != nullptr) {
                // Populate the request 
                int an = find_first(node, [this](int n)->bool { return at(n).type == FTK_RETURN || at(n).type == FTK_OUTPUT; });
                error_count += populate_message(rq_name, lrv_descriptor(input_type), 
                        at(an).children[at(an).type == FTK_RETURN? 0: 1],
                        node_ip, 0);

                if(md != nullptr) {
                    icode.push_back(fop(CALL, rq_name, rs_name, md));
                } else {
                    // Dimension is set here to 0 as the reuests are already indexed
                    icode.push_back(fop(RVF, rq_name, 0));
                    icode.push_back(fop(COPY, rs_name));
                }

            } else {
                // This is either an empty or an error node
                int errm_node = find_first(node, type, "error");
                if(errm_node != 0) 
                    icode.push_back(fop(ERR, get_string(errm_node)));
            }

            icode.back().arg.push_back(node);
            icode.push_back(fop(ENOD, rs_name, dimension(node)));
            stage_dim = std::max(stage_dim, dimension(node));

            // set the foak arg
            if(!contains(foak, name(node))) {
                foak.insert(name(node));
                icode[node_idx].arg.push_back(node);
            } else {
                icode[node_idx].arg.push_back(0);
            }
            // set the foak with output arg
            if(output_type != nullptr &&  !contains(foak_wo, name(node))) {
                foak_wo.insert(name(node));
                icode[node_idx].arg.push_back(node);
            } else {
                icode[node_idx].arg.push_back(0);
            }
            // the number of alternate nodes
            icode[node_idx].arg.push_back(all_nodes(name(node)).size()-1);

            // check that previous nodes of the same name have the same dimension
            if(dimension(node) >= 0)
                for(auto nip: node_ip) 
                    if(nip.first != node && name(nip.first) == name(node) && icode[nip.second].arg[0] >= 0 && icode[nip.second].arg[0] != dimension(node)) {
                        pcerr.AddError(main_file, at(node), sfmt() << "size of result of this node is different from a previous node of the same type");
                        pcerr.AddNote(main_file, at(nip.first), sfmt() << "previous node declared here");
                        ++error_count;
                    }
        }
        DEBUG_CHECK("end stage " << stage);
        icode.push_back(fop(ESTG, icode[stage_idx].arg1, stage));
        icode[stage_idx].arg[2] =  stage_dim;                 // BSTG arg 3: max node dimension 
    }

    // Generate code to populate the Response 
    icode.push_back(fop(BPRP, entry_blck_node));
    icode.back().d1 = emd->output_type();
    icode.back().m1 = emd;

    int entry_arg_node = at(find_first(entry_blck_node, [this](int n)->bool { return at(n).type == FTK_RETURN; })).children[0];
    if(trace_on) print_ast(std::cerr, at(entry_arg_node).children[0]);

    if(0 != dimension(entry_arg_node)) {
        ++error_count;
        pcerr.AddError(main_file, at(entry_arg_node), sfmt() << "in entry \"" << emd->full_name() << "\" the return expression has a higher dimension \""
                << dimension(entry_arg_node) << "\" than expected");
    }

    DEBUG_CHECK("pop message");
    error_count += populate_message(return_name, lrv_descriptor(emd->output_type()), entry_arg_node, node_ip, 0);
    icode.push_back(fop(EPRP));
    icode.push_back(fop(END));

    DEBUG_LEAVE;
    return error_count;
}
void flow_compiler::print_pseudocode(std::ostream &out) const {
    int error_count = 0;
    DEBUG_ENTER;    
    int digits = log10(icode.size())+1;
    int l = 0;
    for(auto const &s: icode) {
        l += 1;
        char lbuf[digits+1];
        snprintf(lbuf, digits+1, "%*d", digits, l);
        out << lbuf << " "  << s << "\n";
    }
    DEBUG_LEAVE;
}
int flow_compiler::compile(std::set<std::string> const &targets) {
    int root = ast_root();
    if(at(root).type != FTK_ACCEPT || at(root).children.size() != 1)
        return 1;
    root = at(root).children[0];
    if(at(root).type != FTK_flow || at(root).children.size() < 1)
        return 1;

    int error_count = 0;

    for(auto const &synerr: syntax_errors) {
        pcerr.AddError(main_file, at(synerr.first), synerr.second);
        ++error_count;
    }
    if(error_count > 0) return error_count;

    // The main file parsed correctly, now
    // import all the nedded proto files before anything else.
    for(int n: at(root).children)
        error_count += compile_if_import(n);

    // Some import errors might be inconsequential but for now,
    // give up in case of import errors. 
    if(error_count != 0) return error_count;

    // Compile all statements, i.e. all global porperty settings,
    // all nodes, containers, and entries.
    for(int n: at(root).children)
        error_count += compile_stmt(n);
    // Check if all node sets have output message defined
    for(auto const &nb: named_blocks) {
        int node = nb.second.second;
        if(type(node) != "node") 
            continue;
        if(message_descriptor.has(node))
            continue;
        ++error_count;
        pcerr.AddError(main_file, at(node), sfmt() << "cannot determine output type for node \""<< name(node) <<"\n");
    }
    if(error_count > 0) return error_count;

    // Adjustments for the AST
    // Node 0 stands for the input
    name.put(0, input_label);
    message_descriptor.put(0, input_dp);

    // Revisit fldx nodes to resolve references
    error_count += compile_fldx(root);
    if(error_count > 0) return error_count;
    // Revisit rexp and oexp nodes 
    error_count += compile_exp_id(root);
    if(error_count > 0) return error_count;
    // Revisit id references
    error_count += compile_id_ref(root);
    if(error_count > 0) return error_count;

    // Fixup value_type for fldr
    error_count += fixup_vt(root);

    for(auto const &ep: named_blocks) if(type(ep.second.second) == "entry") {
        // Build the flow graph for each entry 
        error_count += build_flow_graph(ep.second.second);
    }

    if(error_count > 0) return error_count;

    if(flow_graph.size() == 0) {
        pcerr.AddError(main_file, -1, 0, sfmt() << "no entries defined");
        return 1;
    } 
    icode.clear();
    
    std::map<int, std::set<int>> graph_referenced_nodes;
    for(auto const &gv: flow_graph) {
        auto const &e = at(gv.first);
        std::set<int> entry_referenced_nodes;
        for(auto const &ss: gv.second) {
            for(int n: ss) if(n != gv.first) {
                referenced_nodes.emplace(n, node_info(n, (!condition.has(n)? name(n): sfmt() << name(n) << "-" << at(n).token.line << "-" << at(n).token.column), method_descriptor(n)==nullptr));
                entry_referenced_nodes.insert(n);
                auto &ni = referenced_nodes.find(n)->second;

                int value = 0;
                error_count += get_block_value(value, n, "id", false, {FTK_INTEGER, FTK_STRING});

                if(value > 0) {
                    ni.id = get_value(value);
                    // Check that id doesn't clash with other ids
                    for(auto &cn: referenced_nodes) {
                        if(&ni == &cn.second) 
                            continue;

                        if(name(cn.second.node) == name(ni.node) && cn.second.id == ni.id) {
                            pcerr.AddError(main_file, at(value), sfmt() << "illegal reuse of ID \"" << ni.id << "\" for \"" << name(ni.node) << "\" nodes");
                            pcerr.AddNote(main_file, at(cn.second.node), sfmt() << "previously used here");
                            ++error_count;
                            break;
                        }
                    }
                }

                value = 0;
                error_count += get_block_value(value, n, "group", false, {FTK_INTEGER, FTK_STRING});
                if(value > 0) 
                    ni.group = get_value(value);
            }
        }
        if(verbose && entry_referenced_nodes.size() == 0) 
            pcerr.AddWarning(main_file, e, sfmt() << "entry \""<< method_descriptor(gv.first)->full_name()<< "\" doesn't use any nodes");
        graph_referenced_nodes[gv.first].insert(entry_referenced_nodes.begin(), entry_referenced_nodes.end());
    }
    for(auto ne: named_blocks) if(ne.second.first == "node" && !contains(referenced_nodes, ne.second.second)) 
        pcerr.AddWarning(main_file, at(ne.second.second), sfmt() << "node \"" << ne.first << "\" is not used by any entry");

    // Update dimensions for each data referencing node
    if(error_count > 0) return error_count;
    DEBUG_CHECK("updating dimensions");
    error_count += update_dimensions(root);
    DEBUG_CHECK("dimensions are fine");

    if(error_count > 0) return error_count;
    DEBUG_CHECK("compiling flow graphs");
    for(auto const &gv: flow_graph) {
        // Mark the entry point 
        entry_ip[gv.first] = icode.size();
        error_count += compile_flow_graph(gv.first, gv.second, graph_referenced_nodes[gv.first]);
    }
    DEBUG_CHECK("done flow graphs");
    return error_count;
}
static std::pair<std::string, std::string> make_labels(std::set<std::string> const &fields, google::protobuf::Descriptor const *d=nullptr)  {
    std::string ftip(join(fields, "\\n"));
    if(d != nullptr) {
        std::vector<std::string> all_fieldsv = get_field_names(d, ".");
        std::set<std::string> all_fields(all_fieldsv.begin(), all_fieldsv.end()); 
        if(includes(fields, all_fields)) 
            return std::make_pair(std::string(), ftip);
    }
    return std::make_pair(join(fields, ",\\n"), ftip);
}

static std::string make_label(std::set<std::string> const &fields, google::protobuf::Descriptor const *d=nullptr)  {
    auto l = make_labels(fields, d);
    if(l.second.empty()) return l.first;
    return l.first + "\" edgetooltip=\"" + l.second;
}

void flow_compiler::print_graph(std::ostream &out, int entry) {
    if(entry < 0) {
        for(int e = 0; e  < flow_graph.size(); ++e) 
            print_graph(out, e);
        return;
    }
    auto fgp = flow_graph.find(entry);
    if(fgp == flow_graph.end()) return;

    std::string ename(c_escape(std::string("<")+method_descriptor(entry)->name()+">"));
    std::map<int, std::set<std::string>> incoming;

    out << "digraph " << c_escape(method_descriptor(entry)->full_name()) << " {\n";
    out << "{ node [shape=invtriangle]; \"" << input_label << "\"; node [shape=plaintext];\n";
    out << "\"[i]\"";
    for(int s = 0; s < fgp->second.size(); ++s) {
        out << " -> \"" << (s+1) << "\"";
    }
    out << " -> \"[o]\";\n";
    out << "}\n";
    out << "{ rank = same; \"" << input_label << "\"; \"[i]\"; };\n";
    out << "node [shape=ellipse];\n";
    int s = 0;
    for(auto const &n: fgp->second) {
        ++s;
        if(n.size()  > 0) {
            out << "{ rank = same;\n";
            for(auto nn: n) {
                auto const &inf = referenced_nodes.find(nn)->second;
                if(condition.has(nn))
                    out << c_escape(inf.xname) << "[label=<" << name(nn) << "<sup><font point-size=\"7\">" << inf.order << "</font></sup><br/><font point-size=\"7\" face=\"Courier\">" << html_escape(to_text(condition(nn))) << "</font>>]; ";
                else
                    out << c_escape(inf.xname) << "[label=<" << name(nn) << ">]; ";
            }
            out << s << ";\n};\n";
        }
        for(auto nn: n) {
            std::string dot_node(c_escape(referenced_nodes.find(nn)->second.xname));
            // Get all incoming edges
            incoming.clear();
            for(auto i: get_node_body_refs(incoming, nn))
                if(i.first == 0) {
                    out << input_label << " -> " << dot_node << " [fontsize=9,style=bold,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
                } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
                    std::string dot_i(c_escape(j.second.xname)); 
                    out << dot_i << " -> " << dot_node << " [fontsize=9,style=bold,label=\"" << make_label(i.second, message_descriptor(i.first)) << "\"];\n";
                }
            
            incoming.clear();
            for(auto i: get_node_condition_refs(incoming, nn)) 
                if(i.first == 0) {
                    out << input_label << " -> " << dot_node << " [fontsize=9,style=dashed,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
                } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
                    std::string dot_i(c_escape(j.second.xname)); 
                    out << dot_i << " -> " << dot_node << " [fontsize=9,style=dashed,label=\"" << make_label(i.second, message_descriptor(i.first)) << "\"];\n";
                }
        }
    }
    out << "node [shape=invtriangle];\n";
    out << "{ rank = same; " << ename << "[label=" << c_escape(method_descriptor(entry)->name()) << "]; \"[o]\"; };\n";
    incoming.clear();
    for(auto i: get_node_body_refs(incoming, entry)) 
        if(i.first == 0) {
            out << input_label << " -> " << ename << " [fontsize=9,style=bold,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
        } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
            std::string dot_i(c_escape(j.second.xname)); 
            out << dot_i << " -> " << ename << " [fontsize=9,style=bold,color=dodgerblue2,label=\"" << make_label(i.second, message_descriptor(i.first)) << "\"];\n";
        }
    out << "}\n";
}

