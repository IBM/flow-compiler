#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>

#include "flow-compiler.H"
#include "stru.H"
#include "grpc-helpers.H"

int flow_compiler::genc_protobuf() { 
    int error_count = 0;
    DEBUG_ENTER;
    for(auto fdp: fdps) {
        std::string file(fdp->name());
        std::string ccc = protocc + file;
        DEBUG_CHECK(ccc);
        if(system(ccc.c_str()) != 0) {
            pcerr.AddError(file, -1, 0, "failed to generate protobuf code");
            ++error_count;
        }
    }
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_grpc() { 
    int error_count = 0;
    DEBUG_ENTER;
    for(auto fdp: fdps) if(fdp->service_count() > 0) {
        std::string file(fdp->name());
        std::string ccc = grpccc + file;
        DEBUG_CHECK(ccc);
        if(system(ccc.c_str()) != 0) {
            pcerr.AddError(file, -1, 0, "failed to generate gRPC code");
            ++error_count;
        }
    }
    DEBUG_LEAVE;
    return error_count;
}

