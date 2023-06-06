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

    while(get("//flow/INCLUDE", ptr).size() != include_nodes.size()) {
        for(int incln: get("//flow/INCLUDE")) if(include_nodes.find(incln) == include_nodes.end()) {
            include_nodes.insert(incln);
            value_type vt; std::string value;
            if(eval(at(incln).children[0], value, &vt) != 0) {
                // TODO eval error 
                continue;
            }
            if(included.find(value) != included.end()) {
                error(at(incln), stru::sfmt() << "file already included \"" << value << "\"");
                notep(at(included.find(value)->second), stru::sfmt() << "first included here");
                continue;
            }
            included[value] = incln;
            int itr = parse_file(pp, value, trace_on);
            if(itr > 0 && get("flow", itr).size() > 0)
                graft(parent(incln, ptr), get("flow", itr)[0], sibling_index(incln, ptr)+1);
        }
    }
    
    flow_parserFree(pp, free);
    root_n = ptr;
    
    // import proto files
    for(int i: get("//flow/IMPORT")) {
        value_type vt; std::string value;
        //print_ast(std::cerr, i);
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
    std::cerr << "nodid: " << get("//NODE/ID") << "\n";
    std::cerr << "nodidid: " << get("//NODE/ID/ID") << "\n";
    std::cerr << "nodidid: " << get("//NODE/(ID/ID|ID)") << "\n";
    std::cerr << "msgexp: " << get("//valx/msgexp/did") << "\n";
    std::cerr << "outmsgexp: " << get("//NODE/block/OUTPUT/valx/msgexp/did") << "\n";

    // resolve references
    for(int p: get("//valx/did")) {
        auto ids = get_ids(p);
        std::vector<int> ambiguous_matches;
        int last_match = 0; 
        bool warn_hidden_enums = false;
        std::vector<std::pair<char, int>> match_catnod; // matches: category and node

        switch(ids.size()) {
            case 1:
                // Lookup local, global, enums and grpc errors, in this order.
                // Get the last match.
                for(int m: *this) if(m == p) break; else if(at(m).type == FTK_EQUALS && atc(m, 0).token.text == ids[0]) {
                    match_catnod.push_back(std::make_pair('p', m));
                    last_match = m;
                    //std::cerr << "CHECKING 1 " << m << " " << atc(m, 0).token.text << "\n";
                    //print_ast(std::cerr, m);
                }
            break;
            case 2:
                // Lookup node+local, node-id+local, and enums.
                // Warn if full name enum is hidden by node locals.
                // Error if ambiguous.
                for(int nid: get("//NODE/ID")) if(at(nid).token.text == ids[0]) {
                    int node_last_match = 0;
                    for(int m: get("//EQUALS", parent(nid))) if(atc(m, 0).token.text == ids[1]) {
                        node_last_match = m;
                        //std::cerr << "CHECKING 2 " << m << " " << atc(m, 0).token.text << "\n";
                        //print_ast(std::cerr, m);
                    }
                    if(node_last_match != 0)
                        ambiguous_matches.push_back(node_last_match);
                }
                if(ambiguous_matches.size() == 1) {
                    last_match = ambiguous_matches[0];
                    warn_hidden_enums = true;
                }
            break;
            case 3:
                // Lookup node+node-id+local and enums. 
                // Warn if full name enum is hdden by node locals.
                // Error if ambiguous.
                for(int nid: get("//NODE")) {
                    std::string nids = stru::join(get_ids(nid), ".");
                    int node_last_match = 0;
                    for(int m: get("//EQUALS", nid)) if(nids+"."+atc(m, 0).token.text == stru::join(ids, ".")) {
                        node_last_match = m;
                        //std::cerr << "CHECKING 3 " << m << " " << atc(m, 0).token.text << "\n";
                        //print_ast(std::cerr, m);
                    }
                    if(node_last_match != 0)
                        ambiguous_matches.push_back(node_last_match);
                }
                if(ambiguous_matches.size() == 1) {
                    last_match = ambiguous_matches[0];
                    warn_hidden_enums = true;
                }
            break;
            default:
                // Lookup enums.
            break;
        }

        std::set<std::string> enums;
        if(last_match != 0) {
            ref.set(p, last_match);
            if(vtype.has(last_match)) 
                vtype.copy(last_match, parent(p));
        }
        if(ids.size() == 1 && last_match == 0 && grpcu::store::error_code(ids[0]) >= 0) {
            // FIXME the grpc error code could be hidden by a local variable or another enum. 
            // would it make sense to issue a warning in such a case? maybe in the error context only?
            vtype.set(parent(p), value_type(fvt_int));
            last_match = 1; 
        }
        if(last_match == 0) { 
            std::string mm; // main match TODO check for interference with other matches
            if(gstore.lookup(mm, ids, &enums, false, false, true) == 0) {
                vtype.set(parent(p), value_type(fvt_enum, gstore.enum_full_name_for_value(mm)));
            } else if(enums.size() > 1) {
                error(at(p), stru::sfmt() << "ambigous reference to enum identifier");
                note(at(p), stru::sfmt() << "matches " << stru::join(enums, ", ", " and ", "", "\"", "\""));
            }
        }
        if(last_match == 0 && ambiguous_matches.size() == 0 && enums.size() == 0) {
            error(at(p), stru::sfmt() << "reference to unknown identifier \"" << stru::join(ids, ".") << "\"");
        }
        if(ambiguous_matches.size() > 1) {
            error(at(p), stru::sfmt() << "ambigous reference to " << stru::join(ids, "."));
            int c = 0;
            for(int am: ambiguous_matches) 
                if(++c == 1) 
                    notep(at(am), stru::sfmt() << "defined here");
                else
                    notep(at(am), stru::sfmt() << "also defined here");
        }
    }
    for(auto p: get("//NODE/block/OUTPUT/valx/msgexp/did")) {
        auto ids = get_ids(p);
        std::set<std::string> methods;
        std::string mm; 
        if(gstore.lookup(mm, ids, &methods, true, false, false) != 0) {
            if(methods.size() > 0) {
                error(at(p), stru::sfmt() << "ambigous reference to rpc \"" << stru::join(ids, ".") << "\"");
                note(at(p), stru::sfmt() << "matches " << stru::join(methods, ", ", " and ", "", "\"", "\""));
            } else {
                error(at(p), stru::sfmt() << "rpc not found \"" << stru::join(ids, ".") << "\"");
            }
            continue;
        }
        cmsg.set(parent(p), gstore.method_input_full_name(mm));
        vtype.set(ancestor(p, 2), gstore.message_to_value_type(gstore.method_input_full_name(mm)));
        rpc.set(ancestor(p, 3), mm);
        vtype.set(ancestor(p, 5), gstore.message_to_value_type(gstore.method_output_full_name(mm)));
    }
    for(auto p: get("//valx/msgexp/did")) if(!cmsg.has(parent(p))) {
        auto ids = get_ids(p);
        std::set<std::string> messages;
        std::string mm; 
        if(gstore.lookup(mm, ids, &messages, false, true, false) != 0) {
            if(messages.size() > 0) {
                error(at(p), stru::sfmt() << "ambigous reference to message type \"" << stru::join(ids, ".") << "\"");
                note(at(p), stru::sfmt() << "matches " << stru::join(messages, ", ", " and ", "", "\"", "\""));
            } else {
                error(at(p), stru::sfmt() << "message not found \"" << stru::join(ids, ".") << "\"");
            }
        }
        cmsg.set(parent(p), mm);
        vtype.set(ancestor(p, 2), gstore.message_to_value_type(mm));
    }
    // propagate return type to the node
    for(auto p: get("//NODE/block/RETURN/valx/msgexp")) {
        int nn = ancestor(p, 4);
        if(vtype.has(nn) || !cmsg.has(p))
            continue;
        vtype.set(nn, gstore.message_to_value_type(cmsg(p)));
    }
    // At this point some nodes could have return statements without type.
    // If there is a type, and only one, already deducted for this node family, 
    // propagate that type as a requirement. Othewise generate errors... 

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
