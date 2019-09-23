#ifndef H_STRU1_H
#define H_STRU1_H

#include <iosfwd>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

extern bool use_ansi_escapes;
enum ANSI_ESCAPE {
    ANSI_BOLD,
    ANSI_RESET,
    ANSI_RED,
    ANSI_MAGENTA,
    ANSI_BLACK,
    ANSI_GREEN,
    ANSI_BLUE,
    ANSI_YELLOW,
    ANSI_CYAN
};

inline static
char const *ansi_escape(ANSI_ESCAPE esc, bool use_ansi=use_ansi_escapes) {
    if(!use_ansi) 
        return "";
    switch(esc) {
        case ANSI_BOLD: return "\x1b[1m";
        case ANSI_RESET: return "\x1b[0m";
        case ANSI_RED: return "\x1b[38;5;1m";
        case ANSI_MAGENTA: return "\x1b[38;5;5m";
        case ANSI_BLACK: return "\x1b[38;5;0m";
        case ANSI_GREEN: return "\x1b[38;5;2m";
        case ANSI_BLUE: return "\x1b[38;5;4m";
        case ANSI_YELLOW: return "\x1b[38;5;3m";
        case ANSI_CYAN: return "\x1b[38;5;6m";
    }
}
inline static
std::string ansi_escape(ANSI_ESCAPE e1, ANSI_ESCAPE e2) {
    return std::string(ansi_escape(e1)) + ansi_escape(e2);
}

inline static 
std::ostream &operator << (std::ostream &s, ANSI_ESCAPE esc) {
    s << ansi_escape(esc);
    return s;
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
std::string strip(std::string const &str, std::string const &strip_chars) {
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
inline static 
std::string::size_type find_after_word_boundary(std::string const &string, std::string const &set, std::string::size_type pos=0) {
    if(pos == std::string::npos) return std::string::npos;
    if(pos != 0) pos = string.find_first_of("\t\r\a\b\v\f\n ", pos);
    if(pos == std::string::npos) return std::string::npos;
    return string.find_first_of(set, pos);
}
/**
 * Replace any first occurence of [begin] with [em] and any second occurence with ANSI_RESET.
 * [em] should be an ANSI escape sequence.
 * The ANSI esacpes are omitted altogether if [use_ansi_escapes] is false
 */
inline static
std::ostream &emphasize(std::ostream &out, std::string const &message, std::string const &em=ansi_escape(ANSI_BOLD, ANSI_BLUE), std::string const &begin="\"", std::string const &end="\"", bool keep_quotes=false, bool start_at_word_boundary=false) {
    for(size_t qb = message.find_first_of(begin), 
            qe = qb == std::string::npos? std::string::npos: message.find_first_of(end, qb+1), 
            p = 0; 

            p != std::string::npos;

            p = qe == std::string::npos? std::string::npos: qe+1, 
            qb = p == std::string::npos? std::string::npos: (start_at_word_boundary? find_after_word_boundary(message, begin, p):  message.find_first_of(begin, p)), 
            qe = qb == std::string::npos? std::string::npos: message.find_first_of(end, qb+1)) {

        out << message.substr(p, qb == std::string::npos? message.length(): qb-p);
        if(qb != std::string::npos && qe != std::string::npos) {
            if(use_ansi_escapes)
                out << em;
            if(keep_quotes) 
                out << message.substr(qb, qe-qb+1); 
            else
                out << message.substr(qb+1, qe-qb-1);
            if(use_ansi_escapes)
                out << ansi_escape(ANSI_RESET);
        }
    }
    return out;
}
/**
 * Empahsize text between double quotes with [dquot_em] and text between single quotes with [squot_em]
 * This calls [emphasize] above and observes [use_ansi_escapes].
 */
inline static
std::string emphasize(std::string const &message, std::string const &dquot_em=ansi_escape(ANSI_BOLD, ANSI_BLUE), std::string const &squot_em="") {
    std::ostringstream s1, s2;
    if(dquot_em.empty() && squot_em.empty()) 
        return message;
    if(!dquot_em.empty())
        emphasize(s1, message, dquot_em);
    if(squot_em.empty()) return s1.str();
    emphasize(s2, dquot_em.empty()? message: s1.str(), squot_em, "'", "'");
    return s2.str();
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
}
#endif
