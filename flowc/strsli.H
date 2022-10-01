#ifndef H_STRSLI_H
#define H_STRSLI_H

#include <string>
#include <utility>
#include <cstring>


namespace stru1 {

struct cslice: public std::pair<char const *, char const *> {
    cslice(char const *begin=nullptr, char const *end=nullptr):
        std::pair<char const *, char const *>(begin, end==nullptr && begin!=nullptr? begin+strlen(begin): end) {
        }

    int length() const {
        return first == nullptr? 0: second - first;
    }
    std::string get() const {
        return first != nullptr? std::string(first, second): std::string(); 
    }
    cslice &operator += (int i) {
        if(first != nullptr) 
            first += std::min(i, length());
        return *this;
    }
    cslice &operator ++() {
        return *this += 1;
    }
    cslice operator ++(int) {
        cslice prev = *this;
        *this += 1;
        return prev;
    }
    cslice operator +(int rhs) const {
        cslice lhs = *this; 
        return lhs += rhs;
    }
    cslice cut(int begin, int len = -1) const {
        if(begin > length())
            return cslice();
        return cslice(first + begin, len < 0? second: first + std::min(len, length()-begin));
    }
    cslice left(int len) const {
        return cut(0, len);
    }
    cslice right(int len) const {
        return cut(std::max(0, length()-len), std::min(len, length()));
    }
};

struct splitter {
    typedef std::pair<std::string::size_type, std::string::size_type> slice_t;
    std::string const str;
    std::string const separators;
        
    struct iterator {
       slice_t pos;
       splitter const *owner;
       iterator(splitter const *a_owner): pos(std::pair<std::string::size_type, std::string::size_type>(std::string::npos, std::string::npos)), owner(a_owner) {
       }
       iterator(splitter const *a_owner, std::string::size_type begin, std::string::size_type end): pos(std::pair<std::string::size_type, std::string::size_type>(begin, end)), owner(a_owner) {
       }
       iterator &operator++() {
           pos.first = pos.second;
           if(pos.first != std::string::npos) {
               auto p = owner->str.find_first_not_of(owner->separators, pos.first);
               if(p != std::string::npos) {
                   pos.first = p;
                   pos.second = owner->str.find_first_of(owner->separators, pos.first);
                   if(pos.second == std::string::npos)
                       pos.second = owner->str.length();
               } else {
                   pos.first = pos.second = std::string::npos;
               }
           }
           return *this;
       }
       bool operator ==(iterator const &rhs) const {
           return pos == rhs.pos && owner == rhs.owner;
       }
       bool operator !=(iterator const &rhs) const {
           return !(*this == rhs);
       }
       std::string operator *() const {
           if(pos.first != std::string::npos) 
               return owner->str.substr(pos.first, pos.second-pos.first);
           return "";
       }
       /** 
        * Return the rest of the string after the current token and the following
        * separator sequence.
        */
       std::string tail() const {
           auto n = *this;
           ++n;
           if(n.pos.first != std::string::npos)
               return owner->str.substr(n.pos.first);
           return "";
       }
       /** 
        * Return the separator sequence after this token.
        */
       std::string after() const {
           auto n = *this;
           ++n;
           auto end = n.pos.first != std::string::npos?
               n.pos.first: owner->str.length();
           if(pos.second != std::string::npos)
               return owner->str.substr(pos.second, end-pos.second);
           return "";
       }
    };
    iterator begin() const {
        return ++iterator(this, 0, 0);
    }
    iterator end() const {
        return iterator(this);
    }

    splitter(std::string a_str, std::string a_separators): str(a_str), separators(a_separators) {
    }

    std::string first() const {
        return *begin();
    }
    std::string second() const {
        return begin().tail();
    }
};

}

#endif
