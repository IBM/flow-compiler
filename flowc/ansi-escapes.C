#include <iostream>
#include <sstream>
#include <string>
#include "ansi-escapes.H"

static 
std::string::size_type find_after_word_boundary(std::string const &string, std::string const &set, std::string::size_type pos=0) {
    if(pos == std::string::npos) return std::string::npos;
    auto p = string.find_first_of(set, pos); 
    while(p != std::string::npos) {
        if(p == 0 || strchr("\t\r\a\b\v\f\n ", string[p-1]) != nullptr)
            break;
        p = string.find_first_of(set, p+1);
    }
    return p;
}

namespace ansi {
std::string escape(ANSI_ESCAPE esc) {
    if(esc == ANSI_NONE)
        return "";
    if((int(esc) & int(ANSI_RESET)) != 0)
        return "\x1b[0m";

    std::string e;
    
    if((int(esc) & int(ANSI_BOLD)) != 0)
        e += "\x1b[1m";
    if((int(esc) & int(ANSI_DIM)) != 0)
        e += "\x1b[2m";
    if((int(esc) & int(ANSI_ITALIC)) != 0)
        e += "\x1b[3m";
    if((int(esc) & int(ANSI_UNDERLINE)) != 0)
        e += "\x1b[4m";
    if((int(esc) & int(ANSI_STRIKE)) != 0)
        e += "\x1b[9m";
    if((int(esc) & int(ANSI_REVERSE)) != 0)
        e += "\x1b[7m";
    if((int(esc) & int(ANSI_BLINK)) != 0)
        e += "\x1b[5m";

    auto color = (esc & ANSI_COLOR) / 256;
    if(color != 0) 
        e += std::string("\x1b[38;5;") + char('1'+color-1) + 'm';
    color = (esc & ANSI_BG_COLOR) / 4096;
    if(color != 0) 
        e += std::string("\x1b[48;5;") + char('1'+color-1) + 'm';

    color = (esc & ANSI_RGB) >> 16;
    if(color != 0) 
        e += std::string("\x1b[38;2;") + std::to_string((color >> 16) & 255) + ';' + std::to_string((color >> 8) & 255) + ';' + std::to_string(color & 255) + 'm';
    color = (esc & ANSI_BG_RGB) >> (16+24);
    if(color != 0) 
        e += std::string("\x1b[48;2;") + std::to_string((color >> 16) & 255) + ';' + std::to_string((color >> 8) & 255) + ';' + std::to_string(color & 255) + 'm';
    return e;
}
std::string emphasize(std::string const &message, ANSI_ESCAPE em, std::string const &begin, std::string const &end, bool keep_quotes, bool start_at_word_boundary) {
    std::ostringstream oss;
    for(size_t qb = (start_at_word_boundary? find_after_word_boundary(message, begin): message.find_first_of(begin)),
            qe = (qb == std::string::npos? std::string::npos: message.find_first_of(end, qb+1)),
            p = 0; 

            p != std::string::npos;

            p = qe == std::string::npos? std::string::npos: qe+1, 
            qb = p == std::string::npos? std::string::npos: (start_at_word_boundary? find_after_word_boundary(message, begin, p):  message.find_first_of(begin, p)), 
            qe = qb == std::string::npos? std::string::npos: message.find_first_of(end, qb+1)) {

        oss << message.substr(p, qb == std::string::npos? message.length(): qb-p);
        if(qb != std::string::npos && qe != std::string::npos) {
           
            oss << em;
            if(keep_quotes) 
                oss << message.substr(qb, qe-qb+1); 
            else
                oss << message.substr(qb+1, qe-qb-1);
            if(em != ANSI_NONE)
                oss << ANSI_RESET;
        }
    }
    return oss.str();
}
bool other_enabled = true;
}
std::ostream &operator << (std::ostream &s, ansi::ctl onoff) {
    ansi::other_enabled = onoff == ansi::ctl::toggle? (ansi::other_enabled != true): (onoff==ansi::ctl::on);
    return s;
}
std::ostream &operator << (std::ostream &s, ANSI_ESCAPE esc) {
    if(ansi::other_enabled)
        s << ansi::escape(esc);
    return s;
}
