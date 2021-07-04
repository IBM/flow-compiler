#include <fstream>
#include <iostream>
#include "vex.H"
#include "stru1.H"

struct envmap {
    static bool ends_with(std::string const &name, const std::string &suff) {
        return name.length() >= suff.length() && name.substr(name.length()-suff.length(), suff.length()) == suff;
    }
    static bool is_array_name(std::string const &name) {
        return name == "PATH" || ends_with(name, "_PATH");
    }
    std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
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
    std::cout << "Usage: " << bpath << " <TEMPLATE-FILE> [VAR-FILE...]\n\n";
    std::cout << "\n";
    std::cout << "If no VAR-FILE is given the environment variables will be used.\n";
    std::cout << "\n";
    return rc;
}

int read_json_file(std::map<std::string, std::vector<std::string>> &m, std::istream &is) {
    int rc = 0;
    auto jit = std::make_pair(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
    
    bool accept = 0;
    int state = 0;
    std::string t, vn;
    std::vector<std::string> vv;

    while(!accept && rc == 0 && (t = stru1::scan_json(jit)) != "") {switch(state) {
        case 0:
            state = 1;
            if(t != "{") ++rc;
            break;
        case 1:
            vn = stru1::json_unescape(stru1::strip(t, "\""));
            state = 2;
            break;
        case 2:
            state = 3;
            if(t != ":") ++rc;
            break;
        case 3:
            state  = 4;
            if(t != "[") ++rc;
            vv.clear();
            break;
        case 4:
            if(t != "]") {
                state = 5;
                vv.push_back(stru1::json_unescape(stru1::strip(t, "\"")));
                break;
            }
        case 5:
            if(t == ",") {
                state = 4;
            } else if(t == "]") {
                state = 6;
                m[vn] = vv;
            } else {
                ++rc;
            }
            break;
        case 6:
            if(t == ",") {
                state = 1;
            } else if(t == "}") {
                state = 7;
            } else {
                ++rc;
            }
            break;
        case 7:
            accept = true;
            break;

    }
    }
    return rc;
}

int main(int argc, char *argv[]) {
    if(argc < 2) 
        return usage(argv[0], 0);

    int stdin_used = 1;
    std::ifstream fin;
    std::istream *in = &std::cin;
    if(strcmp(argv[1], "-") != 0) {
        stdin_used = 0;
        fin.open(argv[1]);
        if(!fin.is_open()) {
            std::cerr << argv[1] << ": error reading file\n";
            return 1;
        }
        in = &fin;
    } 
    vex::lvmap maps;
    std::vector<std::map<std::string, std::vector<std::string>>> mv(1);
    auto jt = vex::make_vmap(vex::make_smap(mv[0]));
    std::vector<decltype(jt)> mvs;
    mv.resize(argc-2);

    for(int i = 2; i < argc; ++i) {
        int rc = 0;
        if(strcmp(argv[i], "-") == 0) {
            if(stdin_used != 0) {
                std::cerr << "stdin has already been used\n";
                return 1;
            }
            stdin_used = i;
            rc = read_json_file(mv[i-2], std::cin);
            if(rc != 0) 
                std::cerr << "<stdin>: error reading json variables\n";
        } else {
            std::ifstream fin;
            fin.open(argv[i]);
            if(!fin.is_open()) {
                std::cerr << argv[i] << ": error reading file\n";
                return 1;
            }
            rc = read_json_file(mv[i-2], fin);
            if(rc != 0) 
                std::cerr << argv[i] << ": error reading json variables\n";
        }
        if(rc != 0) {
            return rc;
        }
        mvs.push_back(vex::make_vmap(vex::make_smap(mv[i-2])));
    }
    for(auto &m: mvs) 
        maps.push_front(&m);

    int rc = 1;
    switch(mv.size()) {
        case 0:
            rc = vex::expand(std::cout, *in, envmap());
            break;
        case 1:
            rc = vex::expand(std::cout, *in, vex::make_smap(mv.back()));
            break;
        case 2:
            rc = vex::expand(std::cout, *in, vex::make_cmap(vex::make_smap(mv.back()), vex::make_smap(mv[mv.size()-2])));
            break;
        default:
            int rc = vex::expand(std::cout, *in, maps);
            break;
    }
    return rc;
}
