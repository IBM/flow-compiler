#ifndef H_LOG_STREAM_H
#define H_LOG_STREAM_H

#include <ostream>
#include <string>
#include <ctime>
#include <sys/time.h>
#include "filtered-stream.H"

namespace stru {
/**** Inserter functor for a stream that prints a timestamp at the beginning of line
 */
class ts_inserter {
private:
    bool at_start_of_line;
    unsigned indent;
    std::string separator;
public:
    ts_inserter(indent_width=1): 
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
/******
 * Stream that preceeds every line with the current time stamp.
 */
class ts_ostream: public filtering_ostream<ts_inserter> {
    indent_ostream(std::ostream &os, int width = 1):
        filtering_ostream(os, ts_inserter(width)) {
    }
    unsigned set_width(unsigned width) {
        return inserter().set_width(width);
    }
};
}
