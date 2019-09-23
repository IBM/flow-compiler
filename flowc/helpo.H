#ifndef H_HELPO_H
#define H_HELPO_H
#include <cstring>
#include <cstdlib>
#include <getopt.h>
#include <vector>
#include <map>
#include <iostream>
#include <string>
/**
 * Parse help message and generate structures for getopt_long. 
 * Returns 0 if the command line parsed OK.
 * Returns > 0 when a getopt_long error has been encountered.
 *
 * The help string is parsed for options after the first empty line. 
 *
 * All option lines must start with at least two whitespace characters.
 * The help line that follows must be separated by at least 2 space/tab chars.
 * Example:
 * 
 *      -c, --config-file FILE   
 *              Specify the configuration file to use
 *      -d      Print debug messages
 *      --help  Print usage instructions and exit
 */
namespace helpo {
inline static 
char const *strchre(char const *s, char const *e, char const *c) {
    for(; s != e; ++s) if(strchr(c, *s) != nullptr) return s;
    return e;
}
struct opts {
std::map<std::string, unsigned> option_names;
std::vector<std::vector<std::string>> option_values;

int parse(char const *hlp, int &argc, char ** &argv) {
    int errors = 0;
    bool new_paragraph = true;
    unsigned optn = 0;

    std::string short_opts(":");
    std::vector<struct option> long_opts;

    for(char const *c = hlp, *e = strchr(c, '\n'); e != nullptr; c = e+1, e = strchr(c, '\n')) {
        //std::cout << std::string(c, e) << "\n";
        int cn = 0;
        while(c != e && isspace(*c)) ++cn, ++c;
        if(cn >= 2 && new_paragraph && *c == '-') {

            int has_arg = no_argument;
            unsigned loi = long_opts.size(); 
            std::string so;
            ++optn;
            for(char const *l = c, *r = strchre(c, e, " \t=:,;");  l != nullptr; l = (r == e? nullptr: r+1), r = (l == nullptr? nullptr: strchre(l, e, " \t=:,;"))) {
                if(*l == '-') {
                    if(l+1 != r && *(l+1) == '-') {
                        std::string o(l+2 , r);
                        if(option_names.find(o) != option_names.end()) {
                            std::cerr << "duplicate option: " << o << "\n";
                            ++errors;
                        }
                        option_names[o] = optn;
                        long_opts.push_back({ option_names.find(o)->first.c_str(), has_arg, 0, 0 });
                    } else {
                        // short option
                        std::string shop = std::string(l+1, l+2);
                        if(option_names.find(shop) != option_names.end()) {
                            std::cerr << "duplicate option: " << shop << "\n";
                            ++errors;
                        }
                        option_names[shop] = optn;
                        so += *(l+1);
                    }
                } else if(r != l) {
                    has_arg = required_argument;
                    while(loi < long_opts.size()) long_opts[loi++].has_arg = has_arg;
                }
            }
            if(has_arg == required_argument) for(char c: so) { short_opts += c; short_opts += ':'; }
            else short_opts += so;

        } else {
            new_paragraph = c == e;
        }
    }
    option_values.resize(optn);
/*
    std::cerr << short_opts << "\n";
    for(auto const &lo: long_opts) {
        std::cerr << "{ " 
            << lo.name << ", " 
            << (lo.has_arg? "required_argument": "no_argument") << ", " 
            << (void *) lo.flag << ", " 
            << (char) lo.val << "}\n" ;
    }
*/    
    long_opts.push_back({0, 0, 0, 0});

    char oc; int ox, curind;        
    while(ox=-1, curind=optind, (oc = getopt_long(argc, argv, short_opts.c_str(), &long_opts[0], &ox)) != -1) switch (oc) {
        case ':':
            std::cerr << "Missing argument for option ";
            if(optopt && argv[curind][1] != '-') 
                std::cerr << " -" << (char) optopt << "\n\n";
            else 
                std::cerr << argv[curind] << "\n\n";
            ++errors;
            break;
        case '?':
            std::cerr << "Unrecognized option ";
            if(optopt) 
                std::cerr <<  "-" << (char) optopt << "\n\n";
            else 
                std::cerr << argv[curind] << "\n\n";
            ++errors;
            break;
        default: 
            option_values[option_names[ox >= 0? std::string(long_opts[ox].name): std::string(&oc, &oc+1)]-1].push_back(optarg? optarg: "1");
    }
    argv[optind-1] = argv[0];
    argv+=optind-1;
    argc-=optind-1;
    return errors;
}
std::vector<std::string> const &operator[] (unsigned o) const {
    return option_values[o-1];
}
std::vector<std::string> const &operator[] (std::string const &name) const {
    auto onp = option_names.find(name);
    if(onp == option_names.end()) {
        std::cerr << "Invalid option name \"" << name << "\"\n";
        assert(false);
    }
    return option_values[onp->second-1];
}
bool have(std::string const &name) const {
    return (*this)[name].size() > 0;
}
std::string const &last(std::string const &name) const {
    return (*this)[name].back();
}
std::string const &opt(std::string const &name, std::string const &def="") const {
    return have(name)? last(name): def;
}
int opti(std::string const &name, int def=0) const {
    return have(name)? std::atoi(last(name).c_str()): def;
}
long optul(std::string const &name, long def=0) const {
    return have(name)? std::strtol(last(name).c_str(), nullptr, 0): def;
}
unsigned long optul(std::string const &name, unsigned long def=0) const {
    return have(name)? std::strtoul(last(name).c_str(), nullptr, 0): def;
}
unsigned long long optull(std::string const &name, unsigned long long def=0) const {
    return have(name)? std::strtoull(last(name).c_str(), nullptr, 0): def;
}
long long optull(std::string const &name, long long def=0) const {
    return have(name)? std::strtoll(last(name).c_str(), nullptr, 0): def;
}
float optf(std::string const &name, float def=0) const {
    return have(name)? std::atof(last(name).c_str()): def;
}
double optd(std::string const &name, double def=0) const {
    return have(name)? std::strtod(last(name).c_str(), nullptr): def;
}
bool optb(std::string const &name, bool def=false) const {
    if(!have(name)) return def;
    std::string so(last(name));
    std::transform(so.begin(), so.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(so.c_str()) != 0;
}
};
}
#endif
