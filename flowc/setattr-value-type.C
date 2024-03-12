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
    if(!l.is_null()) switch(op) {
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
    if(!l.is_null() && !r.is_null()) switch(op) {
        case FTK_PLUS:
            if(l.type == r.type && l.is_basic()) 
                vt = l;
            else if(l.is_basic() && r.is_basic())
                vt = fc::value_type(l.is_str()? fc::fvt_str: fc::fvt_flt);
            else if(l.elem_type().type == r.type)
                vt = l;
            else if(l.type == r.elem_type().type)
                vt = r;
            else if(l.is_array() && l.elem_type().type == r.elem_type().type)
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
/**
 * Function definition table
 * name -> preserve const return type, min num args, arg types, preserve const
 */
std::multimap<std::string, std::tuple<bool, fc::value_type, int, fc::value_type>> predef_rt = {
    {"after",     { true, fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_str)})} },
    {"batch",     { true, fc::value_type(2, fc::value_type(fc::fvt_any)), 2, fc::value_type({fc::value_type(1, fc::value_type(fc::fvt_any)), fc::value_type(fc::fvt_int)})} },
    {"before",    { true, fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_str)})} },
    {"ceil",      { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_flt)})} },
    {"exp",       { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_flt)})} },
    {"float",     { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_basic)})} },
    {"floor",     { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_flt)})} },
    {"flt",       { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_basic)})} },
    {"int",       { true, fc::value_type(fc::fvt_int), 1, fc::value_type({fc::value_type(fc::fvt_basic)})} },
    {"integer",   { true, fc::value_type(fc::fvt_int), 1, fc::value_type({fc::value_type(fc::fvt_basic)})},  },
    {"join",      { true, fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(1, fc::value_type(fc::fvt_basic)), fc::value_type(fc::fvt_str)})}, },
    {"join",      { true, fc::value_type(fc::fvt_str), 2, fc::value_type({fc::value_type(1, fc::value_type(fc::fvt_basic)), fc::value_type(1, fc::value_type(fc::fvt_str))})}, },
    {"length",    { true, fc::value_type(fc::fvt_int), 1, fc::value_type({fc::value_type(fc::fvt_str)})} },
    {"ln",        { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_flt)})}, },
    {"now",       { false, fc::value_type(fc::fvt_int), 0, fc::value_type()}, },
    {"rand",      { false, fc::value_type(fc::fvt_flt), 0, fc::value_type({fc::value_type(fc::fvt_int)})}, },
    {"remainder", { true, fc::value_type(fc::fvt_flt), 2, fc::value_type({fc::value_type(fc::fvt_int), fc::value_type(fc::fvt_int)})}, },
    {"round",     { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_flt)})},  },
    {"split",     { true, fc::value_type(1, fc::value_type(fc::fvt_str)), 1, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_str)})} },
    {"str",       { true, fc::value_type(fc::fvt_str), 1, fc::value_type({fc::value_type(fc::fvt_basic)})} },
    {"substr",    { true, fc::value_type(fc::fvt_str), 3, fc::value_type({fc::value_type(fc::fvt_str), fc::value_type(fc::fvt_int), fc::value_type(fc::fvt_int)})},  },
    {"tocname",   { true, fc::value_type(fc::fvt_str), 1, fc::value_type({fc::value_type(fc::fvt_str)})} },
    {"toid",      { true, fc::value_type(fc::fvt_str), 1, fc::value_type({fc::value_type(fc::fvt_str)})} },
    {"tolower",   { true, fc::value_type(fc::fvt_str), 1, fc::value_type({fc::value_type(fc::fvt_str)})} },
    {"toupper",   { true, fc::value_type(fc::fvt_str), 1, fc::value_type({fc::value_type(fc::fvt_str)})} },
    {"trunc",     { true, fc::value_type(fc::fvt_flt), 1, fc::value_type({fc::value_type(fc::fvt_flt)})} },
};
fc::value_type fun_type(std::string fname, std::vector<fc::value_type> const &avt) {
    static int show = 0;
    if(++show == 1) {
        std::cerr << predef_rt << "\n";
    }
    fc::value_type vt;
    for(auto pp = predef_rt.equal_range(fname); pp.first != pp.second; ++pp.first) {
        int rdim = std::get<3>(pp.first->second).can_be_called_with(fc::value_type(avt.begin(), avt.end()));
        if(rdim < 0) 
            continue;
        vt = fc::value_type(rdim, std::get<1>(pp.first->second));
        break;
    }
    return vt;
}
}
namespace fc {
int compiler::compute_value_type(int node) {
    int irc = error_count;
    std::map<std::string, int> family_dimensions;
    if(!vtype.has(node)) {
        auto vt = compute_value_type(node, family_dimensions, true);
        if(!vt.is_null())
            vtype.set(node, vt);
    }
    return error_count - irc; 
}
/* Solve expression types by computing them from subexpression types.
 */
value_type compiler::compute_value_type(int node, std::map<std::string, int> const &fam_dims, bool err_check) {
    std::cerr << "Entering with node: " << node << ", dims: " << fam_dims << "\n";
    auto const &n = at(node);
    value_type rvt;
    // did, ndid, msgexp, vala, range, fun, hash, bang, minus 
    if(n.type == FTK_valx) switch(node_type(child(node, 0))) {  
        case FTK_ndid: {
            int mn = main_node_by_type(node_text(descendant(node, 0, 0)));
            if(mn != 0) {
                auto did = stru::join(get_ids(n.children[0]), ".");
                if(!did.empty()) {
                    if(err_check && gstore.field_full_name(vtype.get(mn).struct_name(), did).empty()) {
                        error(n, stru::sfmt() << "message of type \"" << vtype.get(mn).struct_name() << "\" does not have a field \"" << did << "\"");
                        notep(at(mn), stru::sfmt() << "message type deduced from here");
                    }
                    rvt = gstore.field_to_value_type(vtype.get(mn).struct_name(), did, node_text(descendant(node, 0, 0)));
                } else {
                    rvt = vtype(mn);
                }
            }
        }
        break;
        case FTK_did: 
            if(ref.has(child(node, 0)) && vtype.has(ref.get(child(node, 0)))) {
                // WARNING not covered
                //vtype.copy(ref.get(n.children[0]), n.children[0]); 
                rvt = vtype(ref.get(child(node, 0)));
            }
        break;
        case FTK_msgexp: 
            if(vtype.has(descendant(node, 0, 0)))
                rvt = vtype(descendant(node, 0, 0));
        break;
        case FTK_MINUS: 
            if(vtype.has(n.children[1])) {
                value_type t = op1_type(FTK_MINUS, vtype.get(child(node, 1)));
                rvt = t;
                if(err_check && t.type == fvt_none)
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
        case FTK_POW: {
          /*                
            value_type tl = compute_value_type(child(node, 1), fam_dims, err_check);
            value_type tr = compute_value_type(child(node, 2), fam_dims, err_check);
            if(!t1.is_null() && !t2.is_null()) {
                rvt = op2_type(node_type(child(node, 0)), tl, tr);
                if(err_check && rvt.type == fvt_none)
                    error(n, stru::sfmt() << "incompatible types for operator \"" << node_text(child(node, 0)) << "\"");
            }
            */

            if(vtype.has(child(node, 1)) && vtype.has(child(node, 2))) {
                rvt = op2_type(node_type(child(node, 0)), vtype.get(child(node, 1)), vtype.get(child(node, 2)));
                if(err_check && rvt.type == fvt_none)
                    error(n, stru::sfmt() << "incompatible types for operator \"" << node_text(child(node, 0)) << "\"");
            }
        } break;
        case FTK_QUESTION:
            // TODO when error checking, make sure both branches return compatible types
            //
            if(vtype.has(child(node, 3)))
                rvt = vtype(child(node, 3));
        break;
        case FTK_fun:
            /*** TODO We should never get here! 
             */
            if(vtype.has(child(node, 0)))
                rvt = vtype(child(node, 0));
        break;
        default:
            std::cerr << "internal: not implemented propagating value type for \"valx\" of type \"" << node_token(child(node, 0)) << "\" (" << node_type(child(node, 0))  << ")\n"; 
            print_ast(node);
            assert(false);
    } else if(n.type == FTK_list) {
        unsigned solved = 0;
        value_type t(fvt_struct);
        for(int fa: n.children) if(vtype.has(child(fa, 1))) {
            value_type ft = vtype.get(child(fa, 1));
            ft.fname = node_text(child(fa, 0));
            t.add_type(ft);
            ++solved;
        }
        if(solved == n.children.size())
            rvt = t;
    } else if(n.type == FTK_fun) {
        std::vector<value_type> avt;
        for(unsigned a = 1, e = n.children.size(); a < e; ++a) 
            if(vtype.has(child(node, a))) 
                avt.push_back(vtype.get(child(node, a)));
        if(avt.size()+1 == n.children.size()) {
            value_type vt = fun_type(node_text(child(node, 0)), avt);
            rvt = vt;
        }
    } else {
        std::cerr << "internal: propagating value type for \"" << tk_to_string(n.type) << "\" (" << n.type << ")\n"; 
        assert(false);
    }
    std::cerr << "Done with node: " << node << "\n";
    return rvt;
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
                compute_value_type(p);
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
