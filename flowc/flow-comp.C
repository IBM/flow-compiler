#include "ast-scanner.H"
#include "filu.H"
#include "flow-comp.H"
#include "grpcu.H"
#include "strsli.H"
#include "stru.H"
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

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

    // node sanity check
    for(int n: get("//NODE")) {
        auto dns = get("//(ERRCHK|RETURN|OUTPUT)", n);
        if(dns.size() == 0) {
            error(at(n), stru::sfmt() << "node contains no action instruction, either \"output\", \"return\", or \"error\" is required");
            continue;
        }
        if(dns.size() > 1) {
            error(at(n), stru::sfmt() << "node contains more than one action instruction, only one of \"output\", \"return\", or \"error\" is needed");
            notep(at(dns[0]), stru::sfmt() << "action first defined here");
            for(int i = 1; i < dns.size(); ++i)
                notep(at(dns[i]), stru::sfmt() << "action re-defined here");
        }
    }

    // entry sanity check
    for(int n: get("//ENTRY")) {
        auto dns = get("//RETURN", n);
        if(dns.size() == 0) {
            error(at(n), stru::sfmt() << "\"return\" statement is required here");
            continue;
        }
        if(dns.size() > 1) {
            error(at(n), stru::sfmt() << "entry contains more than one \"return\" statement");
            notep(at(dns[0]), stru::sfmt() << "\"return\" first defined here");
            for(int i = 1; i < dns.size(); ++i)
                notep(at(dns[i]), stru::sfmt() << "\"return\" re-defined here");
        }
    }
    
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

    fixup_symbol_references(debug_on);
    propagate_value_types(debug_on);

    // TODO check if we have entries
    if(get("flow/ENTRY").size() == 0) 
        error(filename, stru::sfmt() << "no entry definition found");
    
    return error_count;
}
/**
 * Fix up symbol references. 
 * Initializes all explicitly typed message expressions. 
 */
int compiler::fixup_symbol_references(bool debug_on) {
    int irc = error_count;
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
    // All output statements in nodes are checked by the parser to have a message expression with 
    // identifier (msgexp/did) that refers to a gRPC method.
    // Entries must explicitly refer to a gRPC method (ENTRY/did)
    for(auto p: get("//(NODE/block/OUTPUT/valx/msgexp/did|ENTRY/did)")) {
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
        if(at(parent(p)).type == FTK_ENTRY) {
            rpc.set(parent(p), mm);
            vtype.set(parent(p), gstore.message_to_value_type(gstore.method_output_full_name(mm)));
        } else {
            cmsg.set(parent(p), gstore.method_input_full_name(mm));
            vtype.set(ancestor(p, 2), gstore.message_to_value_type(gstore.method_input_full_name(mm)));
            rpc.set(ancestor(p, 5), mm);
            vtype.set(ancestor(p, 5), gstore.message_to_value_type(gstore.method_output_full_name(mm)));
        }
    }
    // Other message expressions with identifiers refer to the gRPC message type explicitly. 
    // They can be solved now, before the expression types are resolved.
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
    return error_count-irc;
}
/* Solve expression types by computing them from subexpresssion types.
 */
int compiler::propagate_value_types(bool debug_on) {
    int irc = error_count;
    return error_count - irc;
}

int compiler::fixup_nodes(bool debug_on) {
    // Propagate the return type to the node attribute
    for(auto p: get("//NODE/block/RETURN/valx/msgexp")) {
        int nn = ancestor(p, 4);
        if(vtype.has(nn) || !cmsg.has(p))
            continue;
        vtype.set(nn, gstore.message_to_value_type(cmsg(p)));
    }

    // At this point some nodes could have return statements without type.
    // If there is a type, and only one, already deducted for this node family, 
    // propagate that type as a requirement. Othewise generate errors... 
    std::map<std::string, std::vector<int>> nfams; 
    for(int p: get("//NODE/1")) { 
        // first node child is always ID in a healthy ast
        if(at(p).type != FTK_ID)
            continue;
        // don't bother with error nodes
        if(get("//ERRCHK", parent(p)).size() != 0)
            continue;
        nfams[at(p).token.text].push_back(parent(p));
    }
    for(auto nfe: nfams) {
        if(nfe.second.size() == 1) {
            // all is ok if we have a type
            if(!vtype.has(nfe.second[0])) 
                error(at(nfe.second[0]), stru::sfmt() << "cannot deduce type for node \"" << nfe.first << "\"");
            continue;
        }
        // pick the first node with a type as reference node
        int rn = 0;
        for(int nn: nfe.second)
            if(vtype.has(nn)) {
                rn = nn;
                break;
            }
        for(int nn: nfe.second) if(nn != rn) {
            auto cns = get("//RETURN", nn);
            if(cns.size() != 1) // this node has genrerated an error already 
                continue;

            if(vtype.has(nn)) { // check that the types are compatible
                std::cerr << "CHECK " << vtype.get(nn).to_string() << " AND " << vtype.get(rn).to_string() << "\n";
                continue;
            }
            // assume success but add typechek mandate
            vtype.copy(rn, nn);
            auto mns = get("valx/msgexp", cns[0]);
            std::cerr << "cns: " << cns << " mns: " << mns << "\n";
            print_ast(nn);
            assert(mns.size() == 1);
            assert(!cmsg.has(mns[0]));
            cmsg.set(mns[0], vtype.get(rn).gname);
        }

    }
    std::cerr << "NODES: " << nfams << "\n";
   
    // Now that node output types are established, lookup ndid references.

    return error_count;
}
void compiler::reset() {
    input_filename.clear();
    filenames.clear();
    warning_count = error_count = 0;
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
    std::cerr << "internal error unknown token " << ftk << "\n";
    assert(false);
    return 0;
}
}
