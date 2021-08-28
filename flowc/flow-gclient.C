#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>

#include "flow-compiler.H"
#include "stru1.H"
#include "vex.H"
#include "grpc-helpers.H"

using namespace stru1;

int flow_compiler::find_in_blck(int block_node, std::string const &name, int *pos) const {
    auto blp = block_store.find(block_node);
    if(blp == block_store.end()) return 0;
    int lpos = 0;
    if(pos == nullptr) pos = &lpos;
    for(int e = blp->second.size(); *pos < e; ++(*pos)) 
        if(name == blp->second[*pos].first) 
            return blp->second[(*pos)++].second;
    return 0;
}

int flow_compiler::genc_client_source(std::string const &client_src) {
    int error_count = 0;
    DEBUG_ENTER;
    OFSTREAM_SE(out, client_src);

    std::map<std::string, std::vector<std::string>> local_vars;
    std::vector<MethodDescriptor const *> methods;
    for(auto const &ep: named_blocks) if(ep.second.first == "entry")  
        methods.push_back(method_descriptor(ep.second.second));
    
    for(auto &rn: referenced_nodes) if(!rn.second.no_call) {
        auto cli_node = rn.first;
        if(type(cli_node) == "container") 
            continue;
        methods.push_back(method_descriptor(cli_node));
    }
    if(methods.size() < 1) {
        pcerr.AddError(main_file, -1, 0, "no service entry or node found, cannot generate client");
        return 1;
    }
    if(DEBUG_GENC) {
        std::string ofn = client_src + "-global.json";
        OFSTREAM_SE(outj, ofn);
        stru1::to_json(outj, global_vars);
    }
    if(DEBUG_GENC) {
        std::string ofn = client_src + "-local.json";
        OFSTREAM_SE(outj, ofn);
        stru1::to_json(outj, local_vars);
    }
    extern char const *template_client_C;
    auto global_smap = vex::make_smap(global_vars);
    auto local_smap = vex::make_smap(local_vars);
    vex::expand(out, template_client_C, vex::make_cmap(local_smap, global_smap));
    DEBUG_LEAVE;
    return error_count;
}
