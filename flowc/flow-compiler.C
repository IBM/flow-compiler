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
#include "massert.H"

using namespace stru1;

int flow_compiler::compile_string(std::string &str, int node, int node_type) {
    auto const &s = at(node);
    str = s.token.text;
    return s.type == node_type? 0: 1;
}
int flow_compiler::compile_id(std::string &str, int id_node) {
    return compile_string(str, id_node, FTK_ID);
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
    int dim;      
    char const *help;
};
/** 
 * function signature table:
 *      return_type: Either a basic grpc compiler type, or FTK_ACCEPT for a type that is deduced from the argument types. A negative value denotes a repeated field.
 *      arg_type: Either a basic grpc compiler type or FTK_ACCEPT for any type. A negative value means the argument *must* be a repeated field.
 *      required_argc: The number of required arguments. The number of accepted arguments is the size of arg_type.
 *      dim: The change in dimension when compared to the argument dimension.
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
    { "split",    { -FTK_STRING, { FTK_STRING, FTK_STRING }, 1, 1, "Splits the first argument by any character in the second argument. By default it splits on ASCII whitespace.\n"}},
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
int flow_compiler::compile_expression(int node) {
    int error_count = 0;
    if(node != 0) switch(at(node).type) {
        case FTK_fldr:
            error_count = compile_fldr(node);
            break;
        case FTK_INTEGER: case FTK_STRING: case FTK_FLOAT:
            value_type.put(node, at(node).type);
            break;
        case FTK_fldx: 
            error_count = compile_fldx(node);
            break;
        case FTK_enum: 
            error_count = compile_enum(node);
            break;
        default:
            MASSERT(false) << "node " << node << " was not expected to be of of type " << at(node).type << "\n";
    }
    return error_count;
}
int flow_compiler::compile_fldr(int fldr_node) {
    if(fldr_node == 0)
        return 0;
    int error_count = 0;
    auto const &fldr = at(fldr_node);
    //DEBUG_CHECK("at FLDR node " << fldr_node << " of type " << fldr.type);

    assert(fldr.type == FTK_fldr);
    assert(fldr.children.size() > 0);

    for(unsigned n = 1, e = fldr.children.size(); n < e; ++n) {
        switch(at(fldr.children[n]).type) {
            case FTK_fldr:
                error_count += compile_fldr(fldr.children[n]);
                break;
            case FTK_INTEGER: case FTK_STRING: case FTK_FLOAT:
                value_type.put(fldr.children[n], at(fldr.children[n]).type);
                break;
            case FTK_fldx: 
                error_count += compile_fldx(fldr.children[n]);
                break;
            case FTK_enum: 
                error_count += compile_enum(fldr.children[n]);
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
/**
 * 'dp' is the message descriptor for for all the fields in the left side of the map.
 */
