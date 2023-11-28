#include "ast-scanner.H"
#include "filu.H"
#include "flow-comp.H"
#include "grpcu.H"
#include "strsli.H"
#include "stru.H"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#define YYCOVERAGE
#include "flow-parser.c"
namespace fc {
    struct entry_data;
}

std::ostream &operator<< (std::ostream &out, struct fc::entry_data const &ed);

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

    // augment the tree with the input symbol if it was not explicitly defined
    if(get("flow").size() != 0 && get("flow/INPUT").size() == 0) 
        nappend(get("flow")[0], node(FTK_INPUT, node(ast::token(FTK_ID, 0, 0, 0, default_input_symbol))));

    // node sanity check
    std::map<std::string, std::map<std::string, int>> node_table;
    std::map<std::string, int> node_counts;
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
        // check that the node ids are unique
        auto nids = get("ID", n);
        auto node_type = node_text(nids[0]);
        if(node_counts.find(node_type) == node_counts.end()) {
            node_counts[node_type] = 1;
        } else {
            node_counts[node_type] += 1;
        }
        auto &typeids = node_table[node_type];
        if(nids.size() == 2) {
            auto node_id = node_text(nids[1]);
            if(typeids.find(node_id) == typeids.end()) {
                typeids[node_id] = nids[0];
            } else {
                error(at(nids[1]), stru::sfmt() << "re-use of node ID \"" << node_id << "\"");
                notep(at(typeids.find(node_id)->second), stru::sfmt() << "first defined here");
            }
        } 
    }
    // Make a second pass to generate node IDs.
    // Singleton nodes: family[_name]
    // Tagged nodes: family_name 
    // Untagged nodes family_index
    // TODO Untagged nodes only one in the family: family
    std::set<std::string> idset;
    for(int n: get("//NODE")) {
        auto nids = get("ID", n);
        auto family = node_text(nids[0]);
        // tagged node
        if(nids.size() > 1) {
            std::string id = stru::sfmt() << family << "_" << node_text(nids[1]);
            iid.set(n, id);
            idset.insert(id);
            continue;
        }
        // singleton node
        if(node_counts[family] == 1) {
            std::string id = family;
            iid.set(n, id);
            idset.insert(id);
            continue;
        }
        // generate tag for untagged nodes
        for(int i = 1; i <= node_counts[family] * 2; ++i) {
            std::string id = stru::sfmt() << family << "_" << i;
            if(idset.find(id) == idset.end()) {
                idset.insert(id);
                iid.set(n, id);
                break;
            }
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
        if(gstore.import_file(value, false) != 0) 
            error(at(i), stru::sfmt() << "failed to import \"" << value << "\"");
    }

    resolve_references(debug_on) ||
    resolve_node_types(debug_on);

    if(get("flow/ENTRY").size() == 0) 
        error(filename, stru::sfmt() << "no entry definition found");

    // The follwing step will generate spurious errors if the compilation 
    // was not without error so far.
    error_count != 0 ||
    propagate_value_types(debug_on) ||

    // If all went well, check that node families have the same return type

    check_node_types(debug_on) ||
    check_node_references(debug_on);
   
    if(error_count == 0) 
        for(int en: get("//ENTRY"))  {
            auto ins = get("//INPUT", en);
            if(ins.size() == 0) ins = get("//flow/INPUT");
            generate(std::cout, en, ins[0], debug_on);
        }

    return error_count;
}
int compiler::get_node_references(std::map<std::string, std::set<int>> &refs, int node_node) const {
    auto s = refs.size();
    for(int nn: all_of_type(FTK_ndid, node_node)) {
        std::string nfamname = node_text(child(nn, 0));
        refs[nfamname].insert(nn);
    }
    return refs.size()-s;
}
int compiler::get_family_references(std::set<std::string> &refs, int n) const {
    auto s = refs.size();
    std::map<std::string, std::set<int>> nrefs;
    get_node_references(nrefs, n);
    for(auto ns: nrefs)
        refs.insert(ns.first);
    return refs.size()-s;
}
int compiler::get_node_references(std::map<std::string, std::set<int>> &refs, std::string node_family) const {
    auto s = refs.size();

    for(int n: get("//NODE"))
        if(node_family == node_text(child(n, 0)))
            get_node_references(refs, n);
    return refs.size()-s;
}
int compiler::get_deep_node_references(std::map<std::string, std::set<int>> &refs, std::string node_family) const {
    auto s = refs.size();

    std::map<std::string, std::set<int>> lrefs;
    std::set<std::string> new_families;    
    new_families.insert(node_family);

    while(new_families.size() > 0) {
        lrefs.clear();
        for(auto nf: new_families)
            get_node_references(lrefs, nf);
        new_families.clear();
        for(auto ls: lrefs) {
            if(refs.find(ls.first) == refs.end())
                new_families.insert(ls.first);
            refs[ls.first].insert(ls.second.begin(), ls.second.end());
        }
    }

    return refs.size()-s;
}
int compiler::get_deep_family_references(std::set<std::string> &refs, int n, std::string input_label) const {
    int s = refs.size();

    std::map<std::string, std::set<int>> entrefs;
    // start with the nodes referenced in the n subtree
    get_node_references(entrefs, n);
    for(auto nf: entrefs) if(nf.first != input_label) {
        std::map<std::string, std::set<int>> noderefs;
        get_deep_node_references(noderefs, nf.first);
        for(auto ls: noderefs)
            refs.insert(ls.first);
        refs.insert(nf.first);
    }
    return refs.size() - s;
}
/**
 * Check nodes for circular references, and for inaccessibility.
 * Warn for unused nodes.
 */
