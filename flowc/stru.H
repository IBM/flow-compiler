#ifndef H_STRU_H
#define H_STRU_H

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>

namespace stru {
inline static 
std::string reps(std::string const &rep, unsigned count) {
    std::stringstream buf;
    for(; count != 0; --count) buf << rep;
    return buf.str();
}
inline static
bool starts_with(std::string const &str, std::string const &with, std::string *rest = nullptr) {
    if(with.length() > str.length() || str.substr(0, with.length()) != with)
        return false;
    if(rest != nullptr) *rest = str.substr(with.length());
    return true;
}
inline static
bool ends_with(std::string const &str, std::string const &with, std::string *prefix = nullptr) {
    if(with.length() > str.length() || str.substr(str.length()-with.length()) != with)
        return false;
    if(prefix != nullptr) *prefix = str.substr(0, str.length()-with.length());
    return true;
}
inline static
std::string strip1(std::string const &str, std::string const &strip_left, std::string const &strip_right) {
    if(str.length() == 0) return str;
    std::string s = str;
    if(s.length() > 0 && strip_left.length() > 0 && strip_left.find(s[0]) != std::string::npos)
        s = s.substr(1);
    if(s.length() > 0 && strip_right.length() > 0 && strip_right.find(s[s.length()-1]) != std::string::npos)
        s = s.substr(0, s.length()-1);
    return s;
}
inline static
std::string strip1(std::string const &str, std::string const &strip_chars="\t\r\a\b\v\f\n ") {
    return strip1(str, strip_chars, strip_chars);
}
inline static
std::string strip(std::string const &str, std::string const &strip_chars="\t\r\a\b\v\f\n ") {
    auto b = str.find_first_not_of(strip_chars);
    if(b == std::string::npos) b = 0;
    auto e = str.find_last_not_of(strip_chars);
    if(e == std::string::npos) return "";
    return str.substr(b, e-b+1);
}
inline static
std::string lstrip(int *columnp, std::string const &str, std::string const &strip_chars="\t\r\a\b\v\f\n ") {
    auto b = str.find_first_not_of(strip_chars);
    int c; int &column = columnp == nullptr? c: *columnp;
    if(b == std::string::npos) {
        column = int(str.length());
        return "";
    }
    column = int(b);
    return str.substr(b);
}
inline static
std::string rstrip(int *columnp, std::string const &str, std::string const &strip_chars="\t\r\a\b\v\f\n ") {
    auto b = str.find_last_not_of(strip_chars);
    int c; int &column = columnp == nullptr? c: *columnp;
    if(b == std::string::npos) {
        column = int(str.length());
        return "";
    }
    column = int(b+1);
    return str.substr(0, b+1);
}
inline static
int split(std::string *left, std::string *right, std::string const &str, std::string const &sep) {
    auto b = str.find_first_of(sep);
    if(b == std::string::npos) {
        if(left) *left = str; 
        if(right) *right = "";
        return 1;
    } else {
        std::string tmp(str);
        if(left) *left = tmp.substr(0, b); 
        if(right) *right = tmp.substr(b+1);
        return 2;
    }
}
inline static 
std::string remove_suffix(std::string const &s, std::string const &suffix) {
    if(suffix.empty() || suffix.length() > s.length() || 
            s.substr(s.length()-suffix.length()) != suffix)
        return s;
    return s.substr(0, s.length()-suffix.length());
}
inline static
std::vector<std::string> const &split(std::vector<std::string> &buf, std::string const &str, std::string const &separators) {
    for(auto b = str.find_first_not_of(separators), e = (b == std::string::npos? b: str.find_first_of(separators, b));
        b != std::string::npos; 
        b = (e == std::string::npos? e: str.find_first_not_of(separators, e)),
        e = (b == std::string::npos? b: str.find_first_of(separators, b))) 
        buf.push_back(e == std::string::npos? str.substr(b): str.substr(b, e-b));
    return buf;
}
std::string remove_empty_lines(std::string s);
bool isspace(std::string const &s);

inline static
std::string to_upper(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::toupper);
    return u;
}
inline static 
bool string_to_bool(std::string const &s) {
    std::string c = strip(s);
    if(c.length() == 0) return false;
    if(c.length() == 1) {
        char t = c[0]; 
        return t == 't' || (t > '0' && t <= '9') || t == 'y' || t == 'T' || t == 'Y';
    }
    c = to_upper(c);
    if(c == "TRUE" || c == "ON" || c == "YES") return true;
    return std::atof(c.c_str()) != 0;
}
inline static
std::string to_lower(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    return u;
}
inline static
std::string to_option(std::string const &s, bool lower=true) {
    std::string u(s);
    if(lower)
        std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    for(auto &c: u) if(c == '_')  c = '-';
    return u;
}
inline static
std::string to_cname(std::string const &s, bool lower=true) {
    return to_option(s, lower);
}
inline static
std::string to_underscore(std::string const &s) {
    std::string u(s);
    for(auto &c: u) if(c == '-' || c == '.')  c = '_';
    return u;
}
inline static
std::string underscore_to_space(std::string const &s) {
    std::string u(s);
    for(auto &c: u) if(c == '_')  c = ' ';
    return u;
}
inline static
std::string to_identifier(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    for(auto &c: u) if(!isalnum(c)) c = '_';
    if(u.length() == 0 || !isalpha(u[0]))
        return std::string("X") + u;
    return u;
}
inline static 
std::string to_line_comment(std::string const &s, std::string const &comment_prefix) {
    if(s.empty()) return s;
    std::string r(comment_prefix);
    for(auto c: s) switch (c) {
        case '\n': r += "\n"; r += comment_prefix; break;
        default: r += c; 
    }
    return r;
}
std::string base64(std::string const &s);
std::string base64d(std::string const &s);
std::string sh_escape(std::string const &s);
std::string html_escape(std::string const &s);
std::string c_escape(std::string const &s);
std::string json_escape(const std::string &s);
std::string json_unescape(std::string const &s);
std::string to_unicode(unsigned long cp1, unsigned long cp2 = 0);

