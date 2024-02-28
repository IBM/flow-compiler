#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace stru {
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
    return std::move(o).str();
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
                o << to_unicode(cp, 0);
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
    return std::move(o).str();
}
}