int compiler::check_node_references(bool debug_on) {
    int irc = error_count;
    std::map<std::string, std::set<int>> allrefs;
    for(int en: get("//ENTRY")) {
        auto ins = get("//INPUT", en);
        if(ins.size() == 0) ins = get("//flow/INPUT");
        int input_node = ins[0];
        std::string input_label = node_text(get("ID", input_node)[0]);

        std::map<std::string, std::set<int>> entrefs;
        // start with the nodes referenced in the entry
        get_node_references(entrefs, en);
        for(auto nf: entrefs) if(nf.first != input_label) {
            std::map<std::string, std::set<int>> noderefs;
            get_deep_node_references(noderefs, nf.first);
            for(auto ls: noderefs)
                allrefs[ls.first].insert(ls.second.begin(), ls.second.end());
            allrefs[nf.first].insert(nf.second.begin(), nf.second.end());
        }
    }
    // check each node for circular references
    for(auto nf: allrefs) {
        std::map<std::string, std::set<int>> noderefs;
        get_deep_node_references(noderefs, nf.first);
        if(noderefs.find(nf.first) != noderefs.end()) 
            error(at(*noderefs.find(nf.first)->second.begin()), stru::sfmt() << "circular reference of node \"" << nf.first << "\"\n");
    }
    // look for unreferenced nodes
    for(int n: get("//NODE")) {
        std::string node_family = node_text(child(n, 0));
        if(allrefs.find(node_family) == allrefs.end())
            warning(at(child(n, 0)), stru::sfmt() << "node \"" << node_family << "\" is not referenced by any entry\n");
    }
    return error_count - irc;
}
/**
 * Check that all nodes return the same (or compatible) message type
 */
int compiler::check_node_types(bool debug_on) {
    int irc = error_count;
    std::map<std::string, std::vector<int>> node_families;
    for(int n: get("//NODE"))
        node_families[node_text(child(n, 0))].push_back(n);
    for(auto const &ft: node_families) {
        value_type nft; int fn = 0;
        for(int n: ft.second) {
            value_type const &nt = vtype(n);
            if(nt.type == fvt_none)
                continue;
            if(nft.type == fvt_none) {
                nft = nt; fn = n;
                continue;
            }
            if(nft != nt) {
                error(at(n), stru::sfmt() << "node returns message of incompatible type");
                notep(at(fn), stru::sfmt() << "first declared here");
            }
        }
    }
    return error_count - irc;
}
/**
 * Resolve right values of the form id[.id]*
 * to either node local variables for this or other nodes, 
 * or enum values.
 */
