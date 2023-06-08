#include "flow-comp.H"
#include "stru.H"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fc {
int compiler::walk(int node) {
    int iec = error_count;

    if(node == 0) {
        for(int e: get("//flow/ENTRY")) 
            if(walk(e) != 0) break;
        return error_count - iec;
    }

    print_ast(node);
    int vn = get("//RETURN/valx", node)[0];
    for(int x: subtree(node)) 
        std::cerr << "[" << x << "]\n";
        

    return error_count - iec;
}
}
