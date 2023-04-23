#ifndef H_MASSERT_H
#define H_MASSERT_H

#include <iostream>
#include "stru.H"

struct quit {
    std::ostream &out;
    int rc;
    quit(int c = 1): out(std::cerr), rc(c) {
    }
    quit &operator <<= (std::string const &msg) {
        out << msg;
        abort();
    }
};

#define MASSERT(c) if(!(c)) quit() <<= stru::sfmt() << __FILE__ << ":" << __LINE__ << " "

#endif