int compiler::resolve_references(bool debug_on) {
    int irc = error_count;
    // resolve references
    for(int p: get("//valx/did")) {
        auto ids = get_ids(p);
        std::vector<int> ambiguous_matches;
        int last_match = 0; 
        bool warn_hidden_enums = false;
        std::vector<std::pair<char, int>> match_catnod; // matches: category and node
        int valx_n = parent(p);

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
                    // Make sure the current node isn't included in matches
                    if(node_last_match != 0 && at(node_last_match).children[1] != valx_n)
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
                    if(node_last_match != 0 && at(node_last_match).children[1] != valx_n)
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
            // Set the current valex ref attribute to the node with the definition
            ref.set(valx_n, last_match);
            if(vtype.has(last_match)) 
                vtype.copy(last_match, parent(p));
        }
        if(ids.size() == 1 && last_match == 0 && grpcu::store::error_code(ids[0]) >= 0) {
            // FIXME the grpc error code could be hidden by a local variable or another enum. 
            // would it make sense to issue a warning in such a case? maybe in the error context only?
            vtype.set(valx_n, value_type(fvt_int));
            last_match = 1; 
        }
        if(last_match == 0) { 
            std::string mm; // main match TODO check for interference with other matches
            if(gstore.lookup(mm, ids, &enums, false, false, true) == 1) {
                vtype.set(valx_n, value_type(fvt_enum, gstore.enum_full_name_for_value(mm)));
            } else if(enums.size() > 1) {
                error(at(p), stru::sfmt() << "ambiguous reference to enum identifier");
                note(at(p), stru::sfmt() << "matches " << stru::join(enums, ", ", " and ", "", "\"", "\""));
            }
        }
        if(last_match == 0 && ambiguous_matches.size() == 0 && enums.size() == 0) {
            error(at(p), stru::sfmt() << "reference to unknown identifier \"" << stru::join(ids, ".") << "\"");
        }
        if(ambiguous_matches.size() > 1) {
            error(at(p), stru::sfmt() << "ambiguous reference to \"" << stru::join(ids, ".") << "\"");
            int c = 0;
            for(int am: ambiguous_matches) 
                if(++c == 1) 
                    notep(at(am), stru::sfmt() << "defined here");
                else
                    notep(at(am), stru::sfmt() << "also defined here");
        }
    }
    // Look for circular references
    int count = 0;
    for(int s: get("//valx")) {
        ++count;
        std::set<int> visited;
        std::vector<int> queue;
        queue.push_back(s);
        for(int f: get("//valx", s))
            queue.push_back(f);
        int r = 0;

        while(r == 0 && queue.size() > 0) {
            int x = queue.back(); 
            queue.pop_back();
            int c = ref(x);
            visited.insert(x);
            if(c == s) {
                r = x;
            } else if(c != 0) {
                for(int f: get("//valx", c))
                    if(visited.find(f) == visited.end())
                        queue.push_back(f);
            }
        }
        if(r != 0) {
            error(at(s), stru::sfmt() << "expression references itself");
            if(r != s)
                notep(at(r), stru::sfmt() << "referenced here");
        }
    }

    return error_count-irc;
}
/**
 * Initializes all explicitly typed message expressions.
 */
