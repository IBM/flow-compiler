#ifndef H_FILTERED_STREAM_H
#define H_FILTERED_STREAM_H

#include <ostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <string>

namespace stru {

class streambuf_inserter {
public:
     int operator()(std::streambuf &dst, int ch) {
         return dst.sputc(ch);
     }
};

/**** Inserter functor for a stream that prints with line count
 */
class lc_inserter {
private:
    bool at_start_of_line;
    int lc;
    int indent;
    std::string separator;
public:
    lc_inserter(int indent_width=0, int line_count=1, std::string sep=" "): 
        at_start_of_line(true), lc(line_count), indent(indent_width), separator(sep) {
    }
    int operator()(std::streambuf &dst, int ch) {
        bool got_error = false;
        if(at_start_of_line && ch != '\n') {
            char buf[256];
            int length = snprintf(buf, sizeof(buf), "%*d%s", indent, lc++, separator.c_str());
            got_error = length > 0 && length < sizeof(buf) && dst.sputn(buf, length) != length;
        }
        at_start_of_line = ch == '\n';
        return got_error? EOF: dst.sputc(ch);
    }
};
/**** Inserter functor for a stream that prints a timestamp at the beginning of line
 */
class ts_inserter {
private:
    bool at_start_of_line;
    int indent;
    std::string separator;
public:
    ts_inserter(int indent_width=1): 
        at_start_of_line(true), indent(indent_width), separator("") {
    }
    ts_inserter(std::string sep): 
        at_start_of_line(true), indent(sep.length()), separator(sep) {
    }
    int operator()(std::streambuf &dst, int ch) {
        bool got_error = false;
        if(at_start_of_line && ch != '\n') {

            timeval tv;
            gettimeofday(&tv, 0);
            struct tm *nowtm = localtime(&tv.tv_sec);
            char tmbuf[64], buf[256]; 
            strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
            static char const *seconds_format = sizeof(tv.tv_sec) == sizeof(long)?  "%s.%03ld%*s": "%s.%03d%*s";  
            int length = snprintf(buf, sizeof(buf), seconds_format, tmbuf, tv.tv_usec/1000, indent, separator.c_str());

            got_error = length > 0 && length < sizeof(buf) && dst.sputn(buf, length) != length;
        }
        at_start_of_line = ch == '\n';
        return got_error? EOF: dst.sputc(ch);
    }
};
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
/***********
 * Template for a filtering streambuffer that uses a functor to filter the content
 * in the overflow call.
 */
template <class INSERTER>
class filtered_ostreambuf: public std::streambuf {
private:
    std::streambuf *sbuf;
    INSERTER filtering_inserter;
public:
    filtered_ostreambuf(std::streambuf *output_buff, INSERTER a_inserter):
        sbuf(output_buff), filtering_inserter(a_inserter) {
    }
    virtual int overflow(int ch) {
        int result = EOF;
        if(ch == EOF) 
            result = sync();
        else if(sbuf != nullptr)
            result = filtering_inserter(*sbuf , ch);
        return result;
    }
    INSERTER &inserter() {
        return filtering_inserter;
    }
};
/******* 
 * Class containing an object that can be used to insert a member object into a derived class.
 * The purpose is to be able to initalize the member before any other base classes that might 
 * need to use this memner in their constructor.
 */
template<class OBJ>
struct object_container {
    OBJ pbobj;
    object_container(OBJ obj): pbobj(obj) {
    }
};
/******
 * Template for an ostream that uses a filtering streambuffer. The argument is the functor
 * passed to the streambuffer.
 */
template <class T>
class filtering_ostream: public object_container<filtered_ostreambuf<T>>, public std::ostream {
public:
    filtering_ostream(std::ostream &os, T inserter):
       object_container<filtered_ostreambuf<T>>(filtered_ostreambuf<T>(os.rdbuf(), inserter)),
       std::ostream(&(object_container<filtered_ostreambuf<T>>::pbobj)) {
    }
    T &inserter() {
        return object_container<filtered_ostreambuf<T>>::pbobj.inserter();
    }
};
/******
 * Stream that preceeds every line with the current time stamp.
 */
typedef filtering_ostream<ts_inserter> ts_ostream;
/******
 * Stream that preceeds every line with a line number
 */
typedef filtering_ostream<ts_inserter> lc_ostream;
/******
 * Stream that indents and wraps lines.
 */
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
inline static void wrap(indent_ostream &o) {
    o.set_wrap();
}
inline static void nowrap(indent_ostream &o) {
    o.set_nowrap();
}
inline static void indent(indent_ostream &o) {
    ++o;
}
inline static void unindent(indent_ostream &o) {
    --o;
}
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
template <class T>
inline static 
stru::indent_ostream &operator<<(stru::indent_ostream &out, T t) {
    std::ostream &o = out;
    o << t;
    return out;
}

#endif
