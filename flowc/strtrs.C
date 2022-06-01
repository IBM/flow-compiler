#include "stru1.H"
#include <iostream>


namespace stru1 {
std::string html_escape(std::string const &s) {
    std::string r;
    for(auto c: s) switch (c) {
        case '\t': 
        case '\r': 
        case '\a': 
        case '\b': 
        case '\v': 
        case '\f': 
            r += "&#"; r += std::to_string((int)c); r += ";";
            break;
        case '\n': r+= "<br/>"; break;
        case '\'':  r+= "&apos;"; break;
        case '"':  r+= "&quot;"; break;
        case '>': r+= "&gt;"; break;
        case '<': r+= "&lt;"; break;
        case '&': r+= "&amp;"; break;
        default: r+= c; break;
    }
    return r;
}
std::string sh_escape(std::string const &s) {
    std::string r("'");
    for(auto c: s) switch (c) {
        case '\'': r+= "'\\''"; break;
        default: r+= c; break;
    }
    return r+"'";
}
std::string c_escape(std::string const &s) {
    std::string r("\"");
    for(auto c: s) switch (c) {
        case '\t': r+= "\\t"; break;
        case '\r': r+= "\\r"; break;
        case '\a': r+= "\\a"; break;
        case '\b': r+= "\\b"; break;
        case '\v': r+= "\\v"; break;
        case '\f': r+= "\\f"; break;
        case '\n': r+= "\\n"; break;
        case '"':  r+= "\\\""; break;
        case '\\': r+= "\\\\"; break;
        default: r+= c; break;
    }
    return r+"\"";
}
std::string json_escape(const std::string &s) {
    std::ostringstream o;
    o << '\"';
    for(auto c: s) {
        switch (c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if('\x00' <= c && c <= '\x1f') {
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c);
            } else {
                o << c;
            }
        }
    }
    o << '\"';
    return o.str();
}
std::string json_unescape(std::string const &s) {
    std::ostringstream o;
    bool esc = false;
    std::string hex;
    int collect_hd = 0;
    unsigned long low_cp = 0;
    for(auto c: s) if(collect_hd) {
        hex += c;
        if(--collect_hd == 0) {
            esc = false;
            auto cp = std::strtoul(hex.c_str(), nullptr, 16);       
            if(low_cp != 0) {
                o << to_unicode(low_cp, cp); low_cp = 0;
            } else if(cp < 0xd800 || cp > 0xdbff) {
                o << to_unicode(cp);
            } else {
                low_cp = cp;
            }
        }
    } else if(esc) {
        esc = false;
        switch(c) {
            case 'b': o << "\b"; break;
            case 'f': o << "\f"; break;
            case 'n': o << "\n"; break;
            case 'r': o << "\r"; break;
            case 't': o << "\t"; break;
            case 'u': collect_hd = 4; break;
            default: o << c; break;
        }
    } else if(c == '\\') {
        esc = true;
    } else {
        o << c;
    }
    return o.str();
}
std::string decamelize(std::string const &s) {
    std::ostringstream out;

    char prev_c = ' ';
    for(unsigned sl = s.length(), i = 0; i < sl; ++i) {
        char c = s[i];
        if(::isspace(c) || c == '_') {
            if(prev_c != ' ') out << ' ';
            prev_c = ' ';
        } else if(::isalpha(c) && ::toupper(c) == c) {
            if((::isalpha(prev_c) && ::toupper(prev_c) != prev_c) ||
               (::isalpha(prev_c) && ::toupper(prev_c) == prev_c && i+1 < sl && ::isalpha(s[i+1]) && ::toupper(s[i+1]) != s[i+1]))
                out << ' ';
            out << (prev_c = c);
        } else {
            out << (prev_c = c);
        }
    }
    return out.str();
}
std::string camelize(std::string const &s, bool capitalize_first) {
    std::ostringstream out;
    bool cap_next = capitalize_first;
    for(char c: s) 
        if(isspace(c) || c == '_') {
            cap_next = true;
        } else {
            out << (char) (cap_next? ::toupper(c): c);
            cap_next = false;
        }
    return out.str();
}
std::string to_unicode(unsigned long cp1, unsigned long cp2) {
    std::string u;
    auto cp = cp1;
    // high surrogate?
    if(cp1 >= 0xd800 && cp1 <= 0xdbff && cp2 >= 0xdc00 && cp2 <= 0xdfff)
        cp = (cp1 << 10) + cp2 - 0x35fdc00;
    else if(cp2 != 0) cp = 0x110000;

    if(cp < 0x80) {
        u += char(cp);
    }
    else if(cp <= 0x7ff) {
        u += char(0xC0 | ((cp >> 6) & 0x1F));
        u += char((0x80 | (cp & 0x3F)));
        return u;
    }
    else if(cp <= 0xffff) {
        u += char(0xE0 | ((cp >> 12) & 0x0F));
        u += char(0x80 | ((cp >> 6) & 0x3F));
        u += char(0x80 | (cp & 0x3F));
    }
    else if(cp <= 0x10ffff) {
        u += char(0xF0 | ((cp >> 18) & 0x07));
        u += char(0x80 | ((cp >> 12) & 0x3F));
        u += char(0x80 | ((cp >> 6) & 0x3F));
        u += char(0x80 | (cp & 0x3F));
    } 
    else {
        // invalid code point
    }
    return u;
}
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
