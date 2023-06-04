#include "flow-comp.H"
#include <string>
#include <vector>

namespace fc {

std::vector<std::string> compiler::get_ids(int node) const {
        std::vector<std::string> ids;
        int c = 0;
        switch(at(node).type) {
            case FTK_did:
                for(int n: at(node).children) {
                    ids.push_back(at(n).token.text);
                    ++c;
                }
                break;
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

        }
        return ids;
}
}
