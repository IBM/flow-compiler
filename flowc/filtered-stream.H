#ifndef H_FILTERED_STREAM_H
#define H_FILTERED_STREAM_H

#include <ostream>
#include <cstdio>

namespace stru {

class streambuf_inserter {
public:
     int operator()(std::streambuf &dst, int ch) {
         return dst.sputc(ch);
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
}

#endif
