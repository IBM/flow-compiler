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
int compiler::compile(std::string filename, bool debug_on, bool trace_on, std::string default_input_symbol) {
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

    // define the input symbol id not already there
    if(get("flow").size() != 0 && get("flow/INPUT").size() == 0) 
        nappend(get("flow")[0], node(FTK_INPUT, node(ast::token(FTK_ID, 0, 0, 0, default_input_symbol))));

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
    if(get("flow/ENTRY").size() == 0) 
        error(filename, stru::sfmt() << "no entry definition found");

    // The follwing step will generate spurious errors if the compilation 
    // was not without error so far.
    if(error_count == 0)
        propagate_value_types(debug_on);
    
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
                    last_match = at(m).children[1];
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
                    last_match = at(ambiguous_matches[0]).children[1];
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
                    last_match = at(ambiguous_matches[0]).children[1];
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
                error(at(p), stru::sfmt() << "ambiguous reference to enum identifier");
                note(at(p), stru::sfmt() << "matches " << stru::join(enums, ", ", " and ", "", "\"", "\""));
            }
        }
        if(last_match == 0 && ambiguous_matches.size() == 0 && enums.size() == 0) {
            error(at(p), stru::sfmt() << "reference to unknown identifier \"" << stru::join(ids, ".") << "\"");
        }
        if(ambiguous_matches.size() > 1) {
            error(at(p), stru::sfmt() << "ambiguous reference to " << stru::join(ids, "."));
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
    std::string default_input_type;
    int first_default_input_entry = 0;
    for(auto p: get("//(NODE/block/OUTPUT/valx/msgexp/did|ENTRY/did)")) {
        auto ids = get_ids(p);
        std::set<std::string> methods;
        std::string mm; 
        if(gstore.lookup(mm, ids, &methods, true, false, false) != 0) {
            if(methods.size() > 0) {
                error(at(p), stru::sfmt() << "ambiguous reference to rpc \"" << stru::join(ids, ".") << "\"");
                note(at(p), stru::sfmt() << "matches " << stru::join(methods, ", ", " and ", "", "\"", "\""));
            } else {
                error(at(p), stru::sfmt() << "rpc not found \"" << stru::join(ids, ".") << "\"");
            }
            continue;
        }
        if(at(parent(p)).type == FTK_ENTRY) {
            rpc.set(parent(p), mm);
            vtype.set(parent(p), gstore.message_to_value_type(gstore.method_output_full_name(mm)));
            for(int i: get("INPUT", parent(p))) 
                vtype.set(i, gstore.message_to_value_type(gstore.method_input_full_name(mm)));

            if(get("INPUT", parent(p)).size() == 0) {
                if(first_default_input_entry == 0) {
                    first_default_input_entry = parent(p);
                    default_input_type = gstore.method_input_full_name(mm);
                } else if(default_input_type != gstore.method_input_full_name(mm)) {
                    error(at(p), stru::sfmt() << "input type mistmatch, \"" << gstore.method_input_full_name(mm) << "\" instead of \"" << default_input_type << "\"");
                    notep(at(first_default_input_entry),  stru::sfmt() << "first implied from here");
                }
            }
        } else {
            cmsg.set(parent(p), gstore.method_input_full_name(mm));
            vtype.set(ancestor(p, 2), gstore.message_to_value_type(gstore.method_input_full_name(mm)));
            rpc.set(ancestor(p, 5), mm);
            vtype.set(ancestor(p, 5), gstore.message_to_value_type(gstore.method_output_full_name(mm)));
        }
    }
    if(first_default_input_entry != 0 && !default_input_type.empty()) 
        for(int i: get("//flow/INPUT")) 
            if(!vtype.has(i))
                vtype.set(i, gstore.message_to_value_type(default_input_type));
    
    // Other message expressions with identifiers refer to the gRPC message type explicitly. 
    // They can be solved now, before the expression types are resolved.
    for(auto p: get("//valx/msgexp/did")) if(!cmsg.has(parent(p))) {
        auto ids = get_ids(p);
        std::set<std::string> messages;
        std::string mm; 
        if(gstore.lookup(mm, ids, &messages, false, true, false) != 0) {
            if(messages.size() > 0) {
                error(at(p), stru::sfmt() << "ambiguous reference to message type \"" << stru::join(ids, ".") << "\"");
                note(at(p), stru::sfmt() << "matches " << stru::join(messages, ", ", " and ", "", "\"", "\""));
            } else {
                error(at(p), stru::sfmt() << "message not found \"" << stru::join(ids, ".") << "\"");
            }
        }
        cmsg.set(parent(p), mm);
        vtype.set(ancestor(p, 2), gstore.message_to_value_type(mm));
    }
    // If the return msgexp does not have an explicit type, use the entry output type
    for(auto p: get("//ENTRY/block/RETURN/valx/msgexp")) if(!cmsg.has(p)) {
        std::string mn = rpc.get(ancestor(p, 4));
        if(!mn.empty()) {
            auto inpm = gstore.method_output_full_name(mn);
            cmsg.set(p, inpm);
            vtype.set(parent(p), gstore.message_to_value_type(inpm));
        }
    }
    // Grab the node families
    std::map<std::string, std::vector<int>> nfams, idefs; 
    for(int p: get("//NODE/1")) { 
        // first node child is always ID in a healthy ast
        if(at(p).type != FTK_ID)
            continue;
        // don't bother with error nodes
        //if(get("//ERRCHK", parent(p)).size() != 0)
        //    continue;
        nfams[at(p).token.text].push_back(parent(p));
    }
    // Grab the inputs and check for name collision with nodes
    for(int i: get("//INPUT/ID")) {
        std::string iname = at(i).token.text;
        if(idefs.find(iname) == idefs.end()) {
            auto cf = nfams.find(iname);
            if(cf != nfams.end()) {
                error(at(cf->second[0]), stru::sfmt() << "node uses a name already used for input");
                if(at(i).token.line != 0)
                    notep(at(i), stru::sfmt() << "input defined here");
                continue;
            }
        }
        idefs[iname].push_back(parent(i));
    }
    // Check all node references 
    for(int n: get("//ndid/1")) {
        if(nfams.find(at(n).token.text) == nfams.end() &&
           idefs.find(at(n).token.text) == idefs.end())
            error(at(n), stru::sfmt() << "reference to undefined node or input \"" << at(n).token.text << "\"");
    }
    return error_count-irc;
}
static 
value_type op1_type(int op, value_type l) {
    value_type vt;
    switch(op) {
        case FTK_MINUS:
            if(l.type == fvt_int) 
                vt = value_type(fvt_int);
            else if(l.is_basic()) 
                vt = value_type(fvt_flt);
            break;
        default:
            break;
    }
    return vt;
}
static 
value_type op2_type(int op, value_type l, value_type r) {
    value_type vt;
    switch(op) {
        case FTK_PLUS:
            if(l.type == r.type && l.is_basic()) 
                vt = l;
            else if(l.is_basic() && r.is_basic())
                vt = l.is_str()? value_type(fvt_str): value_type(fvt_flt);
            else if(l.a_type().type == r.type)
                vt = l;
            else if(l.type == r.a_type().type)
                vt = r;
            else if(l.is_array() && l.a_type().type == r.a_type().type)
                vt = l;
            break;
        default:
            break;
    }
    return vt;
}
/* Solve expression types by computing them from subexpression types.
 */
