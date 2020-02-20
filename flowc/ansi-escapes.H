#ifndef H_ANSI_ESCAPES_H
#define H_ANSI_ESCAPES_H

#include <string>
#include <sstream>

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

namespace ansi {
extern bool use_escapes;

inline static char const *escape(ANSI_ESCAPE esc, bool use_ansi=use_escapes) {
    if(use_ansi) switch(esc) {
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
    return "";
}
inline static std::string escape(ANSI_ESCAPE e1, ANSI_ESCAPE e2) {
    return std::string(escape(e1)) + escape(e2);
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
std::ostream &emphasize(std::ostream &out, std::string const &message, std::string const &em=escape(ANSI_BOLD, ANSI_BLUE), std::string const &begin="\"", std::string const &end="\"", bool keep_quotes=false, bool start_at_word_boundary=false) {
    for(size_t qb = message.find_first_of(begin), 
            qe = qb == std::string::npos? std::string::npos: message.find_first_of(end, qb+1), 
            p = 0; 

            p != std::string::npos;

            p = qe == std::string::npos? std::string::npos: qe+1, 
            qb = p == std::string::npos? std::string::npos: (start_at_word_boundary? find_after_word_boundary(message, begin, p):  message.find_first_of(begin, p)), 
            qe = qb == std::string::npos? std::string::npos: message.find_first_of(end, qb+1)) {

        out << message.substr(p, qb == std::string::npos? message.length(): qb-p);
        if(qb != std::string::npos && qe != std::string::npos) {
            if(use_escapes)
                out << em;
            if(keep_quotes) 
                out << message.substr(qb, qe-qb+1); 
            else
                out << message.substr(qb+1, qe-qb-1);
            if(use_escapes)
                out << escape(ANSI_RESET);
        }
    }
    return out;
}
/**
 * Empahsize text between double quotes with [dquot_em] and text between single quotes with [squot_em]
 * This calls [emphasize] above and observes [use_ansi_escapes].
 */
inline static
std::string emphasize(std::string const &message, std::string const &dquot_em=escape(ANSI_BOLD, ANSI_BLUE), std::string const &squot_em="") {
    std::ostringstream s1, s2;
    if(dquot_em.empty() && squot_em.empty()) 
        return message;
    if(!dquot_em.empty())
        emphasize(s1, message, dquot_em);
    if(squot_em.empty()) return s1.str();
    emphasize(s2, dquot_em.empty()? message: s1.str(), squot_em, "'", "'");
    return s2.str();
}
}
inline static 
std::ostream &operator << (std::ostream &s, ANSI_ESCAPE esc) {
    s << ansi::escape(esc);
    return s;
}
#endif
