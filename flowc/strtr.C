#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>
#include "stru.H"

namespace stru {
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
std::string decamelize(std::string const &s) {
    std::ostringstream out;

    char prev_c = ' ';
    for(unsigned sl = s.length(), i = 0; i < sl; ++i) {
        char c = s[i];
        if(std::isspace(c) || c == '_') {
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
    return std::move(out).str();
}
std::string camelize(std::string const &s, bool capitalize_first) {
    std::ostringstream out;
    bool cap_next = capitalize_first;
    for(char c: s) 
        if(std::isspace(c) || c == '_') {
            cap_next = true;
        } else {
            out << (char) (cap_next? ::toupper(c): c);
            cap_next = false;
        }
    return std::move(out).str();
}
bool isspace(std::string const &s) {
    for(auto c: s)
        if(!std::isspace(c))
            return false;
    return true;
}
std::string remove_empty_lines(std::string const s) {
    std::ostringstream out;
    std::istringstream in(s);
    std::string line;
    while(std::getline(in, line)) {
        if(!isspace(line))
            out << line << "\n";
    }
    return std::move(out).str();
}
static char const *supn[] = {
    "\u2070",
    "\u00B9",
    "\u00B2",
    "\u00B3",
    "\u2074",
    "\u2075",
    "\u2076",
    "\u2077",
    "\u2078",
    "\u2079",
};
std::string sup(std::string const s) {
    std::ostringstream out;
    for(char c: s) 
        switch(c) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                out << supn[c-'0'];
                break;
            case '+':
                out << "\u207A";
                break;
            case '-':
                out << "\u207B";
                break;
            case '=':
                out << "\u207C";
                break;
            case '(':
                out << "\u207D";
                break;
            case ')':
                out << "\u207E";
                break;
            default:
                out << c;
        }
    return std::move(out).str();
}
static char const *subn[] = {
    "\u2080",
    "\u0081",
    "\u0082",
    "\u0083",
    "\u2084",
    "\u2085",
    "\u2086",
    "\u2087",
    "\u2088",
    "\u2089",
};
std::string sub(std::string const s) {
    std::ostringstream out;
    for(char c: s) 
        switch(c) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                out << subn[c-'0'];
                break;
            case '+':
                out << "\u208A";
                break;
            case '-':
                out << "\u208B";
                break;
            case '=':
                out << "\u208C";
                break;
            case '(':
                out << "\u208D";
                break;
            case ')':
                out << "\u208E";
                break;
            default:
                out << c;
        }
    return std::move(out).str();
}
std::string to_upper(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::toupper);
    return u;
}
std::string to_lower(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    return u;
}
bool string_to_bool(std::string const &s) {
    std::string c = strip(s);
    if(c.length() == 0) 
        return false;
    if(c.length() == 1) {
        char t = c[0]; 
        return t == 't' || (t > '0' && t <= '9') || t == 'y' || t == 'T' || t == 'Y';
    }
    c = to_upper(c);
    if(c == "FALSE" || c == "OFF" || c == "NO") 
        return false;
    if(c == "TRUE" || c == "ON" || c == "YES") 
        return true;
    return std::atof(c.c_str()) != 0;
}
std::string to_underscore(std::string const &s) {
    std::string u(s);
    for(auto &c: u) if(c == '-' || c == '.' || c == ' ')  c = '_';
    return u;
}
std::string to_option(std::string const &s, bool lower) {
    std::string u(s);
    if(lower)
        std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    for(auto &c: u) if(c == '_')  c = '-';
    return u;
}
std::string to_cname(std::string const &s) {
    std::string o = to_option(to_identifier(s));
    return o;
}
std::string underscore_to_space(std::string const &s) {
    std::string u(s);
    for(auto &c: u) if(c == '_')  c = ' ';
    return u;
}
std::string to_identifier(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    for(auto &c: u) if(!isalnum(c)) c = '_';
    if(u.length() == 0 || !isalpha(u[0]))
        return std::string("x") + u;
    return u;
}
std::string to_line_comment(std::string const &s, std::string const &comment_prefix) {
    if(s.empty()) return s;
    std::string r(comment_prefix);
    for(auto c: s) switch (c) {
        case '\n': r += "\n"; r += comment_prefix; break;
        default: r += c; 
    }
    return r;
}

}
