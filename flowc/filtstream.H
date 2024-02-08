#ifndef H_FILTSTREAM_H
#define H_FILTSTREAM_H

#include <ostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <string>

namespace stru {

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
class indenter_inserter {
private:
    int indent_level;
    int indent_width;
    char fill_char;
    int current_line_width;
    std::string indentation;
    int wrap_length;
public:
    indenter_inserter(int level = 0, int width=4, int line_wrap = 120, int fill_ch=' '):
        indent_level(level >= 0? level: 0), 
        indent_width(width >= 0? width: 0), 
        fill_char(fill_ch), current_line_width(0), 
        indentation(indent_width*indent_level, fill_char),
        wrap_length(line_wrap) {
    }    
    int operator()(std::streambuf &dst, int ch) {
        bool got_error = false;
        if(ch == ' ' && wrap_length > 0 && current_line_width > wrap_length) 
            ch = '\n';
        
        if(current_line_width == 0 && ch != '\n' && indentation.length() > 0) 
            got_error = dst.sputn(indentation.c_str(), current_line_width = indentation.length()) != indentation.length();
        
        current_line_width = ch == '\n'? 0: current_line_width+1;
        return got_error? EOF: dst.sputc(ch);
    }
    void set_level(int new_level) {
        if(new_level < 0) new_level = 0;
        if(new_level != indent_level) {
            indent_level = new_level;
            indentation = std::string(indent_width*indent_level, fill_char);
        }
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
 * Template for a filtering streambuffer that use a functor to filter the content
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
};
}
#endif
