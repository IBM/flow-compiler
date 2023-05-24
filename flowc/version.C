#include <string>
#include <set>

#include "flow-templates.H"
#include "flow-comp.H"

namespace fc {
std::string compiler::get_version() {
    return BUILD_VERSION;
}
std::string compiler::get_build_id() {
    return BUILD_ID;
}
std::string compiler::get_default_runtime() {
    return BASE_IMAGE;
}
std::set<std::string> compiler::available_runtimes() {
    std::set<std::string> runtimes;
    for(auto &ss: templates::ztemplate_runtime_Dockerfile) 
        runtimes.insert(ss.first);
    return runtimes;
}
}
