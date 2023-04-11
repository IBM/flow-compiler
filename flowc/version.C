#include <string>
#include <map>
#include <set>
#include "flow-templates.H"

char const *get_version() {
    return BUILD_VERSION;
}
char const *get_build_id() {
    return BUILD_ID;
}
char const *get_default_runtime() {
    return BASE_IMAGE;
}
std::set<std::string> available_runtimes() {
    std::set<std::string> runtimes;
    for(auto &ss: templates::ztemplate_runtime_Dockerfile) 
        runtimes.insert(ss.first);
    return runtimes;
}
