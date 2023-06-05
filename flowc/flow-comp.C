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
            std::cerr << "TOKEN: " << tk_to_string(t.type) << "(" << t.line << ":" << t.column << ")<" << t.text << ">\n";
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
    /* FIXME
    while(getc("flow/INCLUDE", ptr) != include_nodes.size()) {
        for(int incln: get("flow/INCLUDE", [&include_nodes](int n)->bool {return include_nodes.find(n) == include_nodes.end();}, ptr)) {
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
    for(int i: get("//flow/IMPORT")) {
        value_type vt; std::string value;
        print_ast(std::cerr, i);
        if(eval(at(i).children[0] , value, &vt) != 0) { 
            // TODO eval error here
            continue;
        }
        std::cerr << "IMPORTING " << i << ": " << value << "\n";
        if(gstore.import_file(value, false) != 0) 
            error(at(i), stru::sfmt() << "failed to import \"" << value << "\"");
    }

    std::vector<int> tnodes;
    for(int v: *this) if(at(v).type == FTK_valx) for(int p: at(v).children) if(at(p).type == FTK_did) {
        tnodes.push_back(p);
    }
    std::cerr << "tnodes: " << tnodes << "\n";
    std::cerr << "getnod: " << get("//valx/did") << "\n";
    std::cerr << "getnod: " << get("//valx//did") << "\n";
    std::vector<std::string> k, l;
    for(auto s: stru::splitter("//valx/did//", "/"))
        k.push_back(s);
    std::cerr << "K: " << k << "\n" << "L: "<< l << "\n";

    // resolve references
    for(int p: get("//valx/did")) {
        auto ids = get_ids(p);
        //std::cerr << "LOOKING UP: " << ids << "\n";
        //print_ast(std::cerr, *p);
        std::string mm; // main match
        std::vector<std::pair<char, int>> match_catnod; // matches: category and node
        std::set<std::string> enums;

        switch(ids.size()) {
            case 1:
                // Lookup local, global, enums and grpc errors, in this order.
                // Get the first match.
                for(int m: *this) if(m == p) break; else if(at(m).type == FTK_EQUALS && atc(m, 0).token.text == ids[0]) {
                    match_catnod.push_back(std::make_pair('p', m));
                    std::cerr << "CHECKING OUT " << m << " " << atc(m, 0).token.text << "\n";
                    print_ast(std::cerr, m);
                }
                if(match_catnod.size() == 0) {
                    if(gstore.lookup(mm, ids, &enums, false, false, true) == 0) {
                        vtype.set(parent(p), value_type(fvt_enum));
                    } else if(enums.size() > 1) {
                        error(at(p), stru::sfmt() << "ambigous reference to enum identifier");
                        note(at(p), stru::sfmt() << "matches " << stru::join(enums, ", ", " and ", "", "\"", "\""));
                    } else if(grpcu::store::error_code(ids[0]) >= 0) {
                        // FIXME the error code could be hidden by a local variable or another enum. 
                        // would it make sense to issue a warning in such a case? maybe in the error context only?
                        vtype.set(parent(p), value_type(fvt_int));
                    } else {
                        error(at(p), stru::sfmt() << "reference to unknown identifier \"" << stru::join(ids, ".") << "\"");
                    }
                } else {
                    int an = match_catnod.back().second;
                    ref.set(p, an);
                    if(vtype.has(an)) 
                        vtype.copy(an, parent(p));
                }
            break;
            case 2:
                // Lookup node+local, node-id+local, and enums.
                // Warn if full name enum is hidden by node locals.
                // Error if ambiguous.
            break;
            case 3:
                // Lookup node+node-id+local and enums. 
                // Warn if full name enum is hdden by node locals.
                // Error if ambiguous.
            break;
            default:
                // Lookup enums.
            break;
        }
        /*
        std::string mn;
        if(gstore.lookup(mn, get_ids(i)) != 0) {
            error(at(i), stru::sfmt() << "reference to unknown identifier \"" << stru::join(get_ids(i), ".") << "\"");
        }
        */
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
    {FTK_ACCEPT, "ACCEPT"},
    {FTK_FAILED, "FAILED"},
    {FTK_flow,   "flow"},

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
std::string compiler::tk_to_string(int ftk) const {
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
int compiler::string_to_tk(std::string ftk) const {
    for(auto nte: non_terminals) 
        if(nte.second == ftk) return nte.first;
    for(int t = 0; t < sizeof(yyTokenName)/sizeof(yyTokenName[0]); ++t)
        if(ftk == yyTokenName[t])
            return t;
    return 0;
}
}
