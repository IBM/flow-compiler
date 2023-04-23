#include "stru.H"
#include <iostream>

namespace stru {
int get_time_value(std::string const &timeout_str) {
    char  *eptr = nullptr;
    auto timeout = std::strtod(timeout_str.c_str(), &eptr);
    if(timeout <= 0) return 0;
    std::string unit;
    if(eptr != nullptr) unit = to_lower(strip(eptr, " \t\r\a\b\f\v\n"));
    if(unit.empty() || unit == "seconds" || unit == "s" || unit == "sec" || unit == "second") return int(timeout * 1000);
    if(unit == "ms" || unit == "millisec" || unit == "millisecond" || unit == "milliseconds") return int(timeout);
    if(unit == "minutes" || unit == "minute" || unit == "min" || unit == "m") return int(timeout * 60000);
    if(unit == "hours" || unit == "hour" || unit == "h") return int(timeout * 3600 * 1000);
    return 0;
}
std::string cevs_transform(std::string const &s) {
    if(s.length() == 0) return "<<\"\"";
    char const *left = "<{";
    char const *right = "}>";
    size_t before = 0; 
    std::ostringstream out;
    for(auto fp = s.find(left), fe = fp; fp != std::string::npos; fp = s.find(left, fe)) {
        fe = s.find(right, fp+1);
        if(fe == std::string::npos) {
            fe = fp+1;
            continue;
        }
        if(fp != before) 
            out << " << " << c_escape(s.substr(before, fp-before)) << "\n";
        
        std::string varr, tr;
        bool have_tr = split(&varr, &tr, s.substr(fp + strlen(left), fe - fp - strlen(left)), "/") == 2;
        out << " << ";
        if(have_tr) {
            if(tr == "s") {
                out << "flowc::json_string(";
            } else if(tr == "b") {
                out << "flowc::stringtobool(";
            } else {
                out << "(";
            }
        }
        out << "flowdef::v" << varr;
        if(have_tr) out << ")";
        out << "\n";

        fe += strlen(right);
        before = fe;
    }
    if(before < s.length())
        out << " << " << json_escape(s.substr(before)) << "\n";
    
    return out.str();
}
}
