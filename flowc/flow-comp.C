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
        if(top == FTK_ACCEPT ||  top == FTK_FAILED) 
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
            unid.set(n, id);
            idset.insert(id);
            continue;
        }
        // singleton node
        if(node_counts[family] == 1) {
            std::string id = family;
            unid.set(n, id);
            idset.insert(id);
            continue;
        }
        // generate tag for untagged nodes
        for(int i = 1; i <= node_counts[family] * 2; ++i) {
            std::string id = stru::sfmt() << family << "_" << i;
            if(idset.find(id) == idset.end()) {
                idset.insert(id);
                unid.set(n, id);
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
        if(eval(first_child(i) , value, &vt) != 0) { 
            // TODO eval error here
            continue;
        }
        if(gstore.import_file(value, false) != 0) 
            error(at(i), stru::sfmt() << "failed to import \"" << value << "\"");
    }

    /**** 
     *  1. resolve entries with rpc and input type
     *  2. resolve input types, both global and within entry
     *  3. solve all expressions that depend on
     *          - constants 
     *          - inputs 
     *  4. solve nodes 
     *  5. loop back to 3 if more nodes could be solved
     */
    int rrc;
    // solve all entry rpcs and input and output types 
    rrc = resolve_entries(debug_on);
    std::cerr << "RESOLVED " << rrc << " entries\n";
    // solve all node rpcs, and the explicit return nodes
    rrc = resolve_nodes(debug_on);
    std::cerr << "RESOLVED " << rrc << " nodes\n";
    // solve all indentifiers valx/did 
    rrc = resolve_did_references(debug_on);
    std::cerr << "RESOLVED " << rrc << " dids\n";
    // resolve input field references (valx/ndid)
    rrc = resolve_input_references(debug_on);
    std::cerr << "RESOLVED " << rrc << " input refs\n";
    int solved_nodes = 0, past_solved_nodes = 0;
    do {
        past_solved_nodes = solved_nodes;
        resolve_expressions(debug_on);
        past_solved_nodes = resolve_nodes(debug_on);
    } while(past_solved_nodes != solved_nodes);
    // set the const level
    if(error_count == 0)  
        set_const_level();
/*
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
    if(error_count == 0)  {
        set_const_level();
    }
   
    if(error_count == 0) 
        return cpp_generator(std::cout);
*/
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
int compiler::resolve_did_references(bool debug_on) {
    int count = 0;
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
                for(int m: *this) if(m == p) break; else if(at(m).type == FTK_EQUALS && node_text(child(m, 0)) == ids[0]) {
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
                    for(int m: get("//EQUALS", parent(nid))) if(node_text(child(m, 0)) == ids[1]) {
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
                    for(int m: get("//EQUALS", nid)) if(nids+"."+node_text(child(m, 0)) == stru::join(ids, ".")) {
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
            // Set the current valx ref attribute to the node with the definition
            ref.set(valx_n, last_match);
            if(vtype.has(last_match)) 
                vtype.copy(last_match, parent(p));
            if(const_level.has(last_match))
                const_level.copy(last_match, parent(p));
            ++count;
        }
        if(ids.size() == 1 && last_match == 0 && grpcu::store::error_code(ids[0]) >= 0) {
            // FIXME the grpc error code could be hidden by a local variable or another enum. 
            // Would it make sense to issue a warning in such a case? maybe in the error context only?
            vtype.set(valx_n, value_type(fvt_int));
            const_level.set(valx_n, 3);
            last_match = 1; 
            ++count;
        }
        if(last_match == 0) { 
            std::string mm; // main match TODO check for interference with other matches
            if(gstore.lookup(mm, ids, &enums, grpcu::MO_ENUMS) == 1) {
                vtype.set(valx_n, value_type(fvt_enum, gstore.enum_full_name_for_value(mm)));
                const_level.set(valx_n, 3);
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
    for(int s: get("//valx")) {
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
    return count;
}
int compiler::resolve_input_references(bool debug_on) {
    int count = 0;
    for(int node: get("//valx/ndid")) {
        for(int input_node: get("//INPUT"))
            if(node_text(first_child(node)) == node_text(first_child(input_node))) {
                std::vector<std::string> field_names;
                for(unsigned ci = 1, ce = child_count(node); ci < ce; ++ci)
                    field_names.push_back(node_text(child(node, ci)));
                value_type vt = vtype(input_node).field_type(field_names);
                if(vt.is_null()) {
                    error(at(first_child(node)), stru::sfmt() << "\"" << stru::join(field_names, ".") << "\" does not refer to a field in the mesage of type \"" << vt.gname << "\"");
                } else {
                    vtype.set(parent(node), vt);
                    const_level.set(parent(node), 1);
                    ++count;
                }
                break;
            }
    }
    return count;
}
/**
 * Resolve the RPC for entries, along with their input and return type
 */
int compiler::resolve_entries(bool debug_on) {
    int first_auto_input_entry = 0;
    std::string auto_input_type;
    int count = 0;

    for(auto p: get("//ENTRY/did")) {
        auto ids = get_ids(p);
        std::set<std::string> methods;
        std::string matched_method; 
        if(gstore.lookup(matched_method, ids, &methods, grpcu::MO_METHODS) != 1) {
            // if not a unique match
            if(methods.size() == 0) {
                error(at(p), stru::sfmt() << "rpc not found \"" << stru::join(ids, ".") << "\"");
            } else {
                error(at(p), stru::sfmt() << "ambiguous reference to rpc \"" << stru::join(ids, ".") << "\"");
                note(at(p), stru::sfmt() << "matches " << stru::join(methods, ", ", " and ", "", "\"", "\""));
            }
            // keep going to possibly report more errors
            continue;
        }
        ++count;
        // we have an unique match
        rpc.set(parent(p), matched_method);
        // set the vtype attribute for ENTRY to the type of the result
        vtype.set(parent(p), gstore.message_to_value_type(gstore.method_output_full_name(matched_method), ""));
        // if this entry has an input, set its type
        auto input_type = gstore.message_to_value_type(gstore.method_input_full_name(matched_method), "");
        bool input_type_set = false;
        for(int i: get("INPUT", parent(p))) {
            vtype.set(i, input_type);
            input_type_set = true;
        }
        if(input_type_set) 
            continue;

        if(first_auto_input_entry == 0) {
            first_auto_input_entry = parent(p);
            auto_input_type = gstore.method_input_full_name(matched_method);
            for(int i: get("//flow/INPUT")) 
                vtype.set(i, input_type);
            continue;
        }
        // issue an error if out input type doesn't match the first deduced type
        if(gstore.method_input_full_name(matched_method) != auto_input_type) {
            error(at(p), stru::sfmt() << "input type mismatch, \"" << gstore.method_input_full_name(matched_method) << "\" instead of \"" << auto_input_type << "\"");
            notep(at(first_auto_input_entry),  stru::sfmt() << "first implied from here");
        }
    }
    return count;
}
int compiler::resolve_nodes(bool debug_on) {
    int count = 0;
    for(auto o: get("//NODE/block/OUTPUT/valx/msgexp/did")) if(!rpc.has(ancestor(o, 5))) {
        std::cerr << "NODE OUTPUT at " << o << "\n";
        auto ids = get_ids(o);
        std::set<std::string> methods;
        std::string matched_method; 
        if(gstore.lookup(matched_method, ids, &methods, grpcu::MO_METHODS) != 1) {
            // if not a unique match
            if(methods.size() == 0) {
                error(at(o), stru::sfmt() << "rpc not found \"" << stru::join(ids, ".") << "\"");
            } else {
                error(at(o), stru::sfmt() << "ambiguous reference to rpc \"" << stru::join(ids, ".") << "\"");
                note(at(o), stru::sfmt() << "matches " << stru::join(methods, ", ", " and ", "", "\"", "\""));
            }
            continue;
        }
        ++count;
        rpc.set(ancestor(o, 5), matched_method);
        // node's zd types
        vtype.set(ancestor(o, 3), gstore.message_to_value_type(gstore.method_output_full_name(matched_method), ""));
        vtype.set(ancestor(o, 1), gstore.message_to_value_type(gstore.method_input_full_name(matched_method), ""));
    }
    for(auto r: get("//(NODE|ENTRY)/block/RETURN/valx/msgexp/did")) if(!vtype.has(ancestor(r, 3))) {
        std::cerr << "NODE RETURN at " << r << "\n";
        auto ids = get_ids(r);
        std::set<std::string> messages;
        std::string matched_message; 
        if(gstore.lookup(matched_message, ids, &messages, grpcu::MO_MESSAGES) != 1) {
            // if not a unique match
            if(messages.size() == 0) {
                error(at(r), stru::sfmt() << "message not found \"" << stru::join(ids, ".") << "\"");
            } else {
                error(at(r), stru::sfmt() << "ambiguous reference to message \"" << stru::join(ids, ".") << "\"");
                note(at(r), stru::sfmt() << "matches " << stru::join(messages, ", ", " and ", "", "\"", "\""));
            }
            continue;
        }
        ++count;
        // node's zd type
        vtype.set(ancestor(r, 3), gstore.message_to_value_type(matched_message, ""));
        vtype.set(ancestor(r, 1), gstore.message_to_value_type(matched_message, ""));
    }
    for(auto v: get("//NODE/block/(RETURN|OUTPUT)/valx")) if(vtype.has(v) && !vtype.has(ancestor(v, 3))) {
        ++count;
        // TODO check for type conflict
        vtype.copy(v, ancestor(v, 3));
    }
    return count;
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
/*
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
*/
std::set<std::string> compiler::get_referenced_families(int valx_node) const {
    std::set<std::string> families;
    std::vector<int> refd;
    refd.push_back(valx_node);
    unsigned i = 0;
    while(i < refd.size()) {
        int rvx = refd[i++];
        if(node_type(child(rvx, 0)) == FTK_ndid) 
            families.insert(node_text(child(child(rvx, 0), 0)));
        for(int vx: get("//valx", rvx)) {
            if(ref.has(vx)) {
                if(std::find(refd.begin(), refd.end(), ref(vx)) == refd.end())
                    refd.push_back(ref(vx));
            } else if(node_type(child(vx, 0)) == FTK_ndid) {
                families.insert(node_text(descendant(vx, 0, 0)));
            }
        }
    }
    return families;
}
}
