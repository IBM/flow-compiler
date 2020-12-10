#ifndef H_STRU1_H
#define H_STRU1_H

#include <iosfwd>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>

template <class A, class B> inline static 
std::ostream &operator << (std::ostream &s, std::map<A, B> const &m);
template <class A> inline static 
std::ostream &operator << (std::ostream &s, std::vector<A> const &m);
template <class A> inline static 
std::ostream &operator << (std::ostream &s, std::set<A> const &m);
template <class A, class B> inline static 
std::ostream &operator << (std::ostream &s, std::pair<A, B> const &p) {
    return s << p.first << ": "<< p.second;
}
template <class A, class B> inline static 
std::ostream &operator << (std::ostream &s, std::map<A, B> const &m) {
    s << "{";
    int c = 0;
    for(auto const &p: m) {
        if(++c > 1) s << ", ";
        s << p;
    }
    return s << "}";
}
template <class A> inline static 
std::ostream &operator << (std::ostream &s, std::vector<A> const &m) {
    s << "[";
    int c = 0;
    for(auto const &p: m) {
        if(++c > 1) s << ", ";
        s << p;
    }
    return s << "]";
}
template <class A> inline static 
std::ostream &operator << (std::ostream &s, std::set<A> const &m) {
    s << "{";
    int c = 0;
    for(auto const &p: m) {
        if(++c > 1) s << ", ";
        s << p;
    }
    return s << "}";
}

namespace stru1 {
class sfmt {
    std::ostringstream os;
public:
    inline operator std::string() {
        return os.str();
    }
    template <class T> sfmt &operator <<(T v) {
        os << v; return *this;
    }
};
// Indentation manipulators
struct indent {
    bool add;
    indent(bool doit = true): add(doit) {}
};
struct unindent {
    bool add;
    unindent(): add(false) {}
};

class indented_stream {
    int indent_value;
    int indent_level;
    std::ostream &os;
    bool add_indentation;
public:    
    indented_stream(std::ostream &a_os, int initial_indent=1, int a_indent_value=4):
        indent_value(a_indent_value > 0? a_indent_value: 0), indent_level(initial_indent), os(a_os), add_indentation(initial_indent > 0) {
        }

