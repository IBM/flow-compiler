
#include <iostream>
#include "ansi-escapes.H"
#include "flow-opcodes.H"
#include "grpc-helpers.H"


std::ostream &operator<< (std::ostream &out, fop const &fop) {
    bool use_ansi = ansi::use_escapes && (&out == &std::cerr || &out == &std::cout);

    if(fop.code == MTHD || fop.code == BSTG || fop.code == BPRP) out << ansi::escape(ANSI_BOLD, use_ansi);
    else if(fop.code == INDX) out << ansi::escape(ANSI_YELLOW, use_ansi);

    out << op_name(fop.code) << "  ";
    if(fop.code == MTHD || fop.code == BSTG || fop.code == BPRP || fop.code == INDX) out << ansi::escape(ANSI_RESET, use_ansi);

    if(!fop.arg2.empty()) {
        out << ansi::escape(ANSI_BLUE, use_ansi) << fop.arg1  << ansi::escape(ANSI_RESET, use_ansi) << ", " << ansi::escape(ANSI_BLUE, use_ansi) 
            << fop.arg2 << ansi::escape(ANSI_RESET, use_ansi) << " ";
    } else if(!fop.arg1.empty()) {
        out << ansi::escape(ANSI_BLUE, use_ansi) << fop.arg1 << ansi::escape(ANSI_RESET, use_ansi) << " ";
    }
    if(fop.d2 != nullptr) {
        if(fop.d1 == nullptr) out << "0, ";
        else out << ansi::escape(ANSI_MAGENTA, use_ansi) << "d1: " << fop.d1->full_name() << ansi::escape(ANSI_RESET, use_ansi) << ", ";
        out << ansi::escape(ANSI_MAGENTA, use_ansi) << "d2: " << fop.d2->full_name() << " " << ansi::escape(ANSI_RESET, use_ansi);
    } else if(fop.d1 != nullptr) {
        out << ansi::escape(ANSI_MAGENTA, use_ansi) << "d1: " << fop.d1->full_name() << " " << ansi::escape(ANSI_RESET, use_ansi);
    }
    if(fop.m1 != nullptr) 
        out << ansi::escape(ANSI_GREEN, use_ansi) << "m1: " << fop.m1->full_name() << ansi::escape(ANSI_RESET, use_ansi) << " ";
    bool first = true;
    if((fop.code == LOOP || fop.code == NSET) && fop.arg.size() > 0) 
        out << ansi::escape(ANSI_YELLOW, use_ansi) << "INDX @" << ansi::escape(ANSI_RESET, use_ansi);
    if(fop.code > CON1 && fop.code < CON2) {
        if(fop.arg.size() > 0) 
            out << ansi::escape(ANSI_GREEN, use_ansi) << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[0]) << ansi::escape(ANSI_RESET, use_ansi) << " ";
        if(fop.arg.size() > 1)
            out << "< " << ansi::escape(ANSI_GREEN, use_ansi) << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[1]) << ansi::escape(ANSI_RESET, use_ansi) << " ";
    }
    if(fop.el != nullptr) 
        out << "el: " << ansi::escape(ANSI_RED, use_ansi) << get_full_name(fop.el) << ansi::escape(ANSI_RESET, use_ansi) << " ";
    if(fop.er != nullptr) 
        out << "er: " << ansi::escape(ANSI_RED, use_ansi) << get_full_name(fop.er) << ansi::escape(ANSI_RESET, use_ansi) << " ";
    if(fop.ev1 != nullptr) 
        out << "ev1: " << ansi::escape(ANSI_RED, use_ansi) << fop.ev1->full_name() << ansi::escape(ANSI_RESET, use_ansi) << " ";
    for(auto a: fop.arg) { 
        if(!first) out << ", ";
        out << a; 
        first = false;
    }
    return out;
}
