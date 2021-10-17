#include <iostream>
#include "flow-compiler.H"

void flow_compiler::print_pseudocode(std::ostream &out) const {
    int error_count = 0;
    int digits = log10(icode.size())+1;
    int l = 0;
    for(auto const &s: icode) {
        l += 1;
        char lbuf[digits+1];
        snprintf(lbuf, digits+1, "%*d", digits, l);
        out << lbuf << " "  << s << "\n";
    }
}
