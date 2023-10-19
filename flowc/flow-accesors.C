#include "flow-comp.H"
#include <string>
#include <vector>

namespace fc {
/**
 * Grab the values of all ID children, skipping the first when the node is ndid.
 * If the node is ID, grab only its value.
 */
std::vector<std::string> compiler::get_ids(int node) const {
    std::vector<std::string> ids;
    int c = 0;
    switch(at(node).type) {
        case FTK_ndid:
            for(int n: at(node).children) {
                if(c > 0)
                    ids.push_back(at(n).token.text);
                ++c;
            }
            break;
        case FTK_ID:
            ids.push_back(at(node).token.text);
            ++c;
            break;
        default:
            for(int n: at(node).children) 
                if(at(n).type == FTK_ID) {
                    ids.push_back(at(n).token.text);
                    ++c;
                }
            break;
    }
    return ids;
}
int compiler::main_node_by_type(std::string node_type) const {
    // look for any node that mathes the name
    // look through entries with input defined
    // check if it matches the predefined input
    for(int n: get("//(NODE/1|INPUT/ID)"))
        if(vtype.has(parent(n)) && at(n).token.text == node_type)
            return parent(n);
    return 0;
}
}
