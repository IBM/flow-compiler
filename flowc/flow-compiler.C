#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wlogical-op-parentheses"
#endif

#include "flow-compiler.H"
#include "stru1.H"
#include "grpc-helpers.H"
#include "flow-ast.H"
#include "flow-parser.c"

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
                if(!io::Tokenizer::ParseInteger(token.text, UINT64_MAX, &ftok.integer_value)) {
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
                        case '{': ftok.type = FTK_OPENBRA; break;
                        case '}': ftok.type = FTK_CLOSEBRA; break;
                        case '(': ftok.type = FTK_OPENPAR; break;
                        case ')': ftok.type = FTK_CLOSEPAR; break;
                        case '[': ftok.type = FTK_OPENSQB; break;
                        case ']': ftok.type = FTK_CLOSESQB; break;
                        case '^': ftok.type = FTK_CARET; break;
                        case '~': ftok.type = FTK_TILDA; break;

                        case '=': ftok.type = FTK_EQUALS; look_ahead = true; break;
                        case '>': ftok.type = FTK_GT; look_ahead = true;  break;
                        case '<': ftok.type = FTK_LT; look_ahead = true; break;
                        case '&': ftok.type = FTK_AMP; look_ahead = true; break;
                        case '|': ftok.type = FTK_BAR; look_ahead = true; break;
                        case '!': ftok.type = FTK_BANG; look_ahead = true; break;
                        default: break;
                    }
                    
                    if(look_ahead) {
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
                                        case FTK_LT: ftok.type = FTK_LE; break;
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
struct function_info {
    int return_type;
    std::vector<int> arg_type;
    unsigned required_args;
    bool repeated;
    //std::string label;
};
static std::map<std::string, function_info> function_table = {
    { "span",     { FTK_STRING,  { FTK_STRING, FTK_INTEGER, FTK_INTEGER }, 3, 0}},
    { "prefix",   { FTK_STRING,  { FTK_STRING, FTK_INTEGER  }, 2, 0 }},
    { "suffix",   { FTK_STRING,  { FTK_STRING, FTK_INTEGER  }, 2, 0 }},
    { "length",   { FTK_INTEGER, { FTK_STRING }, 1, 0 }},
    { "concat",   { FTK_STRING,  { FTK_STRING, FTK_STRING }, 2, 0 }},
    { "size",     { FTK_INTEGER, { 0 }, 1, 0 }},
    { "append",   { 0, { 0 }, 1, 1 }},
    { "join",     { FTK_STRING,  { FTK_STRING, FTK_STRING, FTK_STRING, FTK_STRING }, 2, 0 }}
};

int flow_compiler::compile_fldr(int fldr_node, FieldDescriptor const *left_dp, int left_type, int left_dim) {
    int error_count = 0;
    auto const &fldr = at(fldr_node);
    assert(fldr.type == FTK_fldr);
    assert(fldr.children.size() >= 1);

    // Lookup the function prototype
    std::string fname(get_id(fldr.children[0]));
    auto funp = function_table.find(fname);
    if(funp == function_table.end()) {
        ++error_count;
        pcerr.AddError(main_file, at(fldr_node), sfmt() << "unknown function \"" << fname << "\"");
        return error_count;
    }
    if(funp->second.required_args + 1 !=  fldr.children.size()) {
        ++error_count;
        if(funp->second.required_args == funp->second.arg_type.size())
            pcerr.AddError(main_file, at(fldr_node), sfmt() << "function \"" << fname << "\" takes " << funp->second.required_args << " arguments but " << (fldr.children.size() - 1) << " were given");
        else
            pcerr.AddError(main_file, at(fldr_node), sfmt() << "function \"" << fname << "\" takes at least " << funp->second.required_args << " arguments but " << (fldr.children.size() - 1) << " were given");
        return error_count;
    } 
    for(unsigned n = 1, e = fldr.children.size(); n < e; ++n) {
        switch(at(fldr.children[n]).type) {
            case FTK_fldx: 
                break;
            case FTK_fldr:
                // Left type here must be from function prototype
                error_count += compile_fldr(fldr.children[n], nullptr, 0, 0);
                break;
            case FTK_INTEGER:
            case FTK_STRING:
            case FTK_FLOAT:
                // Check against the function protoype
                break;
            case FTK_dtid:
                // Check against the function protoype
                break;
            case FTK_ID:
                ++error_count;
                pcerr.AddError(main_file, at(fldr.children[n]), sfmt() << "cannot assign to function argument");
                break;    
        }
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
                error_count += compile_fldr(fldd.children[1], fidp, left_type, 0);
            break;
            case FTK_INTEGER:
            case FTK_STRING:
            case FTK_FLOAT:
            case FTK_dtid:
                if(left_type == FTK_fldm) {
                    ++error_count;
                    pcerr.AddError(main_file, at(d), sfmt() << "field \"" << id << "\" is of type message");
                } else if(left_type == FTK_fldm && at(fldd.children[1]).type == FTK_fldm) {

                } else if(left_type != at(fldd.children[1]).type) {
                    pcerr.AddWarning(main_file, at(d), sfmt() << "value will be converted to \"" << node_name(left_type) << "\" before assignment to field \"" << get_name(dp) << "\"");
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
int flow_compiler::compile_node_ref(int id_node) {
    std::string node_name(get_id(id_node));
    Descriptor const *dp = nullptr;
    if(node_name == input_label) {
        dp = input_dp;
    } else {
        auto nnp = named_blocks.find(node_name);
        if(nnp == named_blocks.end()) {
            pcerr.AddError(main_file, at(id_node), sfmt() << "unknown node \"" << node_name << "\"");
            return 1;
        }
        int node_node = nnp->second.second;
        dp = method_descriptor(node_node)->output_type();
    }
    message_descriptor.put(id_node, dp);
    return 0;
}
/*
 * 2nd pass, this should be called after all the message types are set
 * Resolve dimensions for each data referencing node
 */
int flow_compiler::update_dimensions(int node) {
    int error_count = 0;
    auto const &children = at(node).children;
    switch(at(node).type) {
        case FTK_fldx: {
                int xc = 0;
                for(int u = 1, e = children.size(); u < e; ++u) 
                    if(field_descriptor(children[u])->is_repeated())
                        ++xc;
                dimension.put(node, xc);
            }
            break;
        default:
            for(int n: children) 
                error_count += update_dimensions(n);
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
            pcerr.AddError(main_file, at(f), sfmt() << "Unknown field name \"" << id << "\"");
            return error_count + 1;
        }
        field_descriptor.put(f, fidp);
        if(is_message(fidp)) {
            dp = fidp->message_type(); 
        } else if(i + 1 <  fldx.children.size()) {
            pcerr.AddError(main_file, at(f), sfmt() << "The \"" << id << "\" field in \"" << get_name(dp) << "\" is not of message type");
            return error_count + 1;
        }
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
        case FTK_oexp:
            if(at(node).children.size() == 2 && atc(node, 1).type == FTK_ID)
                error_count += compile_node_ref(at(node).children[1]);
            break;
        case FTK_rexp:
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
    int id_node = 0;
    switch(at(node).type) {
        case FTK_fldd:
            if(atc(node, 1).type == FTK_dtid) {
                id_node = at(node).children[1];
                //print_ast(std::cerr, node);
            }
            break;
        case FTK_bexp:
            if(at(node).children.size() == 1 && atc(node, 0).type == FTK_dtid) {
                id_node = at(node).children[0];
                //print_ast(std::cerr, node);
                break;
            }
            // Fall through to check subexpressions
        default:
            for(auto c: at(node).children)
                error_count += compile_id_ref(c);
    }
    if(id_node == 0) 
        return error_count;
    std::string id_label(get_dotted_id(id_node));
    // Find all enums that match this id
    std::set<EnumValueDescriptor const *> matches;

    for(auto evd: enum_value_set) {
        if(evd->name() == id_label || evd->full_name() == id_label || stru1::ends_with(evd->full_name(), std::string(".")+id_label))  {
            if(matches.size() == 0) 
                enum_descriptor.put(id_node, evd);
            matches.insert(evd);
        }
    }
    if(matches.size() == 0) {
        pcerr.AddError(main_file, at(id_node), sfmt() << "unknown enum label \"" << id_label << "\"");
        ++error_count;
    } else if(matches.size() > 1) {
        pcerr.AddError(main_file, at(id_node), sfmt() << "ambiguous enum label \"" << id_label << "\"");
        ++error_count;
    }
    return error_count;
}
// If exp_node is a valid pointer, the block is expected to contain one output (oexp) or return (rexp) definition 
int flow_compiler::compile_block(int blck_node, std::set<std::string> const &output_nvn, int *exp_node) {
    auto const &blck = at(blck_node);
    if(blck.type != FTK_blck) {
        pcerr.AddError(main_file, blck.token, "parameter definition block expected");
        return 1;
    }
    int error_count = 0;
    int exp_node_count = 0;
    // Node values are uniqe so this will allocate a new block:
    block_data_t &block = block_store[blck_node];

    for(int elem_node: blck.children) {
        // sytax checked: 
        //      blck is a list of elem 
        //      each elem has 2 children
        //      the first child is always ID
        //      the second child is either a value (INTEGER, FLOAT or STRING), another [blck/lblk], or an [oexp/rexp] node
        auto const &elem = at(elem_node);
        assert(elem.type == FTK_elem && elem.children.size() == 2); 
        std::string elem_id = get_id(elem.children[0]);

        //assert(!compile_id(elem_id, elem.children[0]));
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
            case FTK_oexp:
            case FTK_rexp:
                // oexp must have 'output' as label and rexp must have 'return'
                // keep count of how many return/output definitions we have seen so far in exp_node_count
                if(exp_node_count == 0 && exp_node != nullptr && contains(output_nvn, elem_id)) {
                    *exp_node = elem.children[1]; ++exp_node_count;
                    type.put(*exp_node, elem_id);
                    // Store, but defer compilation of output/return expression until all the blocks are compiled
                    block.push_back(std::make_pair(elem_id, elem.children[1]));
                    if(value_node_type == FTK_oexp) {
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
    assert(stmt.type == FTK_stmt && stmt.children.size() >= 2);

    // Ignore unless an import statement
    if(at(stmt.children[0]).token.text != "import") 
        return 0;

    std::string filename;
    if(compile_string(filename, stmt.children[1])) {
        pcerr.AddError(main_file, at(stmt.children[1]), "expected filename string");
        return 1;
    }
    int ec = compile_proto(filename);
    if(ec > 0) 
        pcerr.AddError(main_file, stmt.token, sfmt() << "failed to import \"" << filename << "\"");
    return ec;
}
int flow_compiler::compile_stmt(int stmt_node) {
    int exp_node = 0;
    int error_count = 0;
    auto const &stmt = at(stmt_node);
    assert(stmt.type == FTK_stmt && stmt.children.size() >= 2);
    std::string statement;
    compile_id(statement, stmt.children[0]); // sytax checked for ID already
    // Nodes compiled so far
    std::set<int> compiled_nodes;

    if(statement == "import") {
        // ignore imports as they have been processed already

    } else if(statement == "node" || statement == "container") {
        std::string node_name;
        if(compile_method(node_name, stmt.children[1], 1)) {

            pcerr.AddError(main_file, at(stmt.children[1]), std::string("expected \"")+statement+"\" name");
            return 1;
        }
        int node_node = stmt.children[2];
        if(stmt.children.size() < 3) {
            pcerr.AddError(main_file, at(node_node), "parameter definition block expected");
            return 1;
        }
        name.put(node_node, node_name);
        type.put(node_node, statement);
        if(description.has(stmt.children[0]))
            description.update(node_node, description(stmt.children[0]));

        // Look for the action entry in the node block
        if(compile_block(node_node, {"output", "return", "error"}, statement == "node"? &exp_node: nullptr)) {
            //pcerr.AddError(main_file, at(stmt.children[2]), sfmt() << "invalid \"" << statement << "\" parameter definition block");
            return 1;
        }
        if(statement == "node") {
            if(exp_node != 0) {
                // move attributes to the node 
                method_descriptor.copy(exp_node, node_node);
                input_descriptor.copy(exp_node, node_node);
                message_descriptor.copy(exp_node, node_node);
                if(stmt.children.size() > 3)
                    condition.put(node_node, stmt.children[3]);
            }
        }
        // Check this node against other nodes with the same name
        for(int visited: compiled_nodes) if(name(visited) == node_name) {
           
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
            if(message_descriptor.has(visited)  && message_descriptor.has(node_node) &&
               message_descriptor(visited) != message_descriptor(node_node)) {
                pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "node \"" << node_name << "\" redefined with a different output type");
                pcerr.AddNote(main_file, at(visited), "previously defined here");
                return 1;
            }
        } 

        // find the first node with this name
        auto nep = named_blocks.find(node_name);
        if(nep == named_blocks.end()) {
            // Quick access to the block node id
            named_blocks[node_name] = std::make_pair(statement, node_node);
        } else {
            // Keep in the table the node without condition but with an output type
            if((!condition.has(node_node) || !method_descriptor.has(nep->second.second)) && method_descriptor.has(node_node))
                nep->second.second = node_node;
        }
    } else if(statement == "entry") {
        std::string method; 
        if(compile_method(method, stmt.children[1])) {
            pcerr.AddError(main_file, at(stmt.children[1]), "expected method name");
            return 1;
        }
        int node_node = stmt.children[2];
        type.put(node_node, statement);
        if(stmt.children.size() != 3 || compile_block(node_node, {"return"}, &exp_node)) {
            pcerr.AddError(main_file, at(node_node), "parameter definition block expected");
            return 1;
        }
        if(exp_node == 0) {
            pcerr.AddError(main_file, at(node_node), "entry is missing return definition");
            return 1;
        }
        // Check if method is defined in any of the protos
        MethodDescriptor const *mdp = check_method(method, stmt.children[1]);
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
        // Quick access to the block node id 
        named_blocks[method] = std::make_pair(statement, stmt.children[2]);
        //entries[method] = stmt.children[2];
        // All entries must have the same input type
        if(input_dp == nullptr) {
            input_dp = mdp->input_type();
        } else {
            if(input_dp != mdp->input_type()) { 
                error_count += 1;
                pcerr.AddError(main_file, at(stmt.children[1]), sfmt() << "input type must be the same for all entries");
            }
        }
    } else if(statement == "package") {
        if(compile_id(package_name, stmt.children[1])) {
            pcerr.AddError(main_file, at(stmt.children[1]), "expected package name id");
            error_count += 1;
        }
    } else if(statement == "rest") {
        std::string imagename;
        if(compile_string(imagename, stmt.children[1])) {
            pcerr.AddError(main_file, at(stmt.children[1]), "expected image name string");
            return 1;
        }
        if(!rest_image.empty() && imagename != rest_image) 
            pcerr.AddWarning(main_file, at(stmt.children[1]), "rest image re-definition ignored");
        else 
            rest_image = imagename;
    } else if(statement == "repository") {
        std::string repository;
        if(compile_string(repository, stmt.children[1])) {
            pcerr.AddError(main_file, at(stmt.children[1]), "expected repository path string");
            return 1;
        }
        default_repository = repository;
    } else if(statement == "image_pull_secret") {
        std::string secret;
        if(compile_string(secret, stmt.children[1])) {
            pcerr.AddError(main_file, at(stmt.children[1]), "expected image pull secret name");
            return 1;
        }
        image_pull_secrets.insert(secret);
    } else if(statement == "port") {
        if(at(stmt.children[1]).type != FTK_STRING && at(stmt.children[1]).type != FTK_INTEGER) {
            pcerr.AddError(main_file, at(stmt.children[1]), "port value expected");
            return 1;
        }
        base_port = get_integer(stmt.children[1]);
    } else {
        pcerr.AddError(main_file, stmt.token, sfmt() << "token not expected: \"" << statement << "\"");
        error_count += 1;
    }
    
    // If everything went well, visit the exp node
    if(error_count == 0 && type.has(exp_node) && (type(exp_node) == "return" || type(exp_node) == "output")) {
        auto const *mp = method_descriptor(exp_node);
        Descriptor const *d = mp == nullptr? message_descriptor(exp_node): (type(exp_node) == "return"? mp->output_type(): mp->input_type());
        // Set the method descriptor attribute 
        for(auto c: at(exp_node).children) switch(at(c).type) {
            case FTK_fldm:
                message_descriptor.put(c, d); 
                this->name.put(c, get_name(d));
                error_count += compile_fldm(c, d);
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
void flow_compiler::get_fldm_node_refs(std::map<int, std::set<std::string>> &noset, int fldm_node, bool include_input) {
    for(int fldd_node: at(fldm_node).children) switch(atc(fldd_node, 1).type) {
        case FTK_fldm:
            get_fldm_node_refs(noset, at(fldd_node).children[1], include_input);
            break;
        case FTK_fldx:
            if(get_id(atc(fldd_node, 1).children[0]) != input_label) 
                noset[named_blocks.find(get_id(atc(fldd_node, 1).children[0]))->second.second].insert(get_dotted_id(at(fldd_node).children[1], 1));
            else if(include_input)
                noset[0].insert(get_dotted_id(at(fldd_node).children[1], 1));
            break;
        default:
            break;
    }
}
void flow_compiler::get_fldm_node_refs(std::set<int> &noset, int fldm_node, bool include_input) {
    for(int fldd_node: at(fldm_node).children) switch(atc(fldd_node, 1).type) {
        case FTK_fldm:
            get_fldm_node_refs(noset, at(fldd_node).children[1], include_input);
            break;
        case FTK_fldx:
            if(get_id(atc(fldd_node, 1).children[0]) != input_label) 
                noset.insert(named_blocks.find(get_id(atc(fldd_node, 1).children[0]))->second.second);
            else if(include_input)
                noset.insert(0);
            break;
        default:
            break;
    }
}
void flow_compiler::get_bexp_node_refs(std::map<int, std::set<std::string>> &noset, int bexp_node, bool include_input) {
    switch(at(bexp_node).type) {
        case FTK_bexp: switch(at(bexp_node).children.size()) {
            case 1: 
                get_bexp_node_refs(noset, at(bexp_node).children[0], include_input); 
                break;
            case 2:
                get_bexp_node_refs(noset, at(bexp_node).children[1], include_input); 
                break;
            case 3:
                get_bexp_node_refs(noset, at(bexp_node).children[0], include_input); 
                get_bexp_node_refs(noset, at(bexp_node).children[2], include_input); 
                break;
            default:
                assert(false);
        } break;
        case FTK_fldx:
            if(get_id(at(bexp_node).children[0]) != input_label) 
                noset[named_blocks.find(get_id(at(bexp_node).children[0]))->second.second].insert(get_dotted_id(bexp_node, 1));
            else if(include_input) 
                noset[0].insert(get_dotted_id(bexp_node, 1));
            break;
        default:
            break;
    }
}
void flow_compiler::get_bexp_node_refs(std::set<int> &noset, int bexp_node, bool include_input) {
    switch(at(bexp_node).type) {
        case FTK_bexp: switch(at(bexp_node).children.size()) {
            case 1: 
                get_bexp_node_refs(noset, at(bexp_node).children[0], include_input); 
                break;
            case 2:
                get_bexp_node_refs(noset, at(bexp_node).children[1], include_input); 
                break;
            case 3:
                get_bexp_node_refs(noset, at(bexp_node).children[0], include_input); 
                get_bexp_node_refs(noset, at(bexp_node).children[2], include_input); 
                break;
            default:
                assert(false);
        } break;
        case FTK_fldx:
            if(get_id(at(bexp_node).children[0]) != input_label) 
                noset.insert(named_blocks.find(get_id(at(bexp_node).children[0]))->second.second);
            else if(include_input) 
                noset.insert(0);
               
            break;
        default:
            break;
    }
}
std::vector<int> flow_compiler::all_nodes(std::string const &name) const {
    auto f = named_blocks.find(name);
    assert(f != named_blocks.end() && type(f->second.second) == "node");
    return all_nodes(f->second.second);
}
std::vector<int> flow_compiler::all_nodes(int node) const {
    std::vector<int> all;
    
    // Get all the conditional nodes that have the same name
    std::string node_name = name(node);
    for(auto n: condition.nodes()) if(name(n) == node_name) 
        all.push_back(n);
    auto f = named_blocks.find(node_name);
    if(f == named_blocks.end() || type(f->second.second) != "node") {
        std::cerr << node << " id [" << node_name <<  "] ?? " << (f == named_blocks.end())<< "\n";
        assert(false);
    }
    if(!contains(std::set<int>(all.begin(), all.end()), f->second.second)) 
        all.push_back(f->second.second);

    // Get any empty unconditional node with the same name
    /*
    for(auto n: type.nodes()) if(type(n)== "node" && name(n) == node_name && !condition.has(n) && !contains(std::set<int>(all.begin(), all.end()), n)) {
        all.push_back(n);
        break;
    }*/

    return all;
}
// Search a blk node for a rexp or oexp entry and return the arg node
int flow_compiler::get_arg_node(int blck_node) const {
    assert(at(blck_node).type == FTK_blck);
    for(auto elem_node: at(blck_node).children) 
        switch(atc(elem_node, 1).type) {
            case FTK_oexp:
                return atc(elem_node, 1).children[1];
            case FTK_rexp:
                return atc(elem_node, 1).children[0];
            default:
                break;
        }
    return 0;
}
// Grab the nodes referenced by <blck_node>.
// <type> can be either bexp or oexp, or 0 for both.
// <include_input> will also add the input as node 0

std::map<int, std::set<std::string>> &flow_compiler::get_node_refs(std::map<int, std::set<std::string>> &noset, int blck_node, int type, bool include_input) {
    if(blck_node == 0) return noset;
    if((type == 0 || type == FTK_bexp) && condition.has(blck_node)) {
        get_bexp_node_refs(noset, condition(blck_node), include_input);
    }
    if(type == 0 || type == FTK_oexp) {
        int arg_node = get_arg_node(blck_node);
        if(arg_node != 0) switch(at(arg_node).type) {
            case FTK_ID:
                if(get_id(arg_node) != input_label)
                    noset[named_blocks.find(get_id(arg_node))->second.second].insert("");
                else if(include_input)
                    noset[0].insert("");
                break;
            case FTK_fldm:
                get_fldm_node_refs(noset, arg_node, include_input);
                break;
            default: 
                assert(false);
        }
    }
    return noset;
}
int flow_compiler::build_flow_graph(int blk_node) {
    int error_count = 0;
    // All the nodes reachable from the entry
    std::set<int> used_nodes; 

    for(std::vector<int> todo(&blk_node, &blk_node+1); todo.size() > 0;) {
        int cur_node = todo.back(); todo.pop_back();
        if(contains(used_nodes, cur_node))
            continue;
        // add the current node...
        used_nodes.insert(cur_node);
        // queue all the aliases
        if(cur_node != 0 && name.has(cur_node) && !name(cur_node).empty()) for(auto n: all_nodes(name(cur_node))) {
            if(!contains(used_nodes, n)) 
                todo.push_back(n);
        }
        // queue all the references
        std::map<int, std::set<std::string>> noset;
        get_node_refs(noset, cur_node, 0, true);
        for(auto const &ns: noset) 
            if(!contains(used_nodes, ns.first)) 
                todo.push_back(ns.first);
    }

    // All the possible nodes plus input and the entry
    auto node_count = used_nodes.size();
    // Adjacency matrix for this graph
    std::vector<std::vector<bool>> adjmat(node_count, std::vector<bool>(node_count, false));
    std::vector<int> xton(used_nodes.begin(), used_nodes.end());

    if(node_count > 0) {
        std::map<int, unsigned> ntox;
        for(unsigned x = 0; x != node_count; ++x) ntox[xton[x]] = x;
        for(auto const &nx: ntox) {
            std::map<int, std::set<std::string>> noset;
            get_node_refs(noset, nx.first, 0, true);
            for(auto const &ns: noset) {
                adjmat[nx.second][ntox[ns.first]] = true;
                if(ns.first != 0 && !name(ns.first).empty()) for(auto n: all_nodes(name(ns.first)))
                    adjmat[nx.second][ntox[n]] = true;
            }
        }
    }

    std::vector<std::set<int>> &graph = flow_graph[blk_node];
    std::set<int> solved;
    // Mark the input as solved
    solved.insert(0);

    // When every node is in a separate stage there will be at most mx-2 stages.
    // Each iteration attempts to build a stage. If the stage is empty we are either 
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
            // Successfully finised.
            // Could actually return from here...
            // return 0;
            break;
        }
        //std::cerr << "stage: " << 1+i << ": " << graph.back() << "\n";
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
    } else {
        pcerr.AddError(main_file, at(error_node), sfmt() << "conversion from \"" << right.type_name() << "\" to  \"" << left.type_name() << " could cause data loss\"");
        check = 4;
    }
    //std::cerr << "Check " << left.type_name() << "/" << left.grpc_type_name() << " <- " << right.type_name() <<  "/" << right.grpc_type_name() << " => " << check << "\n";
    return check;
}
static 
op get_conv_op(int r_type, int l_type, int r_grpc_type, int l_grpc_type) {
    std::cerr << "convert " << r_type << " (" << grpc_type_name((google::protobuf::FieldDescriptor::Type) r_grpc_type) << ") to " << l_type << " (" << grpc_type_name((google::protobuf::FieldDescriptor::Type) l_grpc_type) << ")\n";
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
                case FTK_dtid:
                    return COIE;
                default:
                    std::cerr << "Need to convert integer to " << r_type << "(" << r_grpc_type << ")!!\n";
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
                case FTK_dtid:
                    return COFE;
                default:
                    std::cerr << "Need to convert float to " << r_type << "(" << r_grpc_type << ")!!\n";
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
                case FTK_dtid:
                    return COSE;
                default:
                    std::cerr << "Need to convert string to " << r_type << "(" << r_grpc_type << ")!!\n";
            }
            break;
        case FTK_dtid:
            switch(l_type) {
                case FTK_INTEGER:
                    if(l_grpc_type == google::protobuf::FieldDescriptor::Type::TYPE_BOOL) 
                        return COEB;
                    return COEI;
                case FTK_FLOAT:
                    return COEF;
                case FTK_STRING:
                    return COES;
                case FTK_dtid:
                    return COEE;
                default:
                    std::cerr << "Need to convert enum to " << r_type << "(" << r_grpc_type << ")!!\n";
            }
            break;
        default: 
            std::cerr << "Asked fot conv: " << l_type << " to " << r_type << " (" << l_grpc_type << ", " << r_grpc_type << ")\n";
    }
    return NOP;
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
 * lvfd: descriptor associated with the case number 3 od lv_name
 * arg_node: current node
 * node_ip: node to icode address 
 *
 */
int flow_compiler::populate_message(std::string const &lv_name, lrv_descriptor const &lvd, int arg_node, std::map<int, int> &node_ip) {
    int error_count = 0;
    if(lvd.is_repeated()) 
        icode.push_back(fop(LOOP, lv_name, lvd.dp));

    assert(arg_node != 0);
    switch(at(arg_node).type) {
        case FTK_fldm: {
            for(int fldd_node: at(arg_node).children) {
                auto fidp = field_descriptor(fldd_node);
                error_count += populate_message(lv_name+"+"+name(fldd_node), lrv_descriptor(lvd, fidp), at(fldd_node).children[1], node_ip);
            }
        } break;
        case FTK_fldr: {
            auto const &children = at(arg_node).children;
            std::string fname(get_id(children[0]));
            auto ftp = function_table.find(fname);

            // TODO generate call for this 
            for(unsigned i = 1, e = children.size(); i != e; ++i) {
                icode.push_back(fop(SETT, lv_name, lvd.dp, arg_node));
                std::string tmpvarname = sfmt() << "TmpVar_" << children[i];
                lrv_descriptor arg_lvd(ftp->second.return_type);
                populate_message(tmpvarname, arg_lvd, children[i], node_ip);
            }

            // Check the return type against the left value type
            if(ftp->second.return_type > 0) {
                error_count += check_assign(arg_node, lvd, lrv_descriptor(ftp->second.return_type))? 0 :1;
            } else {

            }
            icode.push_back(fop(FUNC, fname, nullptr, arg_node));
            for(unsigned i = 1, e = children.size(); i != e; ++i)
                icode.back().arg.push_back(children[i]);
            icode.push_back(fop(SETT, lv_name, lvd.dp, arg_node));
        } break;
        case FTK_fldx: {
            auto const &fields = at(arg_node).children;
            // TODO improve name generation
            int rvn = get_id(fields[0]) == input_label? 0: named_blocks.find(get_id(fields[0]))->second.second;
            auto const rv_name = get_id(fields[0]) == input_label? cs_name("", 0): cs_name("RS", name(rvn));
            auto const rvd = message_descriptor(rvn);
            unsigned ri = 0;

            if(fields.size() > 1) {
                auto const rvfd = lrv_descriptor(rvd, field_descriptor(fields.back()));
                int chk = check_assign(arg_node, lvd, rvfd);
                error_count += chk? 0:1;

                // if left and right are descriptors we copy
                if(is_message(field_descriptor(fields.back())) && lvd.dp != nullptr) {
                    icode.push_back(fop(COPY, lv_name, rv_name, lvd.dp, rvd));
                } else {
                    ri = icode.size();
                    icode.push_back(fop(RVA, rv_name, rvfd.grpc_type_name(), rvd)); 
                    op coop = get_conv_op(rvfd.type(), lvd.type(), rvfd.grpc_type(), lvd.grpc_type());
                    if(coop != NOP && !(coop == COEE && lvd.enum_descriptor() == rvfd.enum_descriptor())) {
                        icode.push_back(fop(coop, lvd.grpc_type(), rvfd.grpc_type()));
                        if(lvd.type() == FTK_dtid) icode.back().el = lvd.enum_descriptor();
                        if(rvfd.type() == FTK_dtid) icode.back().er = rvfd.enum_descriptor();
                    }
                    icode.push_back(fop(SETL, lv_name, lvd.grpc_type_name(), lvd.dp));
                    icode.push_back(fop(SET, lv_name, rv_name, lvd.dp, rvd));
                }
            } else {
                error_count += check_assign(arg_node, lvd, lrv_descriptor(rvd)) == 2? 0: 1;
                icode.push_back(fop(COPY, lv_name, rv_name, lvd.dp, rvd));
            }
            // Remember the address of this copy instruction
            unsigned ci = icode.size()-1;
            std::vector<int> idxp;
            std::string rv_fields;
            
            for(unsigned i = 1, e = fields.size(); i < e; ++i) {
                // Append all the field names to the right value label
                auto fid = field_descriptor(fields[i]);
                rv_fields += "+";
                rv_fields += get_id(fields[i]);

                if(fid->is_repeated()) {
                    fop idx(INDX, rv_name, rvd);
                    idx.arg.assign(fields.begin(), fields.begin()+i+1);

                    // Add this index to the code if it doesn't exist
                    // and store the location in the idxp vector
                    unsigned xidx = 0;
                    for(unsigned u = 0; u < icode.size(); ++u) {
                        fop const &xop = icode[u];
                        if(fop_compare(idx, xop) == 0) {
                            xidx = u+1;
                            break;
                        }
                    }
                    if(xidx == 0) {
                        icode.push_back(idx);
                        xidx = icode.size();
                    }
                    idxp.push_back(xidx);
                }
            }
            icode[ci].arg2 = rv_name + rv_fields;
            if(ri != 0) icode[ri].arg1 = rv_name + rv_fields;
            // Propagate the indices to the embedding loops 
            auto ip = icode.rbegin(), ie = icode.rend();
            for(auto p = idxp.crbegin(), e = idxp.crend(); p != e; ++p) {
                ip = find_next_xcode(ip, ie);
                if(ip != ie) {
                    // Add this index (i.e. *p) to the list if it's not there already
                    if(std::find(ip->arg.begin(), ip->arg.end(), *p) == ip->arg.end())
                        ip->arg.push_back(*p);
                    ++ip;
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(arg_node), sfmt() << "cannot assign repeated value into a single value field");
                    break;
                }
            }
            // Now propagate any node indices found
            auto nipp = node_ip.find(rvn);
            if(nipp != node_ip.end()) {
                int begin = nipp->second+1;
                while(begin < icode.size() && icode[begin].code == NSET) ++begin;
                for(--begin; begin != nipp->second; --begin) if(icode[begin].arg.size() > 0) {
                    ip = find_next_xcode(ip, ie);
                    if(ip != ie) {
                        // Add this index (i.e. *p) to the list if it's not there already
                        for(auto p: icode[begin].arg)
                            if(std::find(ip->arg.begin(), ip->arg.end(), p) == ip->arg.end())
                                ip->arg.push_back(p);
                        ++ip;
                    } else {
                        ++error_count;
                        pcerr.AddError(main_file, at(arg_node), sfmt() << "cannot assign repeated value into a single value field");
                        break;
                    }
                }
            }
            // if the right value node has any index assigned add it here
            //if(nipp != node_ip.end()) idxp.assign(icode[nipp->second].arg.begin(), icode[nipp->second].arg.end());
        } break;
        case FTK_ID: {
            // Single id reference this is a particular case of fldx but the left value will always be a message
            int rvn = get_id(arg_node) == input_label? 0: named_blocks.find(get_id(arg_node))->second.second;
            auto const rvd = message_descriptor(rvn);
            error_count += check_assign(arg_node, lvd.dp, lrv_descriptor(rvd))? 0: 1;

            icode.push_back(fop(COPY, lv_name, cs_name("RS", rvn), lvd.dp, rvd));
        } break;
        case FTK_STRING: {
            icode.push_back(fop(RVC, get_value(arg_node), arg_node, (int)google::protobuf::FieldDescriptor::Type::TYPE_STRING)); 
            op coop = get_conv_op(FTK_STRING, lvd.type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING, lvd.grpc_type());
            if(coop != NOP) {
                icode.push_back(fop(coop, lvd.grpc_type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING));
                if(lvd.type() == FTK_dtid) icode.back().el = lvd.enum_descriptor();
            }
            icode.push_back(fop(SETL, lv_name, lvd.grpc_type_name(), lvd.dp));
            icode.push_back(fop(SETS, lv_name, lvd.dp, arg_node));
        } break;
        case FTK_INTEGER: {
            icode.push_back(fop(RVC, get_value(arg_node), arg_node, (int) google::protobuf::FieldDescriptor::Type::TYPE_INT64)); 
            op coop = get_conv_op(FTK_INTEGER, lvd.type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_INT64, lvd.grpc_type());
            if(coop != NOP) {
                icode.push_back(fop(coop, lvd.grpc_type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_INT64));
                if(lvd.type() == FTK_dtid) icode.back().el = lvd.enum_descriptor();
            }
            icode.push_back(fop(SETL, lv_name, lvd.grpc_type_name(), lvd.dp));
            icode.push_back(fop(SETI, lv_name, lvd.dp, arg_node));
        } break;
        case FTK_FLOAT: {
            icode.push_back(fop(RVC, get_value(arg_node), arg_node, (int) google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE)); 
            op coop = get_conv_op(FTK_FLOAT, lvd.type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE, lvd.grpc_type());
            if(coop != NOP) {
                icode.push_back(fop(coop, lvd.grpc_type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE));
                if(lvd.type() == FTK_dtid) icode.back().el = lvd.enum_descriptor();
            }
            icode.push_back(fop(SETL, lv_name, lvd.grpc_type_name(), lvd.dp));
            icode.push_back(fop(SETF, lv_name, lvd.dp, arg_node));
        } break;
        case FTK_dtid: {
            icode.push_back(fop(RVC, get_dotted_id(arg_node), arg_node, (int) google::protobuf::FieldDescriptor::Type::TYPE_ENUM )); 
            icode.back().er = enum_descriptor(arg_node)->type();
            op coop = get_conv_op(FTK_dtid, lvd.type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_ENUM, lvd.grpc_type());

            if(coop != NOP && !(coop == COEE && enum_descriptor(arg_node) != nullptr && lvd.enum_descriptor() == enum_descriptor(arg_node)->type())) {
                icode.push_back(fop(coop, lvd.grpc_type(), (int) google::protobuf::FieldDescriptor::Type::TYPE_ENUM));
                icode.back().er = enum_descriptor(arg_node)->type();
                if(lvd.type() == FTK_dtid) {
                    icode.back().el = lvd.enum_descriptor();
                    // TODO check if value is compatible with enum
                }
            }
            
            icode.push_back(fop(SETL, lv_name, lvd.grpc_type_name(), lvd.dp));
            icode.push_back(fop(SETE, lv_name, lvd.dp, arg_node));
        } break;
        default:
            print_ast(std::cerr, arg_node);
            assert(false);
    }
    if(lvd.is_repeated()) 
        icode.push_back(fop(ELP));
    return error_count;
}
/**
 * Return the maximum number of indices that a right value field uses.
 * If max_allowed is not negative, print a note message wherever the depth exceeds max_allowed.
 */
int flow_compiler::find_max_index_depth(int right_value_node, std::map<int, int> const &node_ip, int max_allowed) {
    if(right_value_node == 0) return 0;
    auto const &node = at(right_value_node);
    int xc = 0;
    switch(node.type) {
        case FTK_fldm: {
            int max = 0;
            // Return the max index of any field on the right side of the fldd
            for(int fldd_node: node.children) 
                max = std::max(max, find_max_index_depth(at(fldd_node).children[1], node_ip, max_allowed));
            // Avoid printing notes here
            return max;
        }
        case FTK_fldx: {
            auto const &ids = node.children;
            xc = find_max_index_depth(ids[0], node_ip, -1);
            for(int u = 1, e = ids.size(); u < e; ++u) 
                if(field_descriptor(ids[u])->is_repeated())
                    ++xc;
        } break;
        case FTK_ID: {
            int node_node = named_blocks.find(get_id(right_value_node))->second.second;
            auto nip = node_ip.find(node_node);
            if(nip == node_ip.end()) return 0;
            int i = nip->second+1, e = icode.size();
            xc = 0;
            while(i < e && icode[i].code == NSET) 
                if(icode[i++].arg.size() > 0) ++xc;
        } break;
        case FTK_fldr: {
            int max = 0;
            // Return the max index of any field on the right side of the fldd
            for(int arg_ni = 1, arg_sz = node.children.size(); arg_ni < arg_sz; ++arg_ni) 
                max = std::max(max, find_max_index_depth(node.children[arg_ni], node_ip, max_allowed));
            // Avoid printing notes here
            return max;
        }
        case FTK_STRING: case FTK_FLOAT: case FTK_INTEGER:
            break;
        case FTK_dtid:
            // Must be an enum ref
            break;
        default: 
            print_ast(std::cerr, right_value_node);
            assert(false);
    }
    if(max_allowed >= 0 && xc > max_allowed) 
        pcerr.AddNote(main_file, at(right_value_node), sfmt() << "field \"" << get_dotted_id(right_value_node) << "\" has dimension \"" << xc << "\"");
    return xc;
}

int flow_compiler::compile_flow_graph(int entry_blck_node, std::vector<std::set<int>> const &fg, std::set<int> const &node_set) {
    int error_count = 0;

    std::map<int, int> node_ip;
    auto emd = method_descriptor(entry_blck_node);
    std::string return_name = cs_name("RS", entry_blck_node);

    std::vector<std::set<int>> node_stages(fg);

    TRACE << "Processing entry " << entry_blck_node << ": " << get_name(emd) << "(" <<  cs_name("", 0) << "): " << return_name << "\n";

    // MTHD marks the beggining of a method implementation.
    // The list of all nodes that can be visited by this method is stored in the args (in source order).
    icode.push_back(fop(MTHD, cs_name("", 0), return_name, emd, emd->input_type(), emd->output_type()));
    std::set<int> method_set;
    for(auto const &stage_set: node_stages) 
        for(auto n: stage_set) method_set.insert(n);
    for(auto n: method_set) icode.back().arg.push_back(n);      // MTHD arg 1... nodes used by this entry
    std::set<int> visited;

    // Reorder tnodes and label them accordingly
    std::map<std::string, int> node_order;
    for(auto const &stage_set: node_stages) 
        for(int n: stage_set) if(condition.has(n)) {
            if(contains(node_order, name(n))) {
                ++node_order[name(n)];
            } else {
                node_order[name(n)] = 1;
            }
            auto &mni = referenced_nodes.find(n)->second;
            mni.order = node_order[name(n)];
            mni.name = sfmt() << name(n) << "-" << mni.order;
        }

    // Add any default error nodes that would otherwise be left out
    /*
    std::set<std::string> def_err_set;
    for(auto s = node_stages.rbegin(), e = node_stages.rend(); s != e; ++s) {
        std::set<int> added;
        for(auto n: *s) if(!contains(def_err_set, name(n))) {
            for(auto p: type.nodes()) if(type(p) == "node" && name(n) == name(p) && !condition.has(p) && !message_descriptor.has(p)) {
                std::cerr << "would like to add node " << p << ", " << name(p) << " to " << *s << "\n";
                def_err_set.insert(name(p));
                added.insert(p);
            }
        }
        if(added.size() > 0) s->insert(added.begin(), added.end());
    }
    */
    
    int stage = 0;
    for(auto const &stage_set: node_stages) {
        ++stage;
        //std::cerr << "NOW in STAGE " << stage << ": " << stage_set  << "\n";
        int stage_idx = icode.size();
        int stage_dim = 0; 
        // BSTG marks the beginning of an execution stage. 
        // The stage set contains all the nodes to be processed in this stage in the order 
        // they were declared in the source file.
        // At this point all the inputs needed by this node set are known and calls to nodes
        // in the same stage can be made in parallel.

        icode.push_back(fop(BSTG, stage));                    // BSTG arg 1: stage number
        icode[stage_idx].arg.push_back(stage_set.size());     // BSTG arg 2: the number of nodes in this stage
        icode[stage_idx].arg.push_back(0);                    // BSTG arg 3: max node dimension (placeholder)

        std::vector<int> stage_nodes;
        std::vector<std::string> stage_set_names;

        // Every default node (i.e. node without condition) has to be moved after all conditional nodes with the same name.
        std::set<std::string> distinct_names;    
        for(int n: stage_set) {
            icode[stage_idx].arg.push_back(n);   // BSTG arg 4... nodes in this stage

            std::string nn(name(n));             // default name for this node

            // Process nodes named the same in the order they appear in the source
            if(contains(distinct_names, nn))
                continue;
            distinct_names.insert(nn);

            // First, conditional names
            int order = 0;
            for(int m: stage_set) if(condition.has(m) && name(m) == nn) {
                stage_nodes.push_back(m);
                auto &mni = referenced_nodes.find(m)->second;
                //mni.order = ++order;
                //mni.name = sfmt() << nn << "-" << order; 
                stage_set_names.push_back(mni.name);
            }
            
            for(int m: stage_set) if(!condition.has(m) && name(m) == nn) {
                stage_nodes.push_back(m);
                stage_set_names.push_back(nn);
            }
        }
        
        TRACE << "Creating stage set name: " << join(stage_set_names, ", ") << "\n";
        icode[stage_idx].arg1 = join(stage_set_names, ", ");  // label for this node set
        
        for(int node: stage_nodes) {
            auto md = method_descriptor(node);
            if(!method_descriptor.has(node)) md = nullptr;
            auto input_type = input_descriptor(node);
            auto output_type = message_descriptor(node);
            if(!message_descriptor.has(node)) output_type = nullptr;
            if(!input_descriptor.has(node)) input_type = output_type;

            int base_node = named_blocks.find(name(node))->second.second;

            std::string rs_node(cs_name("RS", name(base_node) ));
            std::string rq_node(cs_name("RQ", node));

            int node_idx = icode.size();
            node_ip[node] = node_idx;

            icode.push_back(fop(BNOD, rs_node, rq_node, output_type, input_type));

            // Should max index depth also look at condition's dimension?
            for(int i = 0, e = find_max_index_depth(get_arg_node(node), node_ip, -1); i != e; ++i)
                icode.push_back(fop(NSET));

            if(condition.has(node))
                icode.push_back(fop(NDIF, name(node), node, base_node, condition(node))); 
            else
                icode.push_back(fop(BNIF, name(node), node, base_node, 0)); 

            if(output_type != nullptr) {
                // Populate the request 
                error_count += populate_message(rq_node, lrv_descriptor(input_type), get_arg_node(node), node_ip);

                if(md != nullptr) 
                    icode.push_back(fop(CALL, rq_node, rs_node, md));
                else 
                    icode.push_back(fop(COPY, rs_node, rq_node, output_type, input_type));

            } else {
                // This is either an empty or an error node
                icode.push_back(fop(NOP));
            }

            icode.back().arg.push_back(node);
            icode.push_back(fop(ENOD, rs_node));
            // update the BNOD with the number of indices (the dimension of the result)
            int dim = 0;
            for(int p = node_idx+1; p < icode.size() && icode[p].code == NSET; ++p) 
                dim += icode[p].arg.size() > 0? 1: 0;
            icode[node_idx].arg.push_back(dim);
            icode[node_idx].arg.push_back(node);
            icode[node_idx].arg.push_back(stage);
            stage_dim = std::max(stage_dim, dim);
            if(!contains(visited, base_node)) {
                visited.insert(base_node);
                icode[node_idx].arg.push_back(base_node);
            } else {
                icode[node_idx].arg.push_back(0);
            }
        }
        icode.push_back(fop(ESTG, icode[stage_idx].arg1, stage));
        icode[stage_idx].arg[2] =  stage_dim;                 // BSTG arg 3: max node dimension 
    }

    // Generate code to populate the Response 
    icode.push_back(fop(BPRP, entry_blck_node));
    icode.back().d1 = emd->output_type();
    icode.back().m1 = emd;

    int entry_arg_node = get_arg_node(entry_blck_node); 
    int rdepth = find_max_index_depth(entry_arg_node, node_ip, -1);

    int ldepth = at(entry_arg_node).type == FTK_ID? 0: get_index_depth(emd->output_type());

    TRACE << "Preparing for reply set: entry_arg_node: " << entry_arg_node << ", ldepth: " << ldepth << ", rdepth: " << rdepth << "\n";
    if(trace_on) print_ast(std::cerr, entry_arg_node);

    if(ldepth < rdepth) {
        ++error_count;
        pcerr.AddError(main_file, at(entry_arg_node), sfmt() << "in entry \"" << emd->full_name() << "\" the return expression has a higher dimension \""
                << rdepth << "\" than the output type \"" << emd->output_type()->full_name() << "\": \"" << ldepth << "\"");
        // Call this again to display notes
        find_max_index_depth(get_arg_node(entry_blck_node), node_ip, ldepth);
    }

    for(int i = 0; i < rdepth; ++i)
        icode.push_back(fop(NSET));

    error_count += populate_message(return_name, lrv_descriptor(emd->output_type()), entry_arg_node, node_ip);
    icode.push_back(fop(EPRP));
    icode.push_back(fop(END));

    return error_count;
}
void flow_compiler::dump_code(std::ostream &out) const {
    int l = 0;
    for(auto const &s: icode) 
        std::cerr << ++l << " "  << s << "\n";
}
int flow_compiler::compile(std::set<std::string> const &targets) {
    int root = ast_root();
    if(at(root).type != FTK_ACCEPT || at(root).children.size() != 1)
        return 1;
    root = at(root).children[0];
    if(at(root).type != FTK_flow || at(root).children.size() < 1)
        return 1;

    int error_count = 0;

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
        pcerr.AddError(main_file, at(node), sfmt() << "Cannot determine output type for node \""<< name(node) <<"\n");
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
    // Update dimensions for each data referencing node
    error_count += update_dimensions(root);
    if(error_count > 0) return error_count;

    for(auto const &ep: named_blocks) if(type(ep.second.second) == "entry") {
        // Build the flow graph for each entry 
        error_count += build_flow_graph(ep.second.second);
    }

    icode.clear();
    if(error_count > 0) return error_count;

    if(flow_graph.size() == 0) {
        pcerr.AddError(main_file, -1, 0, sfmt() << "no entries defined");
        return 1;
    } 

    for(auto const &gv: flow_graph) {
        auto const &e = at(gv.first);
        std::set<int> entry_referenced_nodes;
        std::stringstream sout;
        sout << "entry(" << e.token.line << "," << e.token.column << ")";
        for(auto const &ss: gv.second) {
            std::vector<std::string> ns;
            for(int n: ss) if(n != gv.first) {
                referenced_nodes.emplace(n, node_info(n, (!condition.has(n)? name(n): sfmt() << name(n) << "-" << at(n).token.line << "-" << at(n).token.column), method_descriptor(n)==nullptr));
                entry_referenced_nodes.insert(n);
                ns.push_back(referenced_nodes.find(n)->second.name);
            }
            sout << "/" << join(ns, ",");
        }
        if(verbose && entry_referenced_nodes.size() == 0) 
            pcerr.AddWarning(main_file, e, sfmt() << "entry \""<< method_descriptor(gv.first)->full_name()<< "\" doesn't use any nodes");
        else if(verbose) 
            pcerr.AddNote(main_file, e, sfmt() << "node sequence: " << sout.str());
        // Mark the entry point 
        entry_ip[gv.first] = icode.size();
        error_count += compile_flow_graph(gv.first, gv.second, entry_referenced_nodes);
    }
    for(auto ne: named_blocks) if(ne.second.first == "node" && !contains(referenced_nodes, ne.second.second)) 
        pcerr.AddWarning(main_file, at(ne.second.second), sfmt() << "node \"" << ne.first << "\" is not used by any entry");
    //std::cerr << referenced_nodes << "\n";

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
    /*
    if(fields.size() > 3) {
        std::set<std::string> f3; 
        int c = 0;
        for(auto f: fields) if(c++ < 3) 
            f3.insert(f);
        else 
            break;
        return std::make_pair(join(f3, ",\n") + "...", ftip);
    }
    */
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
                    out << c_escape(inf.name) << "[label=<" << name(nn) << "<sup><font point-size=\"7\">" << inf.order << "</font></sup><br/><font point-size=\"7\" face=\"Courier\">" << html_escape(to_text(condition(nn))) << "</font>>]; ";
                else
                    out << c_escape(inf.name) << "[label=<" << name(nn) << ">]; ";
            }
            out << s << ";\n};\n";
        }
        for(auto nn: n) {
            std::string dot_node(c_escape(referenced_nodes.find(nn)->second.name));
            // Get all incoming edges
            incoming.clear();
            for(auto i: get_node_refs(incoming, nn, FTK_oexp, true))
                if(i.first == 0) {
                    out << input_label << " -> " << dot_node << " [fontsize=9,style=bold,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
                } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
                    std::string dot_i(c_escape(j.second.name)); 
                    out << dot_i << " -> " << dot_node << " [fontsize=9,style=bold,label=\"" << make_label(i.second, method_descriptor(i.first)->output_type()) << "\"];\n";
                }
            
            incoming.clear();
            for(auto i: get_node_refs(incoming, nn, FTK_bexp, true)) 
                if(i.first == 0) {
                    out << input_label << " -> " << dot_node << " [fontsize=9,style=dashed,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
                } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
                    std::string dot_i(c_escape(j.second.name)); 
                    out << dot_i << " -> " << dot_node << " [fontsize=9,style=dashed,label=\"" << make_label(i.second, method_descriptor(i.first)->output_type()) << "\"];\n";
                }
        }
    }
    out << "node [shape=invtriangle];\n";
    out << "{ rank = same; " << ename << "[label=" << c_escape(method_descriptor(entry)->name()) << "]; \"[o]\"; };\n";
    incoming.clear();
    for(auto i: get_node_refs(incoming, entry, FTK_oexp, true)) 
        if(i.first == 0) {
            out << input_label << " -> " << ename << " [fontsize=9,style=bold,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
        } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
            std::string dot_i(c_escape(j.second.name)); 
            out << dot_i << " -> " << ename << " [fontsize=9,style=bold,color=dodgerblue2,label=\"" << make_label(i.second, method_descriptor(i.first)->output_type()) << "\"];\n";
        }
    out << "}\n";
}

