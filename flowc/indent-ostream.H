#ifndef H_INDENT_OSTREAM_H
#define H_INDENT_OSTREAM_H

#include <ostream>
#include <string>
#include <cstdio>
#include "filtered-ostream.H"

namespace stru {

/*** Inserter functor for a stream that writes each line indented and wraps at a specified line length.
 * Increase indentation with indent(), ++ operator or stru::indent manipulator.
 * Decrease indentation with unindent(), -- operator or stru::unindent manipulator.
 * The default line wrap length is 120. Set to 0 dsable wrapping.
 *
 */
class indenter_inserter {
private:
    unsigned indent_level;
    unsigned indent_width;
    char fill_char;
    unsigned current_line_width;
    std::string indentation;
    unsigned wrap_length, saved_wrap_length;
    unsigned wrap_indent_width;
    std::string wrap_indentation;
public:
    indenter_inserter(unsigned level = 0, unsigned width=4, unsigned line_wrap = 120, int fill_ch=' '):
        indent_level(level), 
        indent_width(width), 
        fill_char(fill_ch), current_line_width(0), 
        indentation(indent_width*indent_level, fill_char),
        wrap_length(line_wrap), saved_wrap_length(0), 
        wrap_indent_width(line_wrap != 0? indent_width/2+1: 0),
        wrap_indentation(wrap_indent_width, fill_char) {
    }    
    int operator()(std::streambuf &dst, int ch) {
        bool got_error = false;
        bool wrapping = ch == ' ' && wrap_length > 0 && current_line_width > wrap_length;
        if(wrapping)
            ch = '\n';
        
        if(current_line_width == 0 && ch != '\n' && indentation.length() > 0) {
            got_error = dst.sputn(indentation.c_str(), current_line_width = indentation.length()) != indentation.length();
            if(!got_error && wrap_indent_width && wrapping) {
                got_error = dst.sputn(wrap_indentation.c_str(), wrap_indent_width) != wrap_indent_width;
                current_line_width += wrap_indent_width;
            }
        }
        
        current_line_width = ch == '\n'? 0: current_line_width+1;
        return got_error? EOF: dst.sputc(ch);
    }
    /** Set the indentation level to an arbitraty value.
     * Returns the previous indent level.
     */
    unsigned set_level(unsigned new_level) {
        int r = indent_level;
        if(new_level != indent_level) {
            indent_level = new_level;
            indentation = std::string(indent_width*indent_level, fill_char);
        }
        return r;
    }
    unsigned set_width(unsigned new_width) {
        int r = indent_width;
        if(new_width != indent_width) {
            indent_width = new_width;
            indentation = std::string(indent_width*indent_level, fill_char);
        }
        return r;
    }
    /** Set the line wrapping length to an arbitraty value.
     * Returns the previously held value.
     */
    unsigned set_wrap_length(unsigned new_wrap_length) {
        unsigned r = wrap_length;
        wrap_length = new_wrap_length;
        wrap_indentation = std::string(wrap_indent_width, fill_char);
        return r;
    }
    unsigned set_wrap_indent_width(unsigned new_wrap_indent_width) {
        unsigned r = wrap_indent_width;
        wrap_indent_width = new_wrap_indent_width;
        return r;
    }
    bool set_nowrap() {
        bool wrap = wrap_length > 0;
        if(wrap) {
            saved_wrap_length = wrap_length;
            wrap_length = 0;
        }
        return wrap;
    }
    bool set_wrap() {
        if(wrap_length == 0 && saved_wrap_length > 0) {
            wrap_length = saved_wrap_length;
            saved_wrap_length = 0;
        }
        return wrap_length > 0;
    }
    void indent() {
       set_level(indent_level+1);
    }
    void unindent() {
        set_level(indent_level-1);
    }
    auto &operator++ () {
        indent(); return *this;
    }
    auto &operator-- () {
        unindent(); return *this;
    }
};
/******
 * Stream that indents and wraps lines.
 */
class indent_ostream: public filtering_ostream<indenter_inserter> {
public:
    indent_ostream(std::ostream &os, int level = 0, int width = 4, int line_wrap = 120, int fill_ch = ' '):
        filtering_ostream(os, indenter_inserter(level, width, line_wrap, fill_ch)) {
    }
    indent_ostream &operator++ () {
        inserter().indent(); return *this;
    }
    indent_ostream &operator-- () {
        inserter().unindent(); return *this;
    }
    unsigned set_level(unsigned indent_level) {
        return inserter().set_level(indent_level);
    }
    unsigned set_width(unsigned indent_width) {
        flush();
        return inserter().set_width(indent_width);
    }
    unsigned set_wrap_length(unsigned new_wrap_length) {
        flush();
        return inserter().set_wrap_length(new_wrap_length);
    }
    unsigned set_wrap_indent(unsigned wrap_width) {
        flush();
        return inserter().set_wrap_indent_width(wrap_width);
    }
    bool set_wrap() {
        flush();
        return inserter().set_wrap();
    }
    bool set_nowrap() {
        flush();
        return inserter().set_nowrap();
    }
};
inline static std::ostream &wrap(std::ostream &o) {
    auto p = dynamic_cast<indent_ostream *>(&o);
    if(p != nullptr) p->set_wrap();
    return o;
}
inline static std::ostream & nowrap(std::ostream &o) {
    auto p = dynamic_cast<indent_ostream *>(&o);
    if(p != nullptr) p->set_nowrap();
    return o;
}
inline static std::ostream &indent(std::ostream &o) {
    auto p = dynamic_cast<indent_ostream *>(&o);
    if(p != nullptr) ++(*p);
    return o;
}
inline static std::ostream &unindent(std::ostream &o) {
    auto p = dynamic_cast<indent_ostream *>(&o);
    if(p != nullptr) --(*p);
    return o;
}
struct indent_width {
    unsigned width;
    indent_width(unsigned w): width(w) {}
};
struct wrap_length {
    unsigned length;
    wrap_length(unsigned l): length(l) {}
};
struct wrap_indent {
    unsigned width;
    wrap_indent(unsigned w): width(w) {}
};
}
stru::indent_ostream &operator << (stru::indent_ostream &out, stru::indent_width w) {
    out.set_width(w.width); return out;
}
stru::indent_ostream &operator << (stru::indent_ostream &out, stru::wrap_length l) {
    out.set_wrap_length(l.length); return out;
}
stru::indent_ostream &operator << (stru::indent_ostream &out, stru::wrap_indent w) {
    out.set_wrap_indent(w.width); return out;
}
#endif
