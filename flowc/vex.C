#include <fstream>
#include <iostream>
#include "stru1.H"
#include "ansi-escapes.H"
#include "helpo.H"
#include "vex.H"

#define VEX_NAME "vex"
bool ansi::use_escapes = true;
char const *get_version() {
    return BUILD_VERSION;
}
char const *get_build_id() {
    return BUILD_ID;
}

struct environment {
    std::vector<std::string> end() const {
        return std::vector<std::string>();
    }
    std::vector<std::string> get(char const *name) const {
        auto envv = getenv(name);
        if(envv == nullptr) return end();
        std::vector<std::string> r;
        if(strcmp(name, "PATH") == 0 || stru1::ends_with(name, "_PATH")) {
            stru1::split(r, envv, ":");
        } else {
            r.push_back(envv);
        }
        return r;
    }
};

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

extern std::string get_vex_help();

int main(int argc, char *argv[]) {
    helpo::opts opts;
    if(opts.parse(get_vex_help(), argc, argv) != 0 || opts.have("version") || opts.have("help") || argc <= 2) {
        ansi::use_escapes = opts.optb("color", ansi::use_escapes && isatty(fileno(stdout)) && isatty(fileno(stderr)));

        if(opts.have("version")) {
            std::cout << VEX_NAME << " " << get_version() << " (" << get_build_id() << ")\n";
#if defined(__clang__)          
            std::cout << "clang++ " << __clang_version__ << " (" << __cplusplus << ")\n";
#elif defined(__GNUC__) 
            std::cout << "g++ " << __VERSION__ << " (" << __cplusplus << ")\n";
#else
#endif
            return 0;
        } else if(opts.have("help") || argc <= 2) {
            ansi::emphasize(std::cout, ansi::emphasize(get_vex_help(), ansi::escape(ANSI_BLUE)), ansi::escape(ANSI_BOLD), "--", " \r\n\t =,;/", true, true) << "\n";
            return opts.have("help")? 0: 1;
        } else {
            std::cout << "Use --help to see the command line usage and all available options\n\n";
            return 1;
        }
    }

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

    bool env_used = false;

    std::vector<std::map<std::string, std::vector<std::string>>> mv1;
    std::vector<std::map<std::string, std::vector<std::string>>> mv2;

    for(int i = 2; i < argc; ++i) {
        int rc = 0;
        if(strcmp(argv[i], "--environment") == 0 || strcmp(argv[i], "--env") == 0 || strcmp(argv[i], "-e") == 0) {
            if(env_used) {
                std::cerr << "environment has already been used once\n";
                rc = 1;
            }
            env_used = true;
        } else {
            if(env_used) mv2.emplace_back(); 
            else mv1.emplace_back();
            auto &map = env_used? mv2.back(): mv1.back();
    
            std::ifstream fin;
            std::istream *in = &std::cin;

            if(strcmp(argv[i], "-") == 0) {
                if(stdin_used != 0) {
                    std::cerr << "stdin has already been used\n";
                    rc = 1;
                } else {
                    stdin_used = i;
                    rc = read_json_file(map, std::cin);
                    if(rc != 0) 
                        std::cerr << "<stdin>: error reading json variables\n";
                }
            } else {
                std::ifstream fin;
                fin.open(argv[i]);
                if(!fin.is_open()) {
                    std::cerr << argv[i] << ": error reading file\n";
                    rc = 1;
                } else {
                    rc = read_json_file(map, fin);
                    if(rc != 0) 
                        std::cerr << argv[i] << ": error reading json variables\n";
                }
            }
        }
        if(rc != 0) 
            return rc;
    }
    int rc = env_used? 
        vex::expand(std::cout, *in, mv1, environment(), mv2):
        vex::expand(std::cout, *in, mv1);
    return rc;
}
