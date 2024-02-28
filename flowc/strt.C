#include <iostream>
#include <fstream>
#include <cstring>
#include "stru.H"
#include "filu.H"

#define CMD_NAME "strt"
char const *get_version() {
    return BUILD_VERSION;
}
char const *get_build_id() {
    return BUILD_ID;
}

std::string transform(std::string const &transf, std::string value, int &rc) {

    if(transf == "lower") {
        value = stru::to_lower(value);
    } else if(transf == "upper") {
        value = stru::to_upper(value);
    } else if(transf == "option" || transf == "cname") {
        value = stru::to_option(value, false);
    } else if(transf == "id" || transf == "identifier") {
        value = stru::to_identifier(value);
    } else if(transf == "html") {
        value = stru::html_escape(value);
    } else if(transf == "C" || transf == "c" || transf == "string") {
        value = stru::c_escape(value);
    } else if(transf == "shell" || transf == "sh") {
        value = stru::sh_escape(value);
    } else if(transf == "shell_comment" || transf == "shcom") {
        value = stru::to_line_comment(value, "# ");
    } else if(transf == "C_comment" || transf == "ccom" || transf == "c_comment") {
        value = stru::to_line_comment(value, "// ");
    } else if(transf == "trim") {
        value = stru::strip(value);
    } else if(transf == "ltrim") {
        value = stru::lstrip(nullptr, value, "\t\r\a\b\v\f\n ");
    } else if(transf == "rtrim") {
        value = stru::rstrip(nullptr, value, "\t\r\a\b\v\f\n ");
    } else if(transf == "lchop") {
        value = value.length() <= 1? std::string(): value.substr(1);
    } else if(transf == "rchop") {
        value = value.length() <= 1? std::string(): value.substr(0, value.length()-1);
    } else if(transf == "chop") {
        value = value.length() <= 2? std::string(): value.substr(1, value.length()-2);
    } else if(transf == "camelize") {
        value = stru::camelize(value);
    } else if(transf == "bool") {
        value = stru::string_to_bool(value)? "true": "false";
    } else if(transf == "decamelize") {
        value = stru::decamelize(value);
    } else if(transf == "underscore") {
        value = stru::to_underscore(value);
    } else if(transf == "base64") {
        value = stru::base64(value);
    } else if(transf == "base64d") {
        value = stru::base64d(value);
    } else if(transf == "json") {
        value = stru::json_escape(value);
    } else {
        std::cerr << "unrecognized transform '" << transf << "'\n";
        ++rc;
    }
    return value;
}

int main(int argc, char *argv[]) {
    if(argc > 1 && (std::strcmp(argv[1], "--version") == 0 || std::strcmp(argv[1], "-v") == 0)) {
            std::cout << CMD_NAME << " " << get_version() << " (" << get_build_id() << ")\n";
#if defined(__clang__)          
            std::cout << "clang++ " << __clang_version__ << " (" << __cplusplus << ")\n";
#elif defined(__GNUC__) 
            std::cout << "g++ " << __VERSION__ << " (" << __cplusplus << ")\n";
#else
#endif
        return 0;
    }
    
    if(argc <= 2 || argc == 3 && (std::strcmp(argv[2], "-f") == 0 || std::strcmp(argv[2], "-c") == 0)) {
        std::cerr << "Usage " << CMD_NAME << " <transform> ([-c] string... | -f filename...) \n";
        return 1;
    } 
    int rc = 0;
    transform(argv[1], "", rc);
    if(rc != 0) 
        return rc;

    if(std::strcmp(argv[2], "-f") == 0) 
        for(int i = 3; i < argc; ++i) {
            std::ifstream fs(argv[i]);
            if(!fs.is_open()) {
                std::cerr << argv[i] << ": can't read file\n";
                rc = 1;
                break;
            }
            std::cout << transform(argv[1], filu::reads(fs), rc) << "\n";
        } 
    else 
        for(int i = 2; i < argc; ++i) {
            if(i == 2 && std::strcmp(argv[i], "-c") == 0)
                continue;
            std::cout << transform(argv[1], argv[i], rc) << "\n";
        }
    return rc;
}
