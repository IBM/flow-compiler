#include "flow-comp.H"
#include <string>
#include <vector>

namespace fc {
/**
 * Grab the values of all ID children, skipping the first when the node id ndid.
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
}
