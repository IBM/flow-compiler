#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/stat.h>

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wlogical-op-parentheses"
#endif

#include "flow-compiler.H"
#include "stru.H"
#include "cot.H"
#include "flow-parser.c"
#include "massert.H"
#include "filu.H"

/**
 * Add the directory to the proto path
 * Return 0 on success or 1 if can't access directory
 */
int flow_compiler::add_to_proto_path(std::string const &directory, std::string const &mapped_to) {
    struct stat sb;
    if(stat(directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) 
        return 1;
    source_tree.MapPath(mapped_to, directory);
    grpccc += "-I"; grpccc += directory; grpccc += " ";
    protocc += "-I"; protocc += directory; protocc += " ";
    return 0;
}

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
int flow_compiler::compile_proto(std::string const &file, int map_file_dir) {
    auto fdp = importer.Import(file);
    if(fdp == nullptr && map_file_dir != 0) {
        add_to_proto_path(filu::dirname(file));
        fdp = importer.Import(file);
    }
    if(fdp == nullptr) {
        pcerr.AddError(file, -1, 0, "import failed");
        return 1;
    }
    if(fdp->syntax() != FileDescriptor::Syntax::SYNTAX_PROTO3) {
        pcerr.AddError(file, -1, 0, "syntax must be proto3");
        return 1;
    }
    std::set<FileDescriptor const *> included(fdps.begin(), fdps.end());
    if(cot::contains(included, fdp)) return 0;
    fdps.push_back(fdp);
    included.insert(fdp);
    get_enums(enum_value_set, fdp);
    // Add the dependents to our set
    for(int i = 0, e = fdp->dependency_count(); i != e; ++i) {
        auto fdq = fdp->dependency(i);
        if(!cot::contains(included, fdq)) {
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

    return stru::join(l_names, sep, last, begin, prefix, suffix);
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
    return stru::join(l_names, sep, last, begin, prefix, suffix);
}
static bool is_white_punct(std::string const &s) {
    for(auto c: s)
        if(!isspace(c) && !ispunct(c))
            return false;
    return true;
}
static bool is_white(std::string const &s) {
    for(auto c: s)
        if(!isspace(c))
            return false;
    return true;
}
/**
 * Preserve all comments that have @text references. 
 */
void flow_compiler::add_comments(int token, std::vector<std::string> const &comments) {
    // filter comments 
    unsigned b = 0;
    while(b < comments.size() && (
       comments[b].empty() || 
       is_white_punct(comments[b]) ||
       // remove copyright message from the top of the file
       (stru::to_lower(comments[b]).find("copyright") != std::string::npos && token == 1)
       )) ++b;

    std::stringstream buf1, buf2;
    for(unsigned i = b, e = comments.size(); i < e; ++i) 
        buf1 << stru::strip(comments[i]) << "\n";
    std::string line; 
    bool front = true;
    while(!!std::getline(buf1, line)) {
        line = stru::rstrip(nullptr, line);
        if(front && is_white_punct(line))
            continue;
        front = false;
        buf2 << line << "\n";
    }
    if(!buf2.str().empty()) 
        description.put(token, stru::strip(buf2.str()));
}
int flow_compiler::parse() {
    int error_count = 0;
    DEBUG_ENTER;
    io::ZeroCopyInputStream *zi = source_tree.Open(main_file);
    if(zi == nullptr) {
        pcerr.AddError(main_file, -1, 0, "can't read file");
        ++error_count;
        DEBUG_LEAVE;
        return error_count;
    }
    ErrorPrinter ep(pcerr, main_file);
    io::Tokenizer tokenizer(zi, &ep);
    yyParser *fpp = (yyParser *) flow_parserAlloc(malloc);
    if(trace_on)
        flow_parserTrace(stderr, (char *) "flow: ");
    flow_token ftok;
    ftok.type = 0; // In case tokenizer fails

    std::string prev_trailing, next_leading;
    std::vector<std::string> detached;
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
        if(!next_leading.empty()) detached.push_back(next_leading);
        if(!prev_trailing.empty()) detached.insert(detached.begin(), prev_trailing);
        add_comments(ntok, detached);
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
    DEBUG_LEAVE;
    return error_count;
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
