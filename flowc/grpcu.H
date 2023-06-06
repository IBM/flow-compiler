#ifndef H_GRPCU_H
#define H_GRPCU_H

#include <string>
#include <set>
#include <vector>

namespace fc {
    struct value_type;
}

namespace grpcu {
class store {
    std::vector<void const *> file_descriptors;
    std::set<std::string> filenames;
    std::string grpccc, protocc; 
public:
    int add_to_proto_path(std::string directory, std::string mapped_to="");
    int import_file(std::string filename, bool add_to_path);
    int lookup(std::string &match, std::vector<std::string> did, std::set<std::string> *allmp=nullptr, bool match_methods=true, bool match_messages=true, bool match_enums=true) const;
   
    std::set<void const *> find_methods(std::string name) const;
    std::set<void const *> find_messages(std::string name) const;
    std::set<void const *> find_enum_values(std::string name) const;
    std::string method_output_full_name(std::string name) const;
    std::string method_input_full_name(std::string name) const;
    std::string enum_value_name(std::string name) const;
    std::string enum_value_full_name(std::string name) const;
    std::string enum_full_name_for_value(std::string name) const;
    struct fc::value_type message_to_value_type(std::string name) const;
    int enum_value(std::string name) const;

    static int error_code(std::string id);
};

}

#endif
