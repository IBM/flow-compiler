#include <sstream>
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
    std::string ss = 
        (s.length() >= 2 && *s.begin() == '"' && *s.rbegin() == '"')?
            s.substr(1, s.length()-2): s;
    bool esc = false;
    std::string hex;
    int collect_hd = 0;
    unsigned long low_cp = 0;
    for(auto c: ss) if(collect_hd) {
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
            default: o << '\\' << c; break;
        }
    } else if(c == '\\') {
        esc = true;
    } else {
        o << c;
    }
    return o.str();
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
    return out.str();
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
    return out.str();
}
std::string remove_empty_lines(std::string s) {
    std::ostringstream out;
    std::istringstream in(s);
    std::string line;
    while(std::getline(in, line)) {
        if(!isspace(line))
            out << line << "\n";
    }
    return out.str();
}
static char const *b64encode_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64(std::string const &s) {
    std::ostringstream out;
    
    for(unsigned i = 0, e = s.length(); i + 2 < e; i += 3) {

        uint32_t a = (unsigned char)s[i];
        uint32_t b = (unsigned char)s[i+1];
        uint32_t c = (unsigned char)s[i+2];

        uint32_t abc = (a << 16) + (b << 8) + c;

        out << b64encode_table[(abc >> 3 * 6) & 0x3F]
            << b64encode_table[(abc >> 2 * 6) & 0x3F]
            << b64encode_table[(abc >> 1 * 6) & 0x3F]
            << b64encode_table[(abc >> 0 * 6) & 0x3F];
    }
    unsigned e = s.length() / 3 * 3;
    if(e < s.length()) {
        out << b64encode_table[s[e] >> 2];
        if(s.length() - e  == 1) 
            out << b64encode_table[(s[e] & 3) << 4]
                << '=';
        
        else 
            out << b64encode_table[((s[e] & 3) << 4) | (s[e+1] >> 4)]
                << b64encode_table[(s[e+1] & 0xF) << 2];
        out << '=';
    }
    return out.str();
}
static unsigned char const b64decode_table[256] = { 
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  
0,  0,  1,  2,  3,  4,  5,  6, 7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0, 0,  0,  0, 63,  0, 
26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
std::string base64d(std::string const &s) {
    if(s.empty()) 
        return std::string();

    std::string buffer;
    bool pad = s.length() % 4 != 0 || s.back() == '=';
    unsigned e = ((s.length() + 3) / 4 - (pad?1:0)) * 4;
    for(unsigned i = 0, e = s.length(); i < e; i += 4) {
        unsigned abc = b64decode_table[(unsigned char) s[i]] << 18 | 
            b64decode_table[(unsigned char)s[i+1]] << 12 | 
            b64decode_table[(unsigned char)s[i+2]] << 6 | 
            b64decode_table[(unsigned char)s[i+3]];
        buffer += char((abc >> 16) & 0xFF);
        buffer += char((abc >> 8) & 0xFF);
        buffer += char(abc & 0xFF);
    }
    if(pad) {
        unsigned a = b64decode_table[(unsigned char)s[e]] << 18 | 
            b64decode_table[(unsigned char)s[e+1]] << 12;
        buffer += char((a >> 16) & 0xFF);
        if(s.length() > e+2 && s[e+2] != '=') {
            a = b64decode_table[(unsigned char)s[e+2]] << 6;
            buffer += char((a >> 8) & 0xFF);
        }
    }
    return buffer;
}
}
