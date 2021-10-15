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
    int lpos = 0;
    if(pos == nullptr) pos = &lpos;
    if(block_node != 0) {
        auto const &mn = at(block_node);
        for(int e = mn.children.size(); *pos < e; ++(*pos))
            if(get_id(at(mn.children[*pos]).children[0]) == name)
                return at(mn.children[(*pos)++]).children[1];
    }
    return 0;
}

int flow_compiler::genc_client_source(std::string const &client_src) {
    int error_count = 0;
    DEBUG_ENTER;
    OFSTREAM_SE(out, client_src);

    std::map<std::string, std::vector<std::string>> local_vars;
    std::vector<MethodDescriptor const *> methods;
    for(int n: *this) if(at(n).type == FTK_ENTRY) 
        methods.push_back(method_descriptor(n));
    
    for(int n: get_all_referenced_nodes()) if(method_descriptor(n) != nullptr) 
        methods.push_back(method_descriptor(n));
    
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
