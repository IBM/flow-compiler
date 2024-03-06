#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "flow-comp.H"
#include "sfmt.H"
#include "stru.H"
#include "value-type.H"
#include "container-strinsop.H" 

namespace {
fc::value_type op1_type(int op, fc::value_type l) {
    fc::value_type vt;
    switch(op) {
        case FTK_MINUS:
            if(l.type == fc::fvt_int) 
                vt = fc::value_type(fc::fvt_int);
            else if(l.is_basic()) 
                vt = fc::value_type(fc::fvt_flt);
            break;
        default:
            std::cerr << "internal " << __FUNCTION__ << " not implemented for: " << op << "\n";
            assert(false);
            break;
    }
    return vt;
}
fc::value_type op2_type(int op, fc::value_type l, fc::value_type r) {
    fc::value_type vt;
    switch(op) {
        case FTK_PLUS:
            if(l.type == r.type && l.is_basic()) 
                vt = l;
            else if(l.is_basic() && r.is_basic())
                vt = fc::value_type(l.is_str()? fc::fvt_str: fc::fvt_flt);
            else if(l.a_type().type == r.type)
                vt = l;
            else if(l.type == r.a_type().type)
                vt = r;
            else if(l.is_array() && l.a_type().type == r.a_type().type)
                vt = l;
            break;
        case FTK_STAR:   
            if(l.is_str() && r.is_num())
                vt = fc::value_type(fc::fvt_str);
            else if(l.is_int() && r.is_int())
                vt = fc::value_type(fc::fvt_int);
            else if(l.is_num() && r.is_num())
                vt = fc::value_type(fc::fvt_flt);
            break;
        case FTK_PERCENT: 
        case FTK_SLASH:
        case FTK_POW:
            if(l.is_int() && r.is_int())
                vt = fc::value_type(fc::fvt_int);
            else if(l.is_num() && r.is_num())
                vt = fc::value_type(fc::fvt_flt);
            break;
        default:
            std::cerr << "internal " << __FUNCTION__ << " not implemented for: " << op << "\n";
            assert(false);
            break;
    }
    return vt;
}
std::multimap<std::string, std::tuple<fc::value_type, int, fc::value_type>> predef_rt = {
    {"after", { fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_int)})} },
    {"batch", { fc::value_type(2, fc::value_type(fc::fvt_any)), 2, fc::value_type({fc::value_type(1, fc::value_type(fc::fvt_any)), fc::value_type(fc::fvt_int)})} },
    {"before", { fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_int)})} },
    {"ceil", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_flt)} },
    {"exp", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_flt)} },
    {"float", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_basic)} },
    {"floor", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_flt)} },
    {"flt", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_basic)} },
    {"int", { fc::value_type(fc::fvt_int), 1, fc::value_type(fc::fvt_basic)} },
    {"integer", { fc::value_type(fc::fvt_int), 1, fc::value_type(fc::fvt_basic)},  },
    {"join", { fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(1, fc::value_type(fc::fvt_basic)), fc::value_type(fc::fvt_str)})}, },
    {"join", { fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(1, fc::value_type(fc::fvt_basic)), fc::value_type(1, fc::value_type(fc::fvt_str))})}, },
    {"length", { fc::value_type(fc::fvt_int), 1, fc::value_type(fc::fvt_str)} },
    {"ln", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_flt)}, },
    {"rand", { fc::value_type(fc::fvt_flt), 0, fc::value_type(fc::fvt_int)}, },
    {"remainder", { fc::value_type(fc::fvt_flt), 2,  fc::value_type({fc::value_type(fc::fvt_int), fc::value_type(fc::fvt_int)})}, },
    {"round", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_flt)},  },
    {"split", { fc::value_type(1, fc::value_type(fc::fvt_str)), 1, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_str)})} },
    {"str", { fc::value_type(fc::fvt_str), 1, fc::value_type(fc::fvt_basic)} },
    {"substr", { fc::value_type(fc::fvt_str), 3, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_int), fc::value_type(fc::fvt_int)})},  },
    {"tocname", { fc::value_type(fc::fvt_str), 1, fc::value_type(fc::fvt_str)} },
    {"toid", { fc::value_type(fc::fvt_str), 1, fc::value_type(fc::fvt_str)} },
    {"tolower", { fc::value_type(fc::fvt_str), 1, fc::value_type(fc::fvt_str)} },
    {"toupper", { fc::value_type(fc::fvt_str), 1, fc::value_type(fc::fvt_str)} },
    {"trunc", { fc::value_type(fc::fvt_flt), 1, fc::value_type(fc::fvt_flt)} },
};
fc::value_type fun_type(std::string fname, std::vector<fc::value_type> const &avt) {
    static int show = 0;
    if(++show == 1) {
        std::cerr << predef_rt << "\n";
    }
    fc::value_type vt;
    for(auto pp = predef_rt.equal_range(fname); pp.first != pp.second; ++pp.first) 
        if(std::get<2>(pp.first->second).can_assign_from(fc::value_type(avt.begin(), avt.end()))) {
            vt = std::get<0>(pp.first->second);
            break;
        }
    return vt;
}
}
namespace fc {
/* Solve expression types by computing them from subexpression types.
 */
int compiler::compute_value_type(bool debug_on, int node) {
    std::cerr << "Entering with node: " << node << "\n";
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
                    vtype.set(node, gstore.field_to_value_type(vtype.get(mn).struct_name(), did, node_text(path(node, 0, 0))));
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
        case FTK_fun:
            if(vtype.has(n.children[0]))
                vtype.copy(n.children[0], node);
        break;

        default:
            std::cerr << "internal: not implemented propagating value type for \"valx\" of type \"" << tk_to_string(atc(node, 0).type) << "\" (" << atc(node, 0).type << ")\n"; 
            print_ast(node);
            assert(false);
    } else if(n.type == FTK_list) {
        std::cerr << "WE ARE if vtype for list! Node is " << node << "\n";
        unsigned solved = 0;
        value_type t(fvt_struct);
        for(int fa: n.children) if(vtype.has(child(fa, 1))) {
            value_type ft = vtype.get(child(fa, 1));
            ft.fname = atc(fa, 0).token.text;
            t.add_type(ft);
            ++solved;
        }
        if(solved == n.children.size())
            vtype.set(node, t);
    } else if(n.type == FTK_fun) {
        std::cerr << "WE ARE if vtype for fun! Node is " << node << "\n";
        std::vector<value_type> avt;
        for(unsigned a = 1, e = n.children.size(); a < e; ++a) 
            if(vtype.has(child(node, a))) 
                avt.push_back(vtype.get(child(node, a)));
        if(avt.size()+1 == n.children.size()) {
            value_type vt = fun_type(node_text(child(node, 0)), avt);
            if(!vt.is_null()) vtype.set(node, vt);
        }
    } else {
        std::cerr << "internal: propagating value type for \"" << tk_to_string(n.type) << "\" (" << n.type << ")\n"; 
        assert(false);
    }
    std::cerr << "Done with node: " << node << "\n";
    return error_count - irc;
}
int compiler::propagate_node_return_types(bool debug_on) {
    int irc = error_count;
    // Propagate the return type to the node/entry attribute
    for(auto p: get("//(NODE|ENTRY)/block/RETURN/valx/msgexp")) {
        int nn = ancestor(p, 4);
        if(!vtype.has(nn) && vtype.has(parent(p))) {
            auto nv = vtype(parent(p));
            // since this is a node type, make it reference its own family
            nv.reference = node_text(child(nn, 0));
            vtype.set(nn, nv);
        }
    }
    // At this point some nodes could have return statements without type.
    // If there is a type, and only one, already deduced for this node family, 
    // propagate that type as a requirement. Otherwise generate errors...
    std::map<std::string, std::vector<int>> nfams; 
    for(int p: get("//NODE/1")) { 
        // first node child is always ID in a healthy ast
        if(at(p).type != FTK_ID)
            continue;
        // don't bother with error nodes
        if(get("//ERRCHK", parent(p)).size() != 0)
            continue;
        nfams[node_text(p)].push_back(parent(p));
    }
    return error_count-irc;
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
        propagate_node_return_types(debug_on);
    } while(todo.size() != unfixed_nodes);
    // All node return types are set now, 
    // time to fix up all node references
    for(int n: get("//valx/ndid")) {
        int v = parent(n);
        if(vtype(v).ref().empty()) {
            auto nv = vtype(v);
            nv.reference =  node_text(child(n, 0));
            vtype.update(v, nv);
        }
    }
    return error_count - irc;
}
}
