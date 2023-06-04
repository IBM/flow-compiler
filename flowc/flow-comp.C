#include <fstream>
#include <string>

#include "ast-scanner.H"
#include "filu.H"
#include "stru.H"
#include "strsli.H"
#include "flow-comp.H"

#define YYCOVERAGE
#include "flow-parser.c"

namespace fc {

int compiler::parse_file(yyParser *pp, std::string filename, bool trace_on) {
    input_filename = filename;
    filenames.push_back(filename);

    std::ifstream infs(input_filename.c_str());
    if(!infs.is_open()) {
        error(filename, -1, -1, "can't read file");
        return 0;
    }
    // TODO check here if the file is utf-8, i.e. not a binary file by accident
    ast::scanner sc(&infs, filenames.size()-1);
/*
    for(auto t=sc.scan(); t.type != ATK_EOF; t=sc.scan())
       std::cerr << "SCAN: " << ast::token(t.type) << "(" << t.line << ":" << t.column << ")<" << t.text << ">\n";
*/
    auto t = sc.scan();
    while(t.type != ATK_EOF) {
        while(sc.has_error()) {
            auto e = sc.get_error();
            error(e, e.text);
        }
        switch(t.type) {
            case ATK_UNK:
                error(t, stru::sfmt() << "unknown token: \"" << t.text << "\"");
            break;
            default:
            break;
        }
        int ntok = node(t);
        if(trace_on)
            std::cerr << "TOKEN: " << ftk_to_string(t.type) << "(" << t.line << ":" << t.column << ")<" << t.text << ">\n";
        flow_parser(pp, t.type, ntok, this);
        auto top = store.back().type;
        if(top == FTK_ACCEPT) 
            break;
        if(top == FTK_FAILED) 
            break;
        t = sc.scan();
    }
    if(t.type != ATK_NONE && store.back().type != FTK_FAILED) 
        flow_parser(pp, 0, -1, this);

    return (int) store.size();
}
int compiler::compile(std::string filename, bool debug_on, bool trace_on) {
    yyParser *pp = (yyParser *) flow_parserAlloc(malloc);
    if(trace_on)
        flow_parserTrace(stderr, (char *) "parser: ");

    std::set<int> include_nodes;
    std::map<std::string, int> included;
    int ptr = parse_file(pp, filename, trace_on);
    /**
    while(getc("wfgsrc/INCLUDE", ptr) != include_nodes.size()) {
        for(int incln: get("wfgsrc/INCLUDE", [&include_nodes](int n)->bool {return include_nodes.find(n) == include_nodes.end();}, ptr)) {
            std::string fn = eval(at(incln).children[0]);
            if(included.find(fn) == included.end()) {
                int itr = parse_file(pp, fn, trace_on);
                if(itr > 0)
                    graft(parent(incln, ptr), getf("wfgsrc", itr), sibling_index(incln, ptr)+1);
            } else {
                error(at(incln), stru::sfmt() << "file already included \"" << fn << "\"");
                notep(at(included.find(fn)->second), stru::sfmt() << "first included here");
            }
            included[fn] = incln;
            include_nodes.insert(incln);
        }
    }
    */
    flow_parserFree(pp, free);
    root_n = ptr;

    // import proto files
    for(int i: *this) if(at(i).type == FTK_IMPORT) {
        value_type vt; std::string value;
        print_ast(std::cerr, i);
        if(eval(i, value, &vt) != 0) { 
            // TODO eval error here
            continue;
        }
        std::cerr << "IMPORTING " << value << "\n";
        if(gstore.import_file(value, false) != 0) 
            error(at(i), stru::sfmt() << "failed to import \"" << value << "\"");
    }

    return error_count;
}
void compiler::reset() {
    input_filename.clear();
    filenames.clear();
    warning_count = error_count = 0;
    show_line_with_error = show_line_with_warning = true;
    // delete the tree
    store.clear(); root_n = 0;
}
static std::map<int, std::string> non_terminals = {
    {FTK_ACCEPT,            "ACCEPT"},
    {FTK_FAILED,            "FAILED"},
    {FTK_flow,              "flow"},

    {FTK_block,  "block"},
    {FTK_list, "list"},
    {FTK_fassgn, "fassgn"},
    {FTK_msgexp, "msgexp"},
    
    {FTK_fun, "fun"}, 
    {FTK_did,  "did"}, 
    {FTK_ndid, "ndid"}, 
    {FTK_range, "range"},
    {FTK_valx, "valx"},
};
std::string compiler::ftk_to_string(int ftk) {
    assert(ftk >= 0 && ftk < FTK_MAX_NONTERM);
    if(ftk < FTK_ACCEPT) {
        assert(ftk < sizeof(yyTokenName)/sizeof(yyTokenName[0]));
        return yyTokenName[ftk];
    } else {
        auto tp = non_terminals.find(ftk);
        if(tp != non_terminals.end())
            return tp->second;
        return stru::sfmt() << "??" << ftk << "/" << FTK_ACCEPT;
    }
}
}