int compiler::resolve_node_types(bool debug_on) {
    int irc = error_count;
    // All output statements in nodes are checked by the parser to have a message expression with 
    // identifier (msgexp/did) that refers to a gRPC method.
    // Entries must explicitly refer to a gRPC method (ENTRY/did)
    std::string default_input_type;
    int first_default_input_entry = 0;
    for(auto p: get("//(NODE/block/OUTPUT/valx/msgexp/did|ENTRY/did)")) {
        auto ids = get_ids(p);
        std::set<std::string> methods;
        std::string mm; 
        if(gstore.lookup(mm, ids, &methods, true, false, false) != 1) {
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
        if(gstore.lookup(mm, ids, &messages, false, true, false) != 1) {
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
    // Check for undefined node references 
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
            std::cerr << "internal " << __FUNCTION__ << " not implemented for: " << op << "\n";
            assert(false);
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
        case FTK_STAR:   
            if(l.is_str() && r.is_num())
                vt = value_type(fvt_str);
            else if(l.is_int() && r.is_int())
                vt = value_type(fvt_int);
            else if(l.is_num() && r.is_num())
                vt = value_type(fvt_flt);
            break;
        case FTK_PERCENT: 
        case FTK_SLASH:
        case FTK_POW:
            if(l.is_int() && r.is_int())
                vt = value_type(fvt_int);
            else if(l.is_num() && r.is_num())
                vt = value_type(fvt_flt);
            break;
        default:
            std::cerr << "internal " << __FUNCTION__ << " not implemented for: " << op << "\n";
            assert(false);
            break;
    }
    return vt;
}
static
std::map<std::string, value_type> predef_rt = {
    {"rand", value_type(fvt_flt) },
    {"exp", value_type(fvt_flt) },
    {"log", value_type(fvt_flt) },
    {"tolower", value_type(fvt_str) },
    {"toupper", value_type(fvt_str) },
    {"toid", value_type(fvt_str) },
    {"tocname", value_type(fvt_str) },
    {"str", value_type(fvt_str) },
    {"substr", value_type(fvt_str) },
    {"int", value_type(fvt_int) },
    {"flt", value_type(fvt_flt) },
    {"format", value_type(fvt_flt) },
    {"join", value_type(fvt_str) },
    {"split", value_type(fvt_array, {value_type(fvt_str)}) },
};

static 
value_type fun_type(std::string fname, std::vector<value_type> const &avt) {
    value_type vt;
    auto pp = predef_rt.find(fname);
    if(pp != predef_rt.end())
        return pp->second;
    if(fname == "batch" && avt.size() == 1) 
        return value_type(fvt_array, {avt[0]});
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
                if(!did.empty()) {
                    if(gstore.field_full_name(vtype.get(mn).struct_name(), did).empty()) {
                        error(n, stru::sfmt() << "message of type \"" << vtype.get(mn).struct_name() << "\" does not have a field \"" << did << "\"");
                        notep(at(mn), stru::sfmt() << "message type deduced from here");
                    }
                    vtype.set(node, gstore.field_to_value_type(vtype.get(mn).struct_name(), did));
                } else {
                    vtype.copy(mn, node);
                }
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
        case FTK_PERCENT: // allow string % vala
        case FTK_STAR:    // allow only numeric
        case FTK_SLASH:
        case FTK_POW:
            if(vtype.has(n.children[1]) && vtype.has(n.children[2])) {
                value_type t = op2_type(atc(node, 0).type, vtype.get(n.children[1]), vtype.get(n.children[2]));
                vtype.set(node, t);
                if(t.type == fvt_none)
                    error(n, stru::sfmt() << "incompatible types for operator \"" << atc(node, 0).token.text << "\"");
            }
        break;
        case FTK_QUESTION:
            if(vtype.has(n.children[3]))
                vtype.copy(n.children[3], node);
        break;

        default:
            std::cerr << "internal not implemented propagating value for valx of type " << atc(node, 0).type << "\n"; 
            print_ast(node);
            assert(false);
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
    } else if(n.type == FTK_fun) {
        std::vector<value_type> avt;
        for(unsigned a = 1, e = n.children.size(); a < e; ++a) 
            if(vtype.has(n.children[a])) 
                avt.push_back(vtype.get(n.children[a]));
        if(avt.size()+1 == n.children.size()) {
            value_type vt = fun_type(at(n.children[0]).token.text, avt);
            if(!vt.is_null()) vtype.set(node, vt);
        }
    } else {
        std::cerr << "internal propagating value type for " << n.type << "\n"; 
        assert(false);
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
        for(int p: get("//(valx|msgexp/list|fun)"))
            if(!vtype.has(p)) {
                todo.push_back(p);
                compute_value_type(debug_on, p);
            }
        //std::cerr << "FIXVT " << step << " " << todo << "\n";
        fixup_nodes(debug_on);
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
        /*
        if(cmsg.has(p)) {
            vtype.set(nn, gstore.message_to_value_type(cmsg(p)));
            vtype.copy(nn, parent(p));
        }
        */
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
    //std::cerr << "NODES: " << nfams << "\n";
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
bool compiler::is_const_valx(int node) const {
    return false;
}
std::vector<int> compiler::node_family(std::string family_name) const {
    std::vector<int> nodes;
    int d_node = 0;
    for(int n: get("//NODE")) if(node_text(child(n, 0)) == family_name) {
       if(get("valx", n).size() == 0) {
           d_node = n;
       } else {
           nodes.push_back(n);
       }
    }
    if(d_node != 0)
        nodes.push_back(d_node);
    return nodes;
}
struct entry_data {
    int entry_n, return_valx_n;
    std::string input_label;
    // all node families used by this entry, excluding input
    std::vector<std::pair<std::string, std::vector<int>>> families;
    // declared mesage stores name->type
    std::map<std::string, std::string> node_inputs, node_outputs;
    // maps from the node iid->name in the maps above
    std::map<std::string, std::string> ialiases, oaliases;
    // output type, method name, input type
    std::string output_type, name, input_type;
    // families this entry depends at the firt level, without input
    std::map<std::string, std::vector<std::string>> depends;
};
int compiler::generate(std::ostream &out_stream, int en, int input_node, bool debug_on) {
    int irc = error_count;
    entry_data ed;
    ed.input_label = node_text(get("ID", input_node)[0]);
    ed.entry_n = en;
    // it was already verified that there is one and only one RETURN/valx subsequence
    ed.return_valx_n = get("//RETURN/valx", en)[0];
    auto entry_names = gstore.all_method_full_names(stru::join(get_ids(child(en, 0)), "."));

    ed.name = entry_names[1];
    ed.input_type = entry_names[2];
    ed.output_type = entry_names[0];

    for(auto family: get_family_references(en)) if(family != ed.input_label) 
        ed.depends[""].push_back(family);

    if(ed.depends.size() > 0) {
        std::vector<std::pair<std::string, std::vector<int>>> node_families;
        std::vector<std::pair<int, int>> order;  
        for(auto f: get_deep_family_references(ed.return_valx_n, ed.input_label)) {
            node_families.emplace_back(std::make_pair(f, node_family(f)));
            order.push_back(std::make_pair(
                        node_families.back().second.size() == 0? 0: 
                        *std::min_element(node_families.back().second.begin(), node_families.back().second.end()),
                        node_families.size()-1));
        }
        std::sort(order.begin(), order.end());
        for(auto f: order) if(node_families[f.second].first != ed.input_label) 
            ed.families.push_back(node_families[f.second]);
    }

    stru::indented_stream indenter(out_stream, 0);
    auto &out = indenter;
    for(auto &f: ed.families) { 
        auto &nodes = f.second;
        std::string output_type;
        std::map<std::string, std::string> inputs;
        // declare output and input for all nodes in the family, skip over error nodes
        for(int n: nodes) if(vtype.has(n)) {
            std::string input_symbol = stru::sfmt() << "I_" << iid(n);
            std::string output_symbol = stru::sfmt() << "O_" << iid(n);

            if(output_type.empty()) {
                // vtype will always be a meessage type for a node
                output_type = vtype(n).struct_name();
                ed.node_outputs[stru::sfmt() << "O_" << f.first] = output_type;
            }
            ed.oaliases[iid(n)] = stru::sfmt() << "O_" << f.first;
            if(rpc.has(n)) {
                std::string input_type = gstore.method_input_full_name(rpc(n));
                auto alias_decl = inputs.find(input_type);
                // for rpc nodes check if inputs can be shared
                if(alias_decl == inputs.end()) {
                    inputs[input_type] = iid(n);
                    ed.node_inputs[input_symbol] = input_type;
                    ed.ialiases[iid(n)] = input_symbol;
                } else {
                    //out << input_type << " &I_" << iid(n) << " = I_" << alias_decl->second << ";\n";
                    ed.ialiases[iid(n)] = stru::sfmt() << "I_" << alias_decl->second;
                }
            } else {
                // for return nodes, input and output are the same
                ed.ialiases[iid(n)] = stru::sfmt() << "O_" << f.first;
            }
        }
        std::map<std::string, std::set<int>> refs;
        get_deep_node_references(refs, f.first);
        for(auto d: refs) if(d.first != ed.input_label)
            ed.depends[f.first].push_back(d.first);
    }

    out_stream << ed << "\n";
    return error_count - irc;
}
int compiler::dep_tree(stru::indented_stream &indenter, int vn, std::string input_label, std::map<std::string, int> &visited, int depth) {
    auto &out = indenter;
    // iterate over all ndid children of vn
    for(int n: all_of_type(FTK_ndid, vn)) {
        std::string node_type = node_text(child(n, 0));
        if(node_type == input_label || visited.find(node_type) != visited.end()) 
            continue;
        visited[node_type] = depth;
        out << node_type << ": \n";
        ++indenter;
        for(int n: get("//NODE")) 
            if(node_text(child(n, 0)) == node_type) {
                for(int v: get("valx", n)) 
                    dep_tree(indenter, v, input_label, visited, depth+1);
                for(int v: get("//(ERRCHK|RETURN|OUTPUT)/valx", n)) 
                    dep_tree(indenter, v, input_label, visited, depth+1);
                for(int v: get("//HEADER/COLON/valx", n)) 
                    dep_tree(indenter, v, input_label, visited, depth+1);
            }
        --indenter;
    }
    return 0; 
}
int compiler::generate_valx(stru::indented_stream &indenter, int vn, bool debug_on) {
    int irc = error_count;
    auto &out = indenter;
    switch(atc(vn, 0).type) {
        case FTK_msgexp:
            out << "// msgexp " << child(vn, 0) << " for " << vn << "\n";
            break;
        case FTK_ndid:
            out << "// ndid " << child(vn, 0) << " for " << vn << "\n";
            break;
        default:
            std::cerr << "internal error: generate not implemented for " << tk_to_string(atc(vn, 0).type) << " at node " << vn << "\n";
    }
    return error_count - irc;
}
int compiler::generate_valx_ndid(stru::indented_stream &indenter, int nn, bool debug_on) {
    int irc = error_count;

    return error_count - irc;
}
int compiler::generate_valx_msgexp(stru::indented_stream &indenter, int mn, bool debug_on) {
    int irc = error_count;

    return error_count - irc;
}
}
std::ostream &operator<< (std::ostream &out, fc::entry_data const &ed) {
    out << "entry_node: " << ed.entry_n << "\n"
        << "valx_node: " << ed.return_valx_n << "\n"
        << "input_label: " << ed.input_label << "\n"
        << "families: " << ed.families << "\n"
        << "node_inputs: " << ed.node_inputs << "\n"
        << "node_outputs: " << ed.node_outputs << "\n"
        << "ialiases: " << ed. ialiases << "\n"
        << "oaliases: " << ed.oaliases << "\n"
        << "output_type: " << ed.output_type << "\n"
        << "name " << ed.name << "\n"
        << "intput_type: " << ed.input_type << "\n"
        << "depends: " << ed.depends << "\n";
    return out;
}