int compiler::compute_value_type(bool debug_on, int node) {
    int irc = error_count;
    auto const &n = at(node); 
    // did, ndid, msgexp, vala, range, fun, hash, bang, minus 
    if(n.type == FTK_valx) switch(atc(node, 0).type) {  
        case FTK_ndid: {
            int mn = main_node_by_type(atp(node, 0, 0).token.text);
            if(mn != 0) {
                auto did = stru::join(get_ids(n.children[0]), ".");
                if(gstore.field_full_name(vtype.get(mn).struct_name(), did).empty()) {
                    error(n, stru::sfmt() << "message of type \"" << vtype.get(mn).struct_name() << "\" does not have a field \"" << did << "\"");
                    notep(at(mn), stru::sfmt() << "message type deduced from here");
                }
                vtype.set(node, gstore.field_to_value_type(vtype.get(mn).struct_name(), did));
            }
        }
        break;
        case FTK_did: 
            if(ref.has(n.children[0]) && vtype.has(ref.get(n.children[0]))) {
                // WARNING not covered
                vtype.copy(ref.get(n.children[0]), n.children[0]); 
            }
        break;
        case FTK_msgexp: 
            if(vtype.has(atc(node, 0).children[0]))
                vtype.copy(atc(node, 0).children[0], node);
        break;
        case FTK_MINUS: 
            if(vtype.has(n.children[1])) {
                value_type t = op1_type(FTK_MINUS, vtype.get(n.children[1]));
                vtype.set(node, t);
                if(t.type == fvt_none)
                    error(n, stru::sfmt() << "incompatible type for operator \"-\"");
            }
        break;
        case FTK_HASH: 
        case FTK_BANG:
        break;
        case FTK_PLUS: 
            if(vtype.has(n.children[1]) && vtype.has(n.children[2])) {
                value_type t = op2_type(FTK_PLUS, vtype.get(n.children[1]), vtype.get(n.children[2]));
                vtype.set(node, t);
                if(t.type == fvt_none)
                    error(n, stru::sfmt() << "incompatible type for operator \"+\"");
            }
        break;
        case FTK_PERCENT: // allow string % vala
        break;
        case FTK_STAR:    // allow only numeric
        case FTK_SLASH:
        case FTK_POW:
        break;

        default:
            std::cerr << "DEAL this 1: \n"; 
            print_ast(node);
    } else if(n.type == FTK_list) {
        unsigned solved = 0;
        value_type t(fvt_struct);
        for(int fa: n.children) if(vtype.has(at(fa).children[1])) {
            value_type ft = vtype.get(at(fa).children[1]);
            ft.fname = atc(fa, 0).token.text;
            t.add_type(ft);
            ++solved;
        }
        if(solved == n.children.size())
            vtype.set(node, t);
    }
    return error_count - irc;
}
int compiler::propagate_value_types(bool debug_on) {
    int irc = error_count, step = 0;
    unsigned unfixed_nodes;
    std::vector<int> todo;
    do {
        ++step;
        unfixed_nodes = todo.size();
        todo.clear();
        for(int p: get("//(valx|msgexp/list)"))
            if(!vtype.has(p)) {
                todo.push_back(p);
                compute_value_type(debug_on, p);
            }
        std::cerr << "FIXVT " << step << " " << todo << "\n";
    } while(todo.size() != unfixed_nodes);
    return error_count - irc;
}
int compiler::fixup_nodes(bool debug_on) {
    int irc = error_count;
    // Propagate the return type to the node/entry attribute
    for(auto p: get("//(NODE|ENTRY)/block/RETURN/valx/msgexp")) {
        int nn = ancestor(p, 4);
        if(vtype.has(nn))
            continue;
        if(vtype.has(parent(p))) {
            vtype.copy(parent(p), nn);
            continue;
        }
        if(cmsg.has(p)) {
            vtype.set(nn, gstore.message_to_value_type(cmsg(p)));
            vtype.copy(nn, parent(p));
        }
    }

    // At this point some nodes could have return statements without type.
    // If there is a type, and only one, already deduced for this node family, 
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
    /*
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
    */
    std::cerr << "NODES: " << nfams << "\n";
    return error_count-irc;
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
