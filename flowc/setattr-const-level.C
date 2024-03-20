#include <algorithm>
#include "flow-comp.H"
#include "func-table.H"

#include <iostream>

namespace fc {

int compiler::set_const_level(int node) {
    int added = 0, count = 0, level = 0;
    // repeatedly mark nodes for the entire tree 
    // until no new nodes can be marked
    if(node == 0) 
        for(int root: get("//flow")) {
            for(int p = 1; p < 500; ++p) {
                int newly_added = set_const_level(root);
                if(newly_added == 0) 
                    break;
            }
        }
    else if(!const_level.has(node)) switch(node_type(node)) {
        case FTK_INTEGER: case FTK_STRING: case FTK_FLOAT:
            level = 3;
            break;
        case FTK_DOLLAR:
            level = 2;
            break;
        case FTK_EQUALS: case FTK_fassgn: case FTK_LIMIT:
            added = set_const_level(child(node, 1));
            level = const_level(child(node, 1));
            break;
        case FTK_msgexp:
            if(child_count(node) > 0) {
                added = set_const_level(last_child(node));
                level = const_level(last_child(node));
            } else {
                level = 3;
            }
            break;
        case FTK_fun:
            if(fun(node) > 0) {
                level = std::get<1>(fun_info(fun(node)))? 3: 0;
                for(unsigned i = 1, e = child_count(node); i < e; ++i) {
                    int c = child(node, i);
                    added += set_const_level(c);
                    level = std::min(const_level(c), level);
                }
            }
            break;
        case FTK_COLON: case FTK_range:
            added = set_const_level(child(node, 0))+
                    set_const_level(child(node, 1));
            level = node_type(child(node, 0)) == FTK_STAR? 3: const_level(child(node, 0)); 
            level = std::min(level, node_type(child(node, 1)) == FTK_STAR? 3: const_level(child(node, 1))); 
            break;
        case FTK_NODE: case FTK_ENTRY:
            for(int c: at(node).children)
                added += set_const_level(c);
            level = 3;
            for(int cn: get("valx", node)) 
                level = std::min(level, const_level(cn));
            for(int cn: get("//RETURN/valx", node)) 
                level = std::min(level, const_level(cn));
            for(int cn: get("//ERRCHK", node)) 
                level = std::min(level, const_level(cn));
            for(int cn: get("//OUTPUT", node)) 
                level = 0;
            break;
        case FTK_ndid:
            for(int iidn: get("//flow/INPUT/ID")) if(node_text(child(node, 0)) == node_text(iidn)) {
                level = 1;
                break;
            }
            if(level == 0) {
                level = 3;
                for(int nid: get("//flow/NODE")) 
                    if(node_text(child(node, 0)) == node_text(child(nid, 0))) 
                        level = std::min(level, const_level(nid));
            }
            break;
        case FTK_ERRCHK: case FTK_list:
            level = 3;
            for(int c: at(node).children) {
                added += set_const_level(c);
                level = std::min(const_level(c), level);
            }
            break;
        case FTK_valx: 
            if(ref.has(node)) {
                level = const_level(ref(node));

            } else if(is_operator(first_child(node))) {
                level = 3;
                for(unsigned i = 1, e = child_count(node); i < e; ++i) {
                    int c = child(node, i);
                    added += set_const_level(c);
                    level = std::min(const_level(c), level);
                }
            } else {
                added = set_const_level(first_child(node));
                if(const_level.has(first_child(node))) 
                    level = const_level(first_child(node));
            }
            break;
        default:
            for(int c: at(node).children)
                added += set_const_level(c);
            break;
    }
    if(level > 0) {
        const_level.set(node, level); ++added;
    }
    return added;
}
}