template <class CP> // assumes a correctly formed json
inline static std::string scan_json(std::pair<CP, CP> &x) {
    std::string tok;
    CP &p = x.first; CP &e = x.second;
    while(p != e && std::isspace(*p)) 
        ++p;
    if(p == e) // end of stream
        return tok;
    switch(*p) {
        case '{': case '}': case '[': case ']': case ':': case ',': 
            tok = *p++; 
            break;
        case '"': 
            tok = *p++;
            while(p != e) {
                if(*p == '"') break;
                if(*p == '\\') tok += *p++;
                if(p == e) break;
                tok += *p++;
            }
            if(p != e) 
                tok += *p++;
            break;
        default:
            while(p != e && (std::isalnum(*p) || *p == '.' || *p == '-' || *p == '+')) tok += *p++;
            break;
    }
    return tok;
}

/**
 * Merge a list of E1 .. En into a string:
 * The result is: j_begin+P1+j1+...+Pn-1+j_last+Pn+j_end
 * Where each Pi is prefix+Ei+suffix
 */
template <class ITER> inline static
std::string join(ITER begin, ITER end, std::string const &j1, std::string const &j_last="", std::string const &j_begin="", std::string const &prefix="", std::string const &suffix="", std::string const &j_end="") {
    std::ostringstream r;
    for(auto p = begin; p != end; ++p) { 
        auto np = p; ++np;
        if(p == begin) r << j_begin;
        else if(np == end && !j_last.empty()) r << j_last;
        else r << j1;
        r << prefix << *p << suffix;
        if(np == end) r << j_end;
    }
    return r.str();
}
template <class ITERABLE> inline static
std::string join(ITERABLE buf, std::string const &j1, std::string const &j_last="", std::string const &j_begin="",std::string const &prefix="", std::string const &suffix="", std::string const &j_end="") {
    return join(buf.begin(), buf.end(), j1, j_last, j_begin, prefix, suffix, j_end);
}
template <class ITER, class UNOP> inline static
std::string joint(ITER begin, ITER end, UNOP unary_op, std::string const &j1, std::string const &j_last="") {
    std::ostringstream r;
    for(auto p = begin; p != end; ++p) { 
        auto np = p; ++np;
        if(p == begin) r << "";
        else if(np == end && !j_last.empty()) r << j_last;
        else r << j1;
        r << unary_op(*p);
    }
    return r.str();
}
template <class ITERABLE, class UNOP> inline static
std::string joint(ITERABLE buf, UNOP unary_op, std::string const &j1, std::string const &j_last="") {
    return joint(buf.begin(), buf.end(), unary_op, j1, j_last);
}
/**
 * Parse the string [timeout_str] and return the time in milliseconds.
 * Return 0 on parse error or if the time is negative. 
 */
int get_time_value(std::string const &timeout_str);
/**
 * Replace all underscore runs with a space and insert a space before any camel hump.
 * Keep uppercase runs.
 * e.g.
 *          foo_bar -> foo bar
 *          fooBar  -> foo Bar
 *          FOOBar  -> FOO Bar
 *          FOO_BAR -> FOO BAR
 */
std::string decamelize(std::string const &s);
/**
 * Capitalize any letter preceeded by space.
 */
std::string camelize(std::string const &s, bool capitalize_first=false);
}
#endif
