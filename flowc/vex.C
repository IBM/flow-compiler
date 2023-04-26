#include <fstream>
#include <iostream>
#include <cstdlib>
#include "stru.H"
#include "ansi-escapes.H"
#include "helpo.H"
#define VEX_DEBUG
#include "vex.H"

#define VEX_NAME "vex"
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
        if(strcmp(name, "PATH") == 0 || stru::ends_with(name, "_PATH")) {
            stru::split(r, envv, ":");
        } else {
            r.push_back(envv);
        }
        return r;
    }
};
int read_json_file(std::map<std::string, std::vector<std::string>> &m, std::istream &is) {
    int errc = 0;
    auto jit = std::make_pair(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
    
    bool accept = 0;
    int state = 0;
    std::string t, vn, ts;
    std::vector<std::string> vv;

    while(!accept && errc == 0 && (t = stru::scan_json(jit)) != "") {
        ts = stru::json_unescape(stru::strip1(t, "\""));
        switch(state) {
            // accept '{' and wait for variable name    
            case 0:
                state = 1;
                if(t != "{") ++errc;
                break;
            // accept variable name nd wait for ':' separator
            case 1:
                vn = ts;
                state = 2;
                break;
            // accept ':' and wait for variable value
            case 2:
                state = 3;
                if(t != ":") ++errc;
                break;
            // accept begginning of a value list or a value
            case 3:
                vv.clear();
                if(t == "[") {
                    state  = 4;
                } else {
                    state = 6;
                    vv.push_back(ts);
                }
                break;
            // accept the end of the value list or an element of the value list
            case 4:
                if(t == "]") {
                    state = 6;
                } else {
                    vv.push_back(ts);
                    state = 8;
                }
                break;
            // accept a name value pair separator and expect another name value pair or the map end
            case 5:
                if(t == ",") {
                    state = 4;
                } else if(t == "]") {
                    state = 6;
                    m[vn] = vv;
                } else {
                    ++errc;
                }
                break;
            // accept a name value separator ',' or the end of the map '}'
            case 6:
                if(t == ",") {
                    state = 1;
                    m[vn] = vv;
                } else if(t == "}") {
                    state = 7;
                    m[vn] = vv;
                } else {
                    ++errc;
                }
                break;
            case 7:
                accept = true;
                break;
            // aceept a list separator or the end of list
            case 8:
                if(t == "]") {
                    state = 6;
                } else if(t == ",") {
                    state = 4;
                }
                break;
            default:
                break;
        }
    }
    return errc;
}

namespace templates {
std::string vex_help_hlp();
}

int main(int argc, char *argv[], char **envp) {
    helpo::opts opts;
    if(opts.parse(templates::vex_help_hlp(), argc, argv) != 0 || opts.have("version") || opts.have("help") || argc < 2) {
        //ansi::use_escapes = opts.optb("color", ansi::use_escapes && isatty(fileno(stdout)) && isatty(fileno(stderr)));
        std::cerr << (opts.optb("color", isatty(fileno(stderr)) && isatty(fileno(stdout)))? ansi::on: ansi::off);

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
            std::cout << ansi::emphasize(ansi::emphasize(templates::vex_help_hlp(), ANSI_BOLD, "-", " \r\n\t =,;/", true, true), ANSI_BLUE) << "\n";
            //std::cout << ansi::emphasize(templates::vex_help_hlp(), ansi::escape(ANSI_BLUE),  ansi::escape(ANSI_BOLD)) << "\n";
            return opts.have("help")? 0: 1;
        } else {
            std::cout << "Use --help to see the command line usage and all available options\n\n";
            return 1;
        }
    }

    int stdin_used = 1;
    std::ifstream fin;
    std::istream *in = &std::cin;
    std::string template_label("<stdin>");
    if(strcmp(argv[1], "-") != 0) {
        stdin_used = 0;
        fin.open(argv[1]);
        template_label = argv[1];
        if(!fin.is_open()) {
            std::cerr << argv[1] << ": error reading file\n";
            return 1;
        }
        in = &fin;
    } 

    bool env_used = argc == 2;

    std::vector<std::map<std::string, std::vector<std::string>>> mv1;
    std::vector<std::map<std::string, std::vector<std::string>>> mv2;

    int rc;
    for(int i = 2; i < argc; ++i) {
        rc = 0;
        if(strcmp(argv[i], "env:") == 0) {
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
    auto da = env_used?
        vex::make_da(mv1, environment(), mv2):
        vex::make_da(mv1);
    //auto da = vex::make_da(mv1);

    std::istreambuf_iterator<char> eos;
    std::string templ(std::istreambuf_iterator<char>(*in), eos);
    std::string bo = opts.opt("outer-begin", vex::default_escape_strings[0]);
    std::string bi = opts.opt("inner-begin", vex::default_escape_strings[1]);
    std::string ei = opts.opt("inner-end", vex::default_escape_strings[2]);
    std::string eo = opts.opt("outer-end", vex::default_escape_strings[3]);
    rc = vex::pand_impl(std::cout, templ.data(), templ.data()+templ.length(), da, template_label, opts.have("debug"), 
            bo.c_str(), bi.c_str(), ei.c_str(), eo.c_str()
            );
    return rc;
}
