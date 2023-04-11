#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>

#include "flow-compiler.H"
#include "flow-templates.H"
#include "stru1.H"
#include "vex.H"
#include "grpc-helpers.H"

using namespace stru1;

int flow_compiler::genc_cc_client(std::string const &ccfn) {
    int error_count = 0;
    DEBUG_ENTER;
    OFSTREAM_SE(out, ccfn);

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
        std::string ofn = ccfn + "-global.json";
        OFSTREAM_SE(outj, ofn);
        stru1::to_json(outj, global_vars);
    }
    if(DEBUG_GENC) {
        std::string ofn = ccfn + "-local.json";
        OFSTREAM_SE(outj, ofn);
        stru1::to_json(outj, local_vars);
    }
    vex::expand(out, templates::client_C(), "client.C", local_vars, global_vars);
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_py_client(std::string const &pyfn) {
    int error_count = 0;
    DEBUG_ENTER;
    OFSTREAM_SE(outf, pyfn);
    vex::expand(outf, templates::client_py(), "client.py", global_vars);
    DEBUG_LEAVE;
    return error_count;
}
