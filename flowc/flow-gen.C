#include "flow-comp.H"
#include "stru.H"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>

namespace fc {
int compiler::walk(int node) {
    int iec = error_count;

    if(node == 0) {
        for(int e: get("//flow/ENTRY")) 
            if(walk(e) != 0) break;
        return error_count - iec;
    }

    std::map<std::string, int> vsym, nsym;
    //print_ast(node);
    int vn = get("//RETURN/valx", node)[0];

    std::vector<std::string> fevo;
    std::vector<std::string> nodq;
    std::deque<int> nnq;
    std::vector<int> done;
    std::map<std::string, std::set<std::string>> deps;

    //for(int x: subtree(node)) 
    //    std::cerr << "[" << x << "]\n";

    nnq.push_back(node);
    while(nnq.size() > 0) {
        done.push_back(nnq.front());
        int cpn = nnq.front();
        std::string cnn = cpn == node? std::string("@")+std::to_string(node): atc(parent(cpn), 0).token.text;
        
        for(int ni: get("//ndid/1", nnq.front())) {
            std::string nn = at(ni).token.text;
            deps[cnn].insert(nn);
            std::string fn = nn+"@"+stru::join(get_ids(parent(ni)), ".");
            if(std::find(fevo.begin(), fevo.end(), fn) == fevo.end())
                fevo.push_back(fn);
            if(std::find(nodq.begin(), nodq.end(), nn) == nodq.end()) {
                nodq.push_back(nn);
                for(int n: get("//NODE/1")) if(at(n).token.text == nn)
                    for(int c: get("valx", parent(n)))
                        nnq.push_back(c);
                for(int n: get("//NODE/1")) if(at(n).token.text == nn)
                    for(int c: get("block", parent(n)))
                        nnq.push_back(c);
            }
        }
        nnq.pop_front();
    }
    std::cerr << "NNQ(" << node << "): " << nodq << "\n";
    std::cerr << "FEV(" << node << "): " << fevo << "\n";
    std::cerr << "DON(" << node << "): " << done << "\n";
    std::cerr << "DEP(" << node << "): " << deps << "\n";

    return error_count - iec;
}
}
