#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <zlib.h>
#include <cstdio>
#include <fstream>
#include <utility>
#include "filu.H"

std::string to_identifier(std::string const &s) {
    std::string u(s);
    for(auto &c: u) if(!isalnum(c)) c = '_';
    if(u.length() == 0 || !isalpha(u[0]))
        return std::string("X") + u;
    return u;
}
int main(int argc, char **argv) {
    int rc = 0;
    std::cout << std::showbase;
    unsigned long total_size = 0, total_zize = 0;
    int s = 1;
    if(argc > 1 && strcmp(argv[1], "-g") == 0) 
        ++s; 
    for(int i = s; i < argc; ++i) {
        if(i == 1) std::cout << "#include \"filu.H\"\n";
        std::string fn(argv[i]);
        std::ifstream fin;
        fin.open(argv[i]);
        std::ostringstream cbs;
        long cr = filu::gzip(cbs, fin);
        std::string cb = cbs.str();
        std::string id = to_identifier(fn); 
        if(cr > 0) {
            std::cout << std::dec;
            std::cout << "static size_t " << id << "_len = " << cr << ";\n";
            std::cout << "static size_t " << id << "_zlen = " << cb.length() << ";\n";
            total_size += cr;
            total_zize += cb.length();
            std::cout << "static unsigned char " << id << "_z[] = {\n";
            std::cout << std::hex;
            for(unsigned long b = 0; b < cb.length(); b += 32) {
                for(unsigned long i = b, e = std::min(cb.length(), b + 32); i < e; ++i)
                    std::cout << (int)(unsigned char) cb[i] << ", ";
                std::cout << "\n";
            }
            std::cout << "};\n";
            std::cout << std::dec;
            std::cout << "std::string get_" << id << "() { std::ostringstream buf; std::istringstream zbuf(std::string((char const *)" << id << "_z, " << id << "_zlen)); filu::gunzip(buf, zbuf, "<< (s==2?"true":"false") << "); return buf.str();}\n";
        } else {
            std::cout << "size_t " << id << "_len = 0;\n";
            std::cout << "size_t " << id << "_zlen = 0;\n";
            std::cout << "unsizgned char *" << fn << "_z = nullptr;\n";
            rc = 1;
            break;
        }
    }
    std::cerr << "total " << total_size << " bytes, compressed " << total_zize << ", from " << (argc -1) << " files\n";
    return rc;
}
