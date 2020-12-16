#include <fstream>
#include <iostream>
#include "vex.H"

struct envmap: public vex::amap {
    static bool ends_with(std::string const &name, const std::string &suff) {
        return name.length() >= suff.length() && name.substr(name.length()-suff.length(), suff.length()) == suff;
    }
    static bool is_array_name(std::string const &name) {
        return name == "PATH" || ends_with(name, "_PATH");
    }
    std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
        std::cerr << " ENVIRON(" << name << ")["<< index << "]\n";

        char *envv = getenv(name.c_str());
        if(envv == nullptr)
            return std::make_pair(false, std::string());

        if(is_array_name(name)) {
            int cc = 0;
            char const *cb = envv, *ce = nullptr;
            while(true) {
                ce = strchr(cb, ':');
                if(ce == nullptr || cc == index || index < 0) 
                    break;
                ++cc; 
                cb = ce+1;
            };
            if(ce == nullptr) ce = strchr(cb, '\0');
            if(index < 0 || cc == index)
                return std::make_pair(true, std::string(cb, ce));
            return std::make_pair(false, std::string());
        }
        return std::make_pair(true, envv);
    }
    int gets(std::string const &name) const {
        char *envv = getenv(name.c_str());
        if(envv == nullptr)
            return -1;
        if(is_array_name(name)) {
            int cc = 1;
            char const *cb = envv;
            while(true) {
                char const *ce = strchr(cb, ':');
                if(ce == nullptr) return cc;
                ++cc; 
                cb = ce+1;
            };
        }
        return 1;
    }
};

static int usage(char const *bpath, int rc = 0) {
    std::cout << "Variable substitution from environment and configuration files\n\n";
    std::cout << "Usage: " << bpath << " [options] <TEMPLATE-FILE> [VAR-FILE...]\n\n";
    std::cout << "options:\n";
    std::cout << "  --help      Display this scren end exit\n";
    std::cout << "  --no-env    Do not use environment varaibles\n";
    std::cout << "\n";
    return rc;
}

int main(int argc, char *argv[]) {
    if(argc < 2) 
        return usage(argv[0], 0);

    std::ifstream fin;
    std::istream *in = &std::cin;
    if(strcmp(argv[1], "-") != 0) {
        fin.open(argv[1]);
        if(!fin.is_open()) {
            std::cerr << argv[1] << ": error reading file\n";
            return 1;
        }
        in = &fin;
    }
    vex::lvmap maps;
    envmap em;
    std::map<std::string, std::vector<std::string>> m;
    auto jm = vex::make_vdmap(vex::make_smap(m));
    maps.push_front(&em);
    maps.push_front(&jm);

    int rc = vex::expand(std::cout, *in, maps);
    return rc;
}
