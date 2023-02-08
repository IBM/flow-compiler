#include <fstream>
#include <cctype>

#include "filu.H"
#include "flowcomp.H"
#include "gru.H"

#include "flow-parser.c"

static bool is_white_punct(std::string const &s) {
    for(auto c: s)
        if(!isspace(c) && !ispunct(c))
            return false;
    return true;
}

fc::flow_compiler::flow_compiler(): fc::proto_compiler() {
}
/**
 * Preserve all comments that have @text references. 
 */
void fc::flow_compiler::add_comments(int token, std::vector<std::string> const &comments) {
    // filter comments 
    unsigned b = 0;
    while(b < comments.size() && (
       comments[b].empty() || 
       is_white_punct(comments[b]) ||
       // remove copyright message from the top of the file
       (stru1::to_lower(comments[b]).find("copyright") != std::string::npos && token == 1)
       )) ++b;

    std::stringstream buf1, buf2;
    for(unsigned i = b, e = comments.size(); i < e; ++i) 
        buf1 << stru1::strip(comments[i]) << "\n";
    std::string line; 
    bool front = true;
    while(!!std::getline(buf1, line)) {
        line = stru1::rstrip(nullptr, line);
        if(front && is_white_punct(line))
            continue;
        front = false;
        buf2 << line << "\n";
    }
    if(!buf2.str().empty()) 
        ast.description.put(token, stru1::strip(buf2.str()));
}
int fc::flow_compiler::compile_flow(std::string file, bool trace_on) {
    std::string disk_file;
    flow_file = file;

    if(!source_tree.VirtualFileToDiskFile(file, &disk_file)) {
        std::string file_dir = filu::dirname(file);
        add_to_proto_path(file_dir.empty()? std::string("."): file_dir);
        flow_file = filu::basename(file);
    }

    int error_count = 0;
    google::protobuf::io::ZeroCopyInputStream *zi = source_tree.Open(flow_file);
    if(zi == nullptr) {
        AddError(-1, 0, "can't read file");
        ++error_count;
        return error_count;
    }
    ErrorPrinter ep(pcerr, flow_file);
    google::protobuf::io::Tokenizer tokenizer(zi, &ep);
    yyParser *fpp = (yyParser *) flow_parserAlloc(malloc);
    if(trace_on)
        flow_parserTrace(stderr, (char *) "flow: ");
    token ftok;
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
            case google::protobuf::io::Tokenizer::TokenType::TYPE_START:
                ftok.type = 0;
                ++error_count;
                AddError(ftok, "tokenizer internal error");
                break;
            case google::protobuf::io::Tokenizer::TokenType::TYPE_END:
                ftok.type = 0;
                break;
            case google::protobuf::io::Tokenizer::TokenType::TYPE_IDENTIFIER:
                ftok.type = FTK_ID;
                ftok.text = token.text;
                break;
            case google::protobuf::io::Tokenizer::TokenType::TYPE_INTEGER:
                ftok.type = FTK_INTEGER;
                ftok.text = token.text;
                if(!google::protobuf::io::Tokenizer::ParseInteger(token.text, UINT64_MAX, (uint64_t *) &ftok.integer_value)) {
                    // Integer overflow
                    ++error_count;
                    AddError(ftok, "integer value overflow");
                }
                break;
            case google::protobuf::io::Tokenizer::TokenType::TYPE_FLOAT:
                ftok.type = FTK_FLOAT;
                ftok.text = token.text;
                ftok.float_value = google::protobuf::io::Tokenizer::ParseFloat(token.text);
                break;
            case google::protobuf::io::Tokenizer::TokenType::TYPE_STRING:
                ftok.type = FTK_STRING;
                ftok.text.clear();
                google::protobuf::io::Tokenizer::ParseString(token.text, &ftok.text);
                break;
            case google::protobuf::io::Tokenizer::TokenType::TYPE_SYMBOL:
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
                            if(next.type == google::protobuf::io::Tokenizer::TokenType::TYPE_SYMBOL && next.text.length() == 1) switch(next.text[0]) {
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
                AddError(ftok, "unknown token type");
        }
        if(error_count > 0) 
            break;
        int ntok = ast.node(ftok);
        if(!next_leading.empty()) detached.push_back(next_leading);
        if(!prev_trailing.empty()) detached.insert(detached.begin(), prev_trailing);
        add_comments(ntok, detached);
        flow_parser(fpp, ftok.type, ntok, &ast);
        auto top = ast.store.back().type;
        if(top == FTK_ACCEPT) 
            break;
        if(top == FTK_SYNTAX_ERROR) {
            AddError(ftok, "syntax error");
            ++error_count;
            break;
        }
    }
    if(error_count == 0 && ftok.type != 0) 
        flow_parser(fpp, 0, -1, &ast);

    flow_parserFree(fpp, free);
    //DEBUG_LEAVE;
    return error_count;
}