    template <class T> inline indented_stream &operator << (T t);
    inline indented_stream &operator++ () {
        ++indent_level; return *this;
    }
    inline indented_stream &operator-- () {
        --indent_level; return *this;
    }
    void indent() {
        if(add_indentation) {
            if(indent_level > 0) os << std::string(indent_value * indent_level, ' ');
            add_indentation = false;
        }

    }
};
template <> inline indented_stream &indented_stream::operator << (std::string const &t){
    for(auto c: t) {
        indent();
        os << c; 
        if(c == '\n') add_indentation = true;
    }
    return *this;
}
template <> inline indented_stream &indented_stream::operator << (char const *t) {
    if(t == nullptr) return *this;
    for(; *t != '\0'; ++t) {
        indent();
        os << *t; 
        if(*t == '\n') add_indentation = true;
    }
    return *this;
}
template <> inline indented_stream &indented_stream::operator << (struct indent ind) {
    return ind.add? ++(*this): --(*this);
    return *this;
}
template <> inline indented_stream &indented_stream::operator << (struct unindent ind) {
    return --(*this);
}
template <class T> indented_stream &indented_stream::operator << (T t) {
    indent();
    os << t; 
    return *this;
}
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
std::string strip(std::string const &str, std::string const &strip_chars="\t\r\a\b\v\f\n ") {
    auto b = str.find_first_not_of(strip_chars);
    if(b == std::string::npos) b = 0;
    auto e = str.find_last_not_of(strip_chars);
    if(e == std::string::npos) return "";
    return str.substr(b, e-b+1);
}
inline static
std::string lstrip(int &column, std::string const &str, std::string const &strip_chars) {
    auto b = str.find_first_not_of(strip_chars);
    if(b == std::string::npos) {
        column = int(str.length());
        return "";
    }
    column = int(b);
    return str.substr(b);
}
inline static
std::string rstrip(int &column, std::string const &str, std::string const &strip_chars) {
    auto b = str.find_last_not_of(strip_chars);
    if(b == std::string::npos) {
        column = int(str.length());
        return str;
    }
    column = int(b);
    return str.substr(0, b);
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
std::string basename(std::string const &filename, std::string const &suffix = "", std::string *dirname=nullptr) {
    auto lsl = filename.find_last_of("/");
    if(dirname != nullptr) 
        *dirname = lsl == std::string::npos? std::string(): filename.substr(0, lsl);
    std::string f(lsl == std::string::npos? filename: filename.substr(lsl+1));
    return suffix.empty()? f: remove_suffix(f, suffix);
}
inline static 
std::string dirname(std::string const &filename) {
    std::string dn;
    basename(filename, "", &dn);
    return dn;
}
inline static 
std::string path_join(std::string const &base_path, std::string const &path) {
    if(path.empty()) return base_path;
    if(base_path.empty() || path[0] == '/') return path;
    return *base_path.rbegin() == '/'? base_path + path: base_path + '/' + path;
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
std::string to_option(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    for(auto &c: u) if(c == '_')  c = '-';
    return u;
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
inline static 
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
inline static 
std::string json_escape(const std::string &s) {
    std::ostringstream o;
    o << '\"';
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    o << '\"';
    return o.str();
}
inline static
std::ostream &to_json(std::ostream &out, std::string const *x) { return out << json_escape(*x); }
inline static
std::ostream &to_json(std::ostream &out, std::string const &x) { return out << json_escape(x); }
inline static
std::ostream &to_json(std::ostream &out, char const *x) { return out << json_escape(x); }
template <class T> inline static 
std::ostream &to_json(std::ostream &out, T x) { return out << x; }
template <class E> 
std::ostream &to_json(std::ostream &out, std::vector<E> const &v) { 
    out << "[";
    if(v.size() > 0) to_json(out, v[0]);
    for(unsigned i = 1, e = v.size(); i < e; ++i) {
        out << ","; to_json(out, v[i]);
    }
    out << "]";
    return out;
}
template <class V> 
std::ostream &to_json(std::ostream &out, std::map<std::string, V> const &v) { 
    out << "{";
    char const *sep = "";
    for(auto const &p: v) {
        out << sep; to_json(out, p.first); out << ":"; to_json(out, p.second); sep = ",";
    }
    out << "}";
    return out;
}
inline static 
std::string sh_escape(std::string const &s) {
    std::string r("'");
    for(auto c: s) switch (c) {
        case '\'': r+= "'\\''"; break;
        default: r+= c; break;
    }
    return r+"'";
}
inline static 
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
template <class CONTAINER> inline static
std::string join(CONTAINER const &buf, std::string const &j1, std::string const &j_last="", std::string const &j_begin="",std::string const &prefix="", std::string const &suffix="", std::string const &j_end="") {
    return join(buf.begin(), buf.end(), j1, j_last, j_begin, prefix, suffix, j_end);
}
inline static
std::string search_path(std::string const &bin) {
    struct stat  st;
    char const *pathv = std::getenv("PATH");
    if(pathv == nullptr) 
        return bin;
    std::vector<std::string> buf;
    for(auto const &path: split(buf, pathv, ":")) {
        std::string file(path+"/"+bin);
        if(stat(file.c_str(), &st) >= 0 && (st.st_mode & S_IEXEC) != 0)
            return file;
    }
    return bin;
}
template <class CONT, class SET> inline static
bool disjunct(CONT const &c, SET const &s) {
    for(auto const &v: s) if(c.find(v) != c.end()) return false;
    return true;
}
template <class CONT, class SET> inline static
bool includes(CONT const &c, SET const &s) {
    for(auto const &v: s) if(c.find(v) == c.end()) return false;
    return true;
}
template <class CONT, class VALUE> inline static
bool contains(CONT const &c, VALUE const &v) {
    return c.find(v) != c.end();
}
template <class DEST, class CONT, class SET> inline static
bool difference(DEST &d, CONT const &a, SET const &b) {
    for(auto const &v: a) if(b.find(v) == b.end()) d.insert(v);
    return d.size() > 0;
}
/**
 * Parse the string [timeout_str] and return the time in milliseconds.
 * Return 0 on parse error or if the time is negative. 
 */
static inline
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
static inline 
/**
 * Replace all underscore runs with a space and insert a space before any camel hump.
 * Keep uppercase runs.
 * e.g.
 *          foo_bar -> foo bar
 *          fooBar  -> foo Bar
 *          FOOBar  -> FOO Bar
 *          FOO_BAR -> FOO BAR
 */
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
static inline
/**
 * Capitalize and letter preceeded by space.
 */
std::string camelize(std::string const &s, bool capitalize_first=false) {
    std::ostringstream out;
    bool cap_next = capitalize_first;
    for(char c: s) 
        if(isspace(c)) {
            cap_next = true;
        } else {
            out << (cap_next? ::toupper(c): c);
            cap_next = false;
        }
    return out.str();
}
}
#endif
