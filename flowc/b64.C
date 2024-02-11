#include <string>
#include <sstream>

static char const *b64encode_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char const b64decode_table[256] = { 
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  
0,  0,  1,  2,  3,  4,  5,  6, 7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0, 0,  0,  0, 63,  0, 
26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
namespace stru {
std::string base64(std::string const &s) {
    std::ostringstream out;
    
    for(unsigned i = 0, e = s.length(); i + 2 < e; i += 3) {

        uint32_t a = (unsigned char)s[i];
        uint32_t b = (unsigned char)s[i+1];
        uint32_t c = (unsigned char)s[i+2];

        uint32_t abc = (a << 16) + (b << 8) + c;

        out << b64encode_table[(abc >> 3 * 6) & 0x3F]
            << b64encode_table[(abc >> 2 * 6) & 0x3F]
            << b64encode_table[(abc >> 1 * 6) & 0x3F]
            << b64encode_table[(abc >> 0 * 6) & 0x3F];
    }
    unsigned e = s.length() / 3 * 3;
    if(e < s.length()) {
        out << b64encode_table[s[e] >> 2];
        if(s.length() - e  == 1) 
            out << b64encode_table[(s[e] & 3) << 4]
                << '=';
        
        else 
            out << b64encode_table[((s[e] & 3) << 4) | (s[e+1] >> 4)]
                << b64encode_table[(s[e+1] & 0xF) << 2];
        out << '=';
    }
    return std::move(out).str();
}
std::string base64d(std::string const &s) {
    if(s.empty()) 
        return std::string();

    std::string buffer;
    bool pad = s.length() % 4 != 0 || s.back() == '=';
    unsigned e = ((s.length() + 3) / 4 - (pad?1:0)) * 4;
    for(unsigned i = 0, e = s.length(); i < e; i += 4) {
        unsigned abc = b64decode_table[(unsigned char) s[i]] << 18 | 
            b64decode_table[(unsigned char)s[i+1]] << 12 | 
            b64decode_table[(unsigned char)s[i+2]] << 6 | 
            b64decode_table[(unsigned char)s[i+3]];
        buffer += char((abc >> 16) & 0xFF);
        buffer += char((abc >> 8) & 0xFF);
        buffer += char(abc & 0xFF);
    }
    if(pad) {
        unsigned a = b64decode_table[(unsigned char)s[e]] << 18 | 
            b64decode_table[(unsigned char)s[e+1]] << 12;
        buffer += char((a >> 16) & 0xFF);
        if(s.length() > e+2 && s[e+2] != '=') {
            a = b64decode_table[(unsigned char)s[e+2]] << 6;
            buffer += char((a >> 8) & 0xFF);
        }
    }
    return buffer;
}
} 
