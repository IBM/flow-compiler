#ifndef H_ANSI_ESCAPES_H
#define H_ANSI_ESCAPES_H

#include <iosfwd>
#include <string>

enum ANSI_ESCAPE {
    ANSI_NONE = 0,

    ANSI_BOLD = 1,
    ANSI_DIM = 2,
    ANSI_ITALIC = 4,
    ANSI_UNDERLINE = 8,

    ANSI_STRIKE = 16,
    ANSI_REVERSE = 32, 
    ANSI_BLINK = 64,
    ANSI_RESET = 128,

    ANSI_RED = 1*256,
    ANSI_GREEN = 2*256,
    ANSI_YELLOW = 3*256,
    ANSI_BLUE = 4*256,
    ANSI_MAGENTA = 5*256,
    ANSI_CYAN = 6*256,
    ANSI_LGRAY = 7*256,
    ANSI_DGRAY = 8*256,

    ANSI_HRED = 9*256,
    ANSI_HGREEN = 10*256,
    ANSI_HYELLOW = 11*256,
    ANSI_HBLUE = 12*256,
    ANSI_HMAGENTA = 13*256,
    ANSI_HCYAN = 14*256,
    ANSI_WHITE = 15*256,

    ANSI_BG_RED = 1*4096,
    ANSI_BG_GREEN = 2*4096,
    ANSI_BG_YELLOW = 3*4096,
    ANSI_BG_BLUE = 4*4096,
    ANSI_BG_MAGENTA = 5*4096,
    ANSI_BG_CYAN = 6*4096,
    ANSI_BG_LGRAY = 7*4096,
    ANSI_BG_DGRAY = 8*4096,

    ANSI_BG_HRED = 9*4096,
    ANSI_BG_HGREEN = 10*4096,
    ANSI_BG_HYELLOW = 11*4096,
    ANSI_BG_HBLUE = 12*4096,
    ANSI_BG_HMAGENTA = 13*4096,
    ANSI_BG_HCYAN = 14*4096,
    ANSI_BG_WHITE = 15*4096,

    ANSI_COLOR = 15*256,
    ANSI_BG_COLOR = 15*4096,
    ANSI_EMPH = 255,
    ANSI_RGB = uint64_t(256*65536-1) << 16,
    ANSI_BG_RGB = uint64_t(256*65536-1) << (16+24),
};
inline ANSI_ESCAPE operator + (ANSI_ESCAPE e1, ANSI_ESCAPE e2) {
    return ANSI_ESCAPE(uint64_t(e1) | uint64_t(e2));
}

namespace ansi {

std::ostream &toggle(std::ostream &o);
std::ostream &on(std::ostream &o);
std::ostream &off(std::ostream &o);

inline ANSI_ESCAPE rgb(int red, int green, int blue) {
    return ANSI_ESCAPE( uint64_t((red << 16) | (green << 8) | blue) << 16 );
}
inline ANSI_ESCAPE bg_rgb(int red, int green, int blue) {
    return ANSI_ESCAPE( uint64_t((red << 16) | (green << 8) | blue) << (16+24) );
}
std::string escape(ANSI_ESCAPE esc);
/**
 * Replace any first occurence of [begin] with [em] and any second occurence with ANSI_RESET.
 * [em] should be an ANSI escape sequence. ANSI escapes are ignored if use_escapes is false.
 */
std::string emphasize(std::string const &message, ANSI_ESCAPE em=ANSI_BOLD+ANSI_BLUE, std::string const &begin="\"", std::string const &end="\"", bool keep_quotes=false, bool start_at_word_boundary=false);
/**
 * Empahsize text between double quotes with [dquot_em] and text between single quotes with [squot_em]
 * This calls [emphasize] above.
 */
inline static
std::string emphasize2(std::string message, ANSI_ESCAPE dquot_em=ANSI_GREEN+ANSI_BLUE, ANSI_ESCAPE squot_em=ANSI_NONE) {
    return emphasize(emphasize(message, dquot_em, "\"", "\""), squot_em, "'", "'");
}
}
std::ostream &operator << (std::ostream &s, ANSI_ESCAPE esc);
#endif
