#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "flow-comp.H"
#include "sfmt.H"
#include "stru.H"
#include "value-type.H"
#include "container-strinsop.H" 
#include "func-table.H"

namespace fc {
    /*
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
*/
value_type compiler::compute_value_type(int node, bool check_errors) {
    //auto const &n = at(node);
    std::vector<value_type> avt;
    value_type rvt;
    int op_node = node;
    std::vector<std::string> field_names;

    switch(node_type(node)) {
        case FTK_list:
            for(unsigned a = 0, e = child_count(node); a < e; ++a) 
                if(node_type(child(node, a)) == FTK_fassgn) {
                    if(vtype.has(last_child(child(node, a)))) {
                        avt.push_back(vtype.get(last_child(child(node, a))));
                        avt.back().fname = node_text(first_child(child(node, a)));
                    }
                } else {
                    if(vtype.has(child(node, a))) 
                        avt.push_back(vtype(child(node, a)));
                }
            if(avt.size() == child_count(node)) 
                rvt = value_type(avt.begin(), avt.end());
            break;
        case FTK_fassgn: {
            int amsg = ancestor(node, 2), cmsg = last_child(last_child(node));
            if(amsg && cmsg && node_type(amsg) == FTK_msgexp && node_type(cmsg) == FTK_msgexp &&
                         vtype.has(amsg) && !vtype.has(cmsg)) {
                std::string fid = node_text(first_child(node));
                vtype.set(cmsg, vtype(amsg).field_type(fid).zd_type());
                if(vtype(cmsg).is_null())
                    error(at(first_child(node)), stru::sfmt() << "unknown field \"" << fid << "\"");
            }
        }
        break;
        case FTK_valx:
            switch(node_type(first_child(node))) {
                case FTK_msgexp:
                    // list is 
                    if(vtype.has(first_child(node)) && vtype.has(last_child(first_child(node)))) {
                        value_type t = vtype(last_child(first_child(node)));
                        value_type m = vtype(first_child(node));
                        value_type r = node_type(parent(node)) == FTK_RETURN || node_type(parent(node)) == FTK_OUTPUT? vtype(parent(node)): m;
                        value_type v = rpc_type(r, m, t.inf, true);
                        if(!v.is_null()) 
                            vtype.set(node, v);
                    }
                    break;
                case FTK_did:
                    assert(vtype.has(node));
                    break;
                case FTK_ndid:
                    for(int n: get("//NODE")) if(node_text(first_child(n)) == node_text(first_child(first_child(node))) && vtype.has(n)) {
                        // TODO wait for all the nodes in the same family to be typed before attempting this
                        int ndid = first_child(node);
                        for(unsigned ci = 1, ce = child_count(ndid); ci < ce; ++ci)
                            field_names.push_back(node_text(child(ndid, ci)));
                        rvt = value_type(vtype(n).dimension(), vtype(n).zd_type().field_type(field_names));
                        //std::cerr << "LOOKING FOR " << field_names << " in " << vtype(n) << ", and got: " << rvt << "\n";
                        break;
                    }
                    break;
                case FTK_fun:
                    op_node = first_child(node);
                default:
                    // do not look up the function again if we already failed
                    if(fun.has(first_child(node)))
                        break;
                    for(unsigned a = 1, e = child_count(op_node); a < e; ++a) 
                        if(vtype.has(child(op_node, a))) 
                            avt.push_back(vtype.get(child(op_node, a)));
                    if(avt.size()+1 == child_count(op_node)) {
                        // lookup the function/operator all the arguments are available
                        int fun_x = lookup_fun(&rvt, node_text(first_child(op_node)), avt, opts.allow_int_promotions);
                        if(rvt.is_null() && check_errors) {
                            if(op_node == node) 
                                error(at(first_child(op_node)), stru::sfmt() << "no match for operator \"" << node_text(first_child(op_node)) << "\" with operands ..");
                            else
                                error(at(first_child(op_node)), stru::sfmt() << "no match for function \"" << node_text(first_child(op_node)) << "\" with these arguments");
                        }
                        fun.set(first_child(node), fun_x);
                    }
            }
            break;
        default:
            std::cerr << "oooopsy!\n";
            assert(false);
    }
    return rvt;
} 
int compiler::resolve_expressions() {
    int count = 0, prev_count;
    do {
        prev_count = count;
        for(int n: get("//(valx|fassgn|msgexp/list)")) 
            if(!vtype.has(n)) {
                auto n_vtype = compute_value_type(n, true);
                if(!n_vtype.is_null()) {
                    vtype.set(n, n_vtype);
                    ++count;
                }
            }
        std::cerr << "resolve_expressions iteration: " << count << " solved\n";
    } while(count != prev_count);
    std::cerr << "resolve_expressions pass: " << count << " solved\n";
    return count;
}
}