int flow_compiler::compile_fldm(int fldm_node, Descriptor const *dp) {
    if(fldm_node == 0) 
        return 0;

    int error_count = 0;
    auto const &fldm = at(fldm_node);
    assert(fldm.type == FTK_fldm);
    if(!message_descriptor.has(fldm_node))
        message_descriptor.put(fldm_node, dp);

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
                error_count += compile_fldx(fldd.children[1]);
            break;
            case FTK_fldr:
                // Check for the right number of arguments, for valid identifiers and proper return type
                error_count += compile_fldr(fldd.children[1]);
            break;
            case FTK_enum:
                error_count += compile_enum(fldd.children[1], fidp->enum_type());
            break;
            case FTK_INTEGER:
            case FTK_STRING:
            case FTK_FLOAT:
                if(left_type == FTK_fldm) {
                    ++error_count;
                    pcerr.AddError(main_file, at(d), sfmt() << "field \"" << id << "\" is of type message");
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
int flow_compiler::update_noderef_dimension(int node) {
    if(dimension.has(node)) 
        return 0;

    std::string label = get_text(node);
    if(label == input_label) {
        dimension.put(node, 0);
        return 0;
    } 

    int error_count = 0;
    int nodes_dimension = -4;
    auto node_set = get_all_nodes(label);
    for(int n: node_set) {
        if(!dimension.has(n)) {
            update_dimensions(n);
            //std::cerr << " >>>> from " << node << " computed dimension for " << n << " this [" << name(n) << "] is " << dimension(n) << "\n";
        } else {
            //std::cerr << " >>>> from " << node << " dimension for " << n << " this [" << name(n) << "] is " << dimension(n) << "\n";
        }
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
        for(auto n: node_set) if(dimension(n) < 0)  {
            //std::cerr << " >>>> from " << node << " updating dimension for " << n << " this [" << name(n) << "] is " << nodes_dimension << "\n";
            dimension.update(n, nodes_dimension);
        }
    } else {
        // If a node set's dimension could not be computed, generate errors
        for(auto n: node_set) {
            pcerr.AddError(main_file, at(n), sfmt() << "output size can not be determined for node");
            ++error_count;
        }
    }
    
    return error_count; 
}
/*
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
        case FTK_ERROR: {
            int conode = get_ne_condition(node);
            if(conode != 0) 
                dimension.put(node, dimension(conode));
            else 
                dimension.put(node, -2);
        } break;
        case FTK_ENTRY: {
            int anode = get_ne_action(node);
            if(anode != 0) 
                dimension.put(node, dimension(anode));
            else 
                dimension.put(node, -2);
        } break;
        case FTK_NODE: {
            int anode = get_ne_action(node);
            if(anode != 0) 
                dimension.put(node, dimension(anode));
            else 
                dimension.put(node, -2);
            int conode = get_ne_condition(node);
            if(conode != 0 && dimension(node) >= 0 && dimension(conode) > dimension(node)) {
                pcerr.AddError(main_file, at(node), sfmt() << "condition and node dimension mismatch");
                ++error_count;
            }
        } break;
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
        case FTK_RETURN: 
            dimension.put(node, dimension(children.back()));
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
    //DEBUG_CHECK(" for node " << node << "/" << children.size() << " " << node_name(at(node).type) << ": " << dimension(node));
    return error_count;
}
int flow_compiler::compile_fldx(int node) {
    if(node == 0)
        return 0;
    int error_count = 0;

    auto const &fldx = at(node);
    assert(fldx.children.size() > 0);

    // first field must be a node name
    std::string node_name = get_id(fldx.children[0]);
    int node_node = get_first_node(node_name);
    if(node_node == 0 && node_name != input_label) {
        pcerr.AddError(main_file, at(fldx.children[0]), sfmt() << "unknown node \"" << node_name << "\"");
        return error_count + 1;
    }
    Descriptor const *dp = message_descriptor(node_node);
    assert(!message_descriptor.has(fldx.children[0]));
    message_descriptor.put(fldx.children[0], dp);

    for(unsigned i = 1; i < fldx.children.size(); ++i) {
        int f = fldx.children[i];
        std::string id(get_id(f));
        FieldDescriptor const *fidp = dp->FindFieldByName(id);
        if(fidp == nullptr) {
            pcerr.AddError(main_file, at(f), sfmt() << "field name \"" << id << "\" not found in message of tyoe \"" << dp->full_name() << "\"");
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
 * Reslove enum references
 */
int flow_compiler::compile_enum(int node, EnumDescriptor const *ed) {
    if(node == 0) return 0;
    int error_count = 0;
    EnumValueDescriptor const *evdp = nullptr;
    std::string id_label(get_dotted_id(node));
    // find all enums that match this id
    std::set<std::string> matches;
    // search in context first
    if(ed != nullptr) {
        for(int ev = 0, evc = ed->value_count(); ev != evc; ++ev) {
            auto evd = ed->value(ev);
            if(evd->name() == id_label || evd->full_name() == id_label || stru1::ends_with(evd->full_name(), std::string(".")+id_label)) {
                evdp = evd;
                matches.insert(evd->full_name());
            }
        }
    }
    // search globally if nothing was found in context
    if(matches.size() == 0) {
        for(auto evd: enum_value_set) {
            if(evd->name() == id_label || evd->full_name() == id_label || stru1::ends_with(evd->full_name(), std::string(".")+id_label))  {
                evdp = evd;
                matches.insert(evd->full_name());
            }
        }
    }
    if(evdp != nullptr)
        enum_descriptor.put(node, evdp);
    if(matches.size() == 0) {
        pcerr.AddError(main_file, at(node), sfmt() << "unknown enum label \"" << id_label << "\"");
        ++error_count;
    } else if(matches.size() > 1) {
        pcerr.AddError(main_file, at(node), sfmt() << "ambiguous enum label \"" << id_label << "\"");
        pcerr.AddNote(main_file, at(node), sfmt() << "matches " << stru1::join(matches, ", ", " and ", "", "\"", "\""));
        ++error_count;
    }
    return error_count;
}

// If exp_node is a valid pointer, the block is expected to contain one output (oexp) or return (rexp) definition 
/*
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
                if(elem_id == "error" && cot::contains(output_nvn, elem_id)) {
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
                if(exp_node_count == 0 && exp_node != nullptr && cot::contains(output_nvn, elem_id)) {
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
*/
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
int flow_compiler::compile_defines() {
    int error_count = 0;
    for(int n: *this) {
        auto const &stmt = at(n);
        switch(stmt.type) {
            case FTK_DEFINE: {
                std::string statement = get_id(stmt.children[0]);
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
            } 
                break;
            default: 
                break;
        }
    }
    return error_count; 
}
int flow_compiler::build_flow_graph(int blk_node) {
    int error_count = 0;

    // All the nodes reachable from the entry, including the entry itself
    std::set<int> used_nodes; 

    // initialze a processing buffer (stack) with the entry
    // process each node in the buffer until the buffer is empty
    for(std::vector<int> todo(&blk_node, &blk_node+1); todo.size() > 0;) {
        int cur_node = todo.back(); todo.pop_back();
        if(cot::contains(used_nodes, cur_node))
            continue;
        // add the current node...
        used_nodes.insert(cur_node);
        // stack all the aliases unless already processed
        if(cur_node) for(auto n: get_all_nodes(cur_node)) {
            if(!cot::contains(used_nodes, n)) 
                todo.push_back(n);
        }
        // also push all the nodes referenced by the current node in the stack
        std::map<int, std::set<std::string>> noset;
        for(int n: get_referenced_node_types(cur_node))
            if(at(n).type != FTK_ENTRY && !cot::contains(used_nodes, n))
                todo.push_back(n);
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
            //std::map<int, std::set<std::string>> noset;
            //get_node_refs(noset, nx.first, 0);
            for(int nsf: get_referenced_node_types(nx.first)) {
                adjmat[nx.second][ntox[nsf]] = true;
                if(nsf != 0 && at(nsf).type != FTK_ENTRY) 
                    for(auto n: get_all_nodes(nsf))
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
            if(!cot::contains(solved, xton[x])) {
                bool s = true;
                for(unsigned y = 0; y != mx; ++y) 
                    if(adjmat[x][y] && !cot::contains(solved, xton[y])) {
                        s = false;
                        break;
                    }
                if(s) graph.back().insert(xton[x]);
            }
        if(graph.back().size() == 0) 
            break;

        if(cot::contains(graph.back(), blk_node)) {
            solved.insert(blk_node);
            graph.pop_back();
            // Successfully finised
            break;
        }
        for(auto n: graph.back()) 
            solved.insert(n);
    }
    if(!cot::contains(solved, blk_node)) {
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
                if(adjmat[x][x] && !cot::contains(circular, xton[x])) {
                    circular.insert(xton[x]);
                    if(i == 0) 
                        pcerr.AddError(main_file, at(xton[x]), sfmt() << "node \"" << type(xton[x]) << "\" references itself");
                    else
                        pcerr.AddError(main_file, at(xton[x]), sfmt() << "circular reference of node \"" << type(xton[x]) << "\"");
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
    } else if(graph.size() == 0) {
        pcerr.AddWarning(main_file, at(blk_node), sfmt() << "entry \"" << method_descriptor(blk_node)->full_name()<< "\" doesn't use any nodes");
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
static std::ostream &operator <<(std::ostream &out, lrv_descriptor const &lrv) {
    out << "<lrv " << lrv.type_name();
    if(lrv.dp != nullptr) out << ", (" << lrv.dp->full_name() << ")";
    if(lrv.fp != nullptr) out << ", [" << lrv.fp->full_name() << "]";
    out << ">";
    return out;
}
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
 * Return all fldx ast nodes that are referenced from expr_node, and their final dimension
 */
std::set<std::pair<std::string, int>> flow_compiler::get_referenced_fields(int neen) const {
    std::set<std::pair<int, int>> fnr;
    int ec =  get_field_refs_r(fnr, neen, 0);
    std::set<std::pair<std::string, int>> fns;
    if(ec == 0) for(auto nd: fnr) 
        fns.insert(std::make_pair(get_dotted_id(nd.first, 0, 1)+"@"+get_dotted_id(nd.first, 1), nd.second));
    return fns;
}
std::set<std::pair<std::string, int>> flow_compiler::get_provided_fields(int nodex) const {
    std::set<std::pair<std::string, int>> fns;
    std::string nn("input");
    auto d = message_descriptor(nodex);
    if(nodex != 0) {
        nn = type(nodex);
        if(d == nullptr) for(int nx: get_all_nodes(nodex)) 
            if((d = message_descriptor(nx)) != nullptr)
                break;
    }
    MASSERT(d != nullptr) << "Cannot find return type for node " << nodex << "\n";
    auto fv = get_fields(d, dimension(nodex));
    for(auto f: get_fields(d, dimension(nodex)))
        fns.insert(std::make_pair(nn+"@"+f.first, f.second));
    //std::cerr << "$$$ " << nodex << " $$$ " << fns << "\n";
    return fns;
}
int flow_compiler::get_field_refs(std::set<std::pair<int, int>> &refs, int expr_node) const {
    int ec =  get_field_refs_r(refs, expr_node, 0);
    //std::cerr << "@@@ " << expr_node << " @@@ " << refs << "\n";
    //std::cerr << "+++ " << expr_node << " +++ " << get_referenced_fields(expr_node) << "\n";
    return ec;
}
int flow_compiler::get_field_refs_r(std::set<std::pair<int, int>> &refs, int expr_node, int lv_dim) const {
    int error_count = 0;
    auto const &node = at(expr_node);
    switch(node.type) {
        case FTK_fldr: {
            switch(at(node.children[0]).type) {
                case FTK_ID: // function call
                    lv_dim -= function_table.find(get_id(node.children[0]))->second.dim;
                    break;
                case FTK_HASH:
                    ++lv_dim;         
                    break;
                default: 
                    break;
            }
            for(unsigned a = 1; a < node.children.size(); ++a) 
                error_count += get_field_refs_r(refs, node.children[a], lv_dim);
        } break;
        case FTK_fldm: {
            for(int fldd_node: node.children) 
                error_count += get_field_refs_r(refs, at(fldd_node).children[1], lv_dim + (field_descriptor(fldd_node)->is_repeated()? 1: 0));
            
        } break;
        case FTK_fldx: {
            refs.insert(std::make_pair(expr_node, dimension(expr_node) - lv_dim));
        } break;
        default:
            for(auto n: node.children) 
                error_count += get_field_refs_r(refs, n, lv_dim);
          break;
    }
    return error_count;
}
/**
 * Return a mangled field name with placehodlers for indices
 */
std::string flow_compiler::fldx_mname(int fldx_node, int context_dim) const {
    auto const &fields = at(fldx_node).children;
    int rvn = get_id(fields[0]) == input_label? 0: get_first_node(get_id(fields[0]));
    auto const rv_name = get_id(fields[0]) == input_label? cs_name("", 0): cs_name_s("RS", type(rvn));
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
                    // TODO: needs review 
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
 * lv_name: the left-value name
 *     1 - either the name of a message reference that is the input to a node call
 *     2 - the name of a message reference that is the return value for a node
 *     3 - one of the above followed by a list of field names
 *     5 - the name of a temporary variable of a basic type (integer, float or string)
 * lvd: message descriptor associated with the left value
 * lvfd: descriptor associated with the case number 3 of lv_name
 * arg_node: current node
 * node_ip: node to icode-address 
 *
 */
int flow_compiler::populate_message(std::string const &lv_name, lrv_descriptor const &lvd, int arg_node, std::map<int, int> &node_ip, int loop_level) {
    int error_count = 0;
    if(lvd.is_repeated()) {
        std::set<std::pair<int, int>> refs;
        get_field_refs(refs, arg_node);
        std::set<std::pair<std::string, int>> inds;
        if(dimension(arg_node) == 0)
            inds.insert(std::make_pair(std::string("1"), 0));

        for(auto p: refs) {
            auto const &fields = at(p.first).children;
            int rvn = get_id(fields[0]) == input_label? 0: get_first_node(get_id(fields[0]));
            auto const rv_name = get_id(fields[0]) == input_label? cs_name("", 0): cs_name_s("RS", type(rvn));

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
            int rvn = get_id(fields[0]) == input_label? 0: get_first_node(get_id(fields[0]));
            auto const rv_name = get_id(fields[0]) == input_label? cs_name("", 0): cs_name_s("RS", type(rvn));
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
            int rvn = get_id(arg_node) == input_label? 0: get_first_node(get_id(arg_node));
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

int flow_compiler::compile_flow_graph(int entry_blck_node, std::vector<std::set<int>> const &node_stages, std::set<int> const &node_set) {
    int error_count = 0;
    DEBUG_ENTER;

    // pointer to the begining of code for each node
    std::map<int, int> node_ip;
    auto emd = method_descriptor(entry_blck_node);
    std::string return_name = cs_name("RS", entry_blck_node);

    // MTHD marks the beggining of a method implementation.
    // The list of all nodes that can be visited by this method is stored in the args (in source order).
    icode.push_back(fop(MTHD, cs_name("", 0), return_name, emd, emd->input_type(), emd->output_type()));

    DEBUG_CHECK("generating " << node_stages.size() << " stages");

    // Keep track of node sets 
    std::set<std::string> foak;
    // Keep track of nodes with output
    std::set<std::string> foak_wo;
    // Fields available at the begining of a stage
    std::set<std::pair<std::string, int>> fava = get_provided_fields(0);
    // Keep track of compiled error nodes
    std::set<int> errnc;
    
    int stage = 0;
    for(auto const &stage_set: node_stages) {
        ++stage;
        int stage_idx = icode.size();
        int stage_dim = 0; 
        int stage_dim2 = 0;
        // BSTG marks the beginning of an execution stage. 
        // The stage set cot::contains all the nodes to be processed in this stage in the order 
        // they were declared in the source file.
        // At this point all the inputs needed by this node set are known and calls to nodes
        // in the same stage can be made in parallel.

        icode.push_back(fop(BSTG, stage));                    // BSTG arg 1: stage number
        icode[stage_idx].arg.push_back(stage_set.size());     // BSTG arg 2: the number of nodes in this stage
        icode[stage_idx].arg.push_back(0);                    // BSTG arg 3: max node dimension (placeholder)
        DEBUG_CHECK("begin stage " << stage);

        std::vector<std::string> stage_set_names;
        for(int n: stage_set) {
            icode[stage_idx].arg.push_back(n);   // BSTG arg 4... nodes in this stage
            stage_set_names.push_back(name(n));
        }
        icode[stage_idx].arg1 = join(stage_set_names, ", ");  // label for this node set
        DEBUG_CHECK("generating error checks " << stage_set);

        for(int n: *this) if(at(n).type == FTK_ERROR && !cot::contains(errnc, n)) {
            std::set<std::pair<std::string, int>> needed_fields = get_referenced_fields(get_ne_condition(n));
            needed_fields = cot::join(needed_fields, get_referenced_fields(get_errnode_action(n)));
            if(!cot::includes(fava, needed_fields)) 
                continue;
            errnc.insert(n);
            icode.push_back(fop(BERC));
            icode.back().arg.push_back(dimension(n));
            icode.back().arg.push_back(n);
            icode.back().arg.push_back(stage);

            std::set<std::pair<int, int>> refs;
            get_field_refs(refs, n);

            for(int i = 0, e = dimension(n); e > 0 && i != e; ++i) {
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
            if(get_ne_condition(n) != 0) {
                error_count += encode_expression(get_ne_condition(n), FTK_INTEGER, 0);
                ++cc;
            }
            error_count += encode_expression(get_errnode_action(n), FTK_STRING, 0);
            std::string error_code = "INVALID_ARGUMENT";
            if(at(n).children.size() == 3) {
                int cn = at(n).children[1];
                if(at(cn).type == FTK_INTEGER) 
                    error_code = grpc_error_code(get_integer(cn));
                else if(at(cn).type == FTK_ID)
                    error_code = grpc_error_code(get_id(cn));
            }
            icode.push_back(fop(ERR, error_code, cc)); 
            icode.push_back(fop(EERC, dimension(n)));
        }

        DEBUG_CHECK("generating nodes " << stage_set);
      
        for(int pass = 0; pass < 2; ++pass) for(int node: stage_set) if((pass == 0 && method_descriptor(node) == nullptr && dimension(node) >= 0) || (pass == 1 && method_descriptor(node) != nullptr) || (pass == 1 && dimension(node) < 0)) {
            DEBUG_CHECK("generating node " << node << " pass " << pass);
            auto md = method_descriptor(node);
            auto output_type = message_descriptor(node);
            auto input_type = input_descriptor(node);

            std::string rs_name = cs_name_s("RS", type(node));
            std::string rq_name = cs_name("RQ", node);

            int node_idx = icode.size();
            node_ip[node] = node_idx;

            icode.push_back(fop(BNOD, rs_name, rq_name, output_type, input_type));
            icode.back().arg.push_back(dimension(node));
            icode.back().arg.push_back(node);
            icode.back().arg.push_back(stage);

            std::set<std::pair<int, int>> refs;
            get_field_refs(refs, node);

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
            if(get_ne_condition(node) != 0) {
                error_count += encode_expression(get_ne_condition(node), FTK_INTEGER, 0);
                ++cc;
            }
            // Add all the conditions for the nodes with the same name that haven't been compiled yet 
            // but appear before in source order
            for(auto n: get_all_nodes(node)) {
                if(!cot::contains(node_ip, n)) {
                    int cn = get_ne_condition(n);
                    if(cn != 0) {
                        error_count += encode_expression(cn, FTK_INTEGER, 0);
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
                
            int an = get_ne_action_fldm(node);
            if(an == 0) get_ne_action_fldx(node);

            if(an != 0) {
                // Populate the request 
                error_count += populate_message(rq_name, lrv_descriptor(input_type), an, node_ip, 0);
                if(md != nullptr) {
                    icode.push_back(fop(CALL, rq_name, rs_name, md));
                } else {
                    // Dimension is set here to 0 as the reuests are already indexed
                    icode.push_back(fop(RVF, rq_name, 0));
                    icode.push_back(fop(COPY, rs_name));
                }
            }

            icode.back().arg.push_back(node);
            icode.push_back(fop(ENOD, rs_name, dimension(node)));
            stage_dim = std::max(stage_dim, dimension(node));

            // set the foak arg
            if(!cot::contains(foak, type(node))) {
                foak.insert(type(node));
                icode[node_idx].arg.push_back(node);
            } else {
                icode[node_idx].arg.push_back(0);
            }
            // set the foak with output arg
            if(output_type != nullptr &&  !cot::contains(foak_wo, type(node))) {
                foak_wo.insert(type(node));
                icode[node_idx].arg.push_back(node);
            } else {
                icode[node_idx].arg.push_back(0);
            }
            // the number of alternate nodes
            icode[node_idx].arg.push_back(get_all_nodes(node).size()-1);

            // check that previous nodes of the same name have the same dimension
            if(dimension(node) >= 0)
                for(auto nip: node_ip) 
                    if(nip.first != node && type(nip.first) == type(node) && icode[nip.second].arg[0] >= 0 && icode[nip.second].arg[0] != dimension(node)) {
                        pcerr.AddError(main_file, at(node), sfmt() << "size of result of this node is different from a previous node of the same type");
                        pcerr.AddNote(main_file, at(nip.first), sfmt() << "previous node declared here");
                        ++error_count;
                    }
        }
        DEBUG_CHECK("end stage " << stage);
        icode.push_back(fop(ESTG, icode[stage_idx].arg1, stage));
        icode[stage_idx].arg[2] =  stage_dim;  // BSTG arg 3: max node dimension 
        // update available fields
        for(int node: stage_set) {
            // add all fields for any node type that is not present in subsequent stages
            bool final = true;
            for(int s = stage, e = node_stages.size(); s < e; ++s)
                if(final) for(int fn: node_stages[s])
                    if(type(fn) == type(node)) {
                        final = false;
                        break;
                    }
            if(final) 
                fava = cot::join(fava, get_provided_fields(node));
        }
    }

    // Generate code to populate the Response 
    icode.push_back(fop(BPRP, entry_blck_node));
    icode.back().d1 = emd->output_type();
    icode.back().m1 = emd;

    int ean = get_ne_action_fldm(entry_blck_node);
    if(ean == 0) ean = get_ne_action_fldx(entry_blck_node);

    if(0 != dimension(entry_blck_node)) {
        ++error_count;
        pcerr.AddError(main_file, at(ean), sfmt() << "in entry \"" << emd->full_name() << "\" the return expression has a higher dimension \""
                << dimension(entry_blck_node) << "\" than expected");
    }

    error_count += populate_message(return_name, lrv_descriptor(emd->output_type()), ean, node_ip, 0);
    icode.push_back(fop(EPRP));
    icode.push_back(fop(END));

    DEBUG_LEAVE;
    return error_count;
}
/**
 * Make sure that all nodes have only one id declared and they don't clash
 */
int flow_compiler::fixup_node_ids() {
    int error_count = 0;
    std::map<std::string, int> node_xnames;
    std::map<std::string, int> node_counts;
    int node_count = 0;
    // make a first pass to make sure ids don't clash and arenot redefined
    for(int n: *this) if(at(n).type == FTK_NODE) {
        ++node_count;
        if(cot::contains(node_counts, type(n)))
            node_counts[type(n)] += 1;
        else
            node_counts[type(n)] = 1;
        bool have_id = name(n) != type(n);
        auto idvns = blck_value_nodes(get_ne_block_node(n), "id");
        if(idvns.size() > 1 || idvns.size() > 0 && have_id) {
            ++error_count;
            int first_id = n;
            for(auto p = idvns.begin(), e = idvns.end(); p != e; ++p) {
                if(p == idvns.begin() && !have_id) {
                    first_id = *p;
                    continue;
                }
                pcerr.AddError(main_file, at(*p), sfmt() << "identifier redefinition for \"" << type(n) << "\" node");
            }
            pcerr.AddNote(main_file, at(first_id), "first defined here");
            continue;
        }
        if(!have_id && idvns.size() == 1) {
            name.update(n, sfmt() << name(n) << "." << get_text(*idvns.begin()));
            have_id = true;
        }
        if(!have_id)
            continue;

        if(cot::contains(node_xnames, name(n))) {
            ++error_count;
            pcerr.AddError(main_file, at(n), sfmt() << "illegal reuse of id for node \"" << type(n) << "\"");
            pcerr.AddNote(main_file, at(node_xnames[name(n)]), "first defined here");
            continue;
        } 
        node_xnames[name(n)] = n;
    }
    // make a second pass and assign ids for the nodes that don't have one and are part of a group
    for(int n: *this) if(at(n).type == FTK_NODE && name(n) == type(n) && node_counts[type(n)] > 1) {
        for(int i = 1; i <= node_count; ++i) {
            std::string nid = sfmt() << type(n) << "." << i;
            if(!cot::contains(node_xnames, nid)) {
                name.update(n, nid);
                node_xnames[nid] = n;
                break;
            }
        }
    }
    return error_count;
}
/**
 * Walk the tree and attempt to fixup all references to grpc methods and proto messages.
 * Propagate messages and methods to the nodes and entries. 
 */
int flow_compiler::fixup_proto_refs() {
    int error_count = 0;
    int parent_node, prev_node, action_node;

    // make a first pass and solve all method references
    parent_node = prev_node = action_node = 0;
    int dtid_node = 0; // node to get the method name from
    for(int n: *this) {
        switch(at(n).type) {
            case FTK_RETURN: case FTK_OUTPUT: {
                assert(at(n).children.size() == 2 || at(n).children.size() == 1);
                if(action_node != 0) {
                    ++error_count;
                    pcerr.AddError(main_file, at(prev_node), sfmt() << "this \"" << (at(parent_node).type == FTK_ENTRY? "entry": "node") << "\" already has a \"" << get_id(action_node) << "\" statement");
                    pcerr.AddNote(main_file, at(action_node), sfmt() << "declared here");
                    break;
                }
                action_node = prev_node;
                // lookup a method if in an entry or a node with output
                if(dtid_node != 0 || at(parent_node).type == FTK_NODE && get_id(prev_node) == "output") {
                    if(dtid_node == 0) {
                        assert(at(n).children.size() == 2);
                        dtid_node = at(n).children[0];
                    }
                    assert(dtid_node != 0);
                    std::string dtid = get_dotted_id(dtid_node);
                    MethodDescriptor const *md = check_method(dtid, dtid_node);
                    if(md != nullptr) {
                        method_descriptor.put(parent_node, md);
                        message_descriptor.put(parent_node, md->output_type());
                        input_descriptor.put(parent_node, md->input_type());
                    } else {
                        ++error_count;
                    }
                } 
                // lookup a message type if in a node with return
                if(at(parent_node).type == FTK_NODE && get_id(prev_node) == "return" && at(n).children.size() == 2) {
                    dtid_node = at(n).children[0];
                    std::string dtid = get_dotted_id(dtid_node);
                    Descriptor const *od = check_message(dtid, dtid_node);
                    if(od != nullptr) {
                        method_descriptor.put(parent_node, nullptr);
                        message_descriptor.put(parent_node, od);
                        input_descriptor.put(parent_node, od);
                    } else {
                        ++error_count;
                    }
                }
            } break;
            case FTK_ENTRY: 
                dtid_node = at(n).children[1]; 
                action_node = 0;
                parent_node = n;
                break;
            case FTK_NODE: 
                dtid_node = 0;
                action_node = 0;
                parent_node = n;
                if(type(n) == input_label) {
                    ++error_count;
                    pcerr.AddError(main_file, at(n), "redefinition of the input label as a node");
                }
                break;
            default: 
                break;
        }
        prev_node = n;
    }
    if(error_count != 0)
        return error_count;
    // make sure no two entries have the same method and that any return type agrees with the method's return type
    std::map<MethodDescriptor const *, int> mds;
    Descriptor const *input_dp = nullptr;
    int first_entry = 0;
    for(int n: *this) if(at(n).type == FTK_ENTRY) {
        if(cot::contains(mds, method_descriptor(n))) {
            ++error_count;
            pcerr.AddError(main_file, at(n), sfmt() << "redefinition of rpc method is not allowed (\"" << method_descriptor(n) << "\")");
            pcerr.AddNote(main_file, at(mds[method_descriptor(n)]), sfmt() << "first declared here");
        } else {
            mds[method_descriptor(n)] = n;
        }
        if(first_entry == 0) {
            first_entry = n;
            input_dp = method_descriptor(n)->input_type();
        }

        if(input_descriptor(n) != input_descriptor(first_entry)) {
            ++error_count;
            pcerr.AddError(main_file, at(n), sfmt() << "all entries must have the same input type: \"" << input_descriptor(first_entry)->full_name() << "\"\n");
            pcerr.AddNote(main_file, at(first_entry), sfmt() << "first declared here");
        }
        // check that any declared return type agrees with the method's
        int rn =  find_first(n, [this](int x) -> bool {
            return cot::contains(std::set<int>({FTK_OUTPUT, FTK_RETURN}), at(x).type);
        });
        if(rn == 0) {
            ++error_count;
            pcerr.AddError(main_file, at(n), "no return found for this entry");
        } else if(message_descriptor(n) != nullptr && at(rn).children.size() == 2) {
            int dtid_node = at(rn).children[0];
            if(!stru1::ends_with(message_descriptor(n)->full_name(), get_dotted_id(dtid_node))) {
                ++error_count;
                pcerr.AddError(main_file, at(dtid_node), sfmt() << "entry must return message of type \"" << message_descriptor(n)->full_name() << "\", not \"" << get_dotted_id(dtid_node) << "\"");
            }
        }
    }
    // adjust the ast with the fake node 'input' 
    name.put(0, input_label); type(0, input_label);
    message_descriptor.put(0, input_dp);

    std::map<std::string, int> nodefams;
    // propagate the output types from the nodes with declared output to the ones without
    for(int n: *this) if(at(n).type == FTK_NODE && message_descriptor(n) != nullptr && !cot::contains(nodefams, type(n))) {
        // n is the first node with a method in its family
        std::string namen = type(n);
        nodefams[namen] = n;
        for(int m: *this) if(at(m).type == FTK_NODE && type(m) == namen) {
            if(!message_descriptor.has(m)) {
                method_descriptor.put(m, nullptr);
                message_descriptor.copy(n, m);
                input_descriptor.put(m, message_descriptor(m));
            } else if(message_descriptor(m) != message_descriptor(n)) {
                ++error_count;
                pcerr.AddError(main_file, at(m), sfmt() << "node declared with a different output type \"" << message_descriptor(m)->name() << "\"");
                pcerr.AddError(main_file, at(n), sfmt() << "previously declared here with type \"" << message_descriptor(n)->name() << "\"");
            }
        }
    }
    if(error_count != 0)
        return error_count;

    // check there are no nodes without an output type or entries without a return
    for(int n: *this) 
        switch(at(n).type) {
            case FTK_NODE: 
                if(message_descriptor(n) == nullptr) {
                    ++error_count;
                    pcerr.AddError(main_file, at(n), "cannot infer the output type");
                }
                break;
            case FTK_ENTRY:
                break;
            default:
                break;
        }
    return error_count;
}
int flow_compiler::compile_expressions(int node) {
    int error_count = 0;
    // action nodes
    for(int n: *this) switch(at(n).type) {
        case FTK_NODE:
            error_count += compile_fldm(get_ne_action_fldm(n), input_descriptor(n));
            error_count += compile_fldx(get_ne_action_fldx(n));
            break;
        case FTK_ENTRY:
            error_count += compile_fldm(get_ne_action_fldm(n), message_descriptor(n));
            error_count += compile_fldx(get_ne_action_fldx(n));
            break;
        default:
            break;
    }
    // conditional nodes 
    for(int n: *this) switch(at(n).type) {
        case FTK_NODE:
            error_count += compile_expression(get_ne_condition(n));
            break;
        case FTK_ERROR:
            error_count += compile_expression(get_ne_condition(n));
            error_count += compile_expression(get_errnode_action(n));
        default:
            break;
    }
    std::set<int> visited; std::vector<int> queue;
    // update ref-counts
    for(int n: *this) {
        int acn;
        switch(at(n).type) {
            case FTK_ERROR:
                acn = get_ne_condition(n);
                break;
            case FTK_ENTRY:
                acn = get_ne_action(n);
                break;
            default:
                acn = 0;
                break;
        }
        if(acn != 0)
            queue.push_back(acn); 
    }
    while(queue.size() > 0) {
        int acn = queue.back(); queue.pop_back();
        if(!visited.insert(acn).second)
            continue;
        for(int n: subtree(acn)) if(at(n).type == FTK_fldx) {
            std::string label = get_id(at(n).children[0]);
            if(label == input_label) {
                refcount.update(0, refcount(0)+1);
            } else {
                for(int r: get_all_nodes(label)) {
                    refcount.update(r, refcount(r)+1);
                    int a = get_ne_action(r);
                    if(a != 0) queue.push_back(a);
                    int c = get_ne_condition(r);
                    if(a != 0) queue.push_back(c);
                }
            }
        }
    }
    return error_count;
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

    // Check value types for some reserved define names
    error_count += compile_defines();
    // Check that node ids are unique, assign ids to the nodes that don't have one and need it.
    error_count += fixup_node_ids();
    if(error_count != 0) return error_count;
    // Lookup up grpc messages and methods referenced throughout 
    error_count += fixup_proto_refs();
    if(error_count != 0) return error_count;

//    print_ast(std::cout);
    error_count += compile_expressions();
    // Fixup value_type for fldr
    error_count += fixup_vt(root);

    for(int en: *this) if(at(en).type == FTK_ERROR && at(en).children.size() == 3) {
        int cn = at(en).children[1];
        if(at(cn).type == FTK_ID) {
            if(grpc_error_code(get_id(cn)).empty()) {
                ++error_count;
                pcerr.AddError(main_file, at(cn), sfmt() << "unknown grpc error code \"" << get_id(cn) << "\"");
            }
        }
        if(at(cn).type == FTK_INTEGER) {
            if(grpc_error_code(get_integer(cn)).empty()) {
                ++error_count;
                pcerr.AddError(main_file, at(cn), sfmt() << "invalid grpc error code \"" << get_integer(cn) << "\", must be in the rage 1 to 16 ");
            }
        }
    }
    for(int en: *this) if(at(en).type == FTK_ERROR) 
        if(dimension(at(en).children[0]) < dimension(at(en).children.back())) {
            ++error_count;
            pcerr.AddError(main_file, at(en), sfmt() << "cannot construct error message");
        }

    // Build a flow graph for each entry 
    for(int e: *this) if(at(e).type == FTK_ENTRY) 
        error_count += build_flow_graph(e);

    if(error_count > 0) return error_count;

    if(flow_graph.size() == 0) {
        pcerr.AddError(main_file, -1, 0, sfmt() << "no entries defined");
        return 1;
    } 
    icode.clear();
    for(int n: *this) if(at(n).type == FTK_NODE && refcount(n) == 0) 
        pcerr.AddWarning(main_file, at(n), sfmt() << "node \"" << type(n) << "\" is not used by any entry");
    // Update dimensions for each data referencing node
    if(error_count > 0) return error_count;
    error_count += update_dimensions(root);
    if(error_count > 0) return error_count;
    for(auto const &gv: flow_graph) {
        std::set<int> gnodes;
        for(auto &s: gv.second) gnodes.insert(s.begin(), s.end());
        // Mark the entry point 
        entry_ip[gv.first] = icode.size();
        error_count += compile_flow_graph(gv.first, gv.second, gnodes);
    }
    return error_count;
}
