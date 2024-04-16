#ifndef H_SFMT_H
#define H_SFMT_H

#include <sstream>
#include <string>

namespace stru {
class sfmt {
    std::ostringstream os;
public:
    inline operator std::string() {
        //return std::move(os).str();
        return os.str();
    }
    template <class T> sfmt &operator <<(T v) {
        os << v; return *this;
    }
};
}
#endif
