#ifndef H_GRPCU_H
#define H_GRPCU_H

#include <string>
#include <set>
#include <vector>

namespace grpcu {
class store {
    std::vector<void const *> file_descriptors;
    std::set<std::string> filenames;
    std::string grpccc, protocc; 
public:
    int add_to_proto_path(std::string directory, std::string mapped_to="");
    int import_file(std::string filename, bool add_to_path);
    int lookup(std::string &match, std::vector<std::string> did, std::set<std::string> *allmp=nullptr, bool match_methods=true, bool match_messages=true, bool match_enums=true) const;
    
    static int error_code(std::string id);
};

}

#endif
