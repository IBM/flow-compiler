#include <string>
#include <map>
#include <set>
#include <sys/utsname.h>

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
std::string get_system_info() {
#ifdef BUILD_OS    
    return BUILD_OS;
#else
    utsname unb;
    uname(&unb);
    return std::string(unb.sysname)+" "+unb.release+" "+unb.version+", "+unb.machine;
#endif
}
