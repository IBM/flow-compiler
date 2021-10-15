
#include <iostream>
#include "ansi-escapes.H"
#include "flow-opcodes.H"
#include "grpc-helpers.H"

char const *op_name(op o) { 
    switch(o) {
        case NOP:   return "NOP ";
        case LOOP:  return "LOOP";
        case MTHD:  return "MTHD";
        case BNOD:  return "BNOD";
        case IFNC:  return "IFNC";
        case BPRP:  return "BPRP";
        case EPRP:  return "EPRP";
        case BSTG:  return "BSTG";
        case CALL:  return "CALL";
        case COPY:  return "COPY";
        case ELP:   return "ELP ";
        case BLP:   return "BLP ";
        case NLP:   return "NLP ";
        case BNL:   return "BNL ";
        case LPC:   return "LPC ";
        case END:   return "END ";
        case ENOD:  return "ENOD";
        case ESTG:  return "ESTG";

        case ERR:   return "ERR ";

        case SETF: return "SETF";      
        case IOP:  return "IOP";
        case FUNC: return "FUNC";
        case RVF:  return "RVF ";      
        case DACC: return "DACC";           
        case STOL: return "STOL";           
        case CLLS: return "CLLS";           

        case SVF:  return "SVF ";      
        case RVC:  return "RVC ";      
        case RVV:  return "RVV ";      
        case COFI: return "COFI";    
        case COFS: return "COFS";   
        case COFB: return "COFB";  
        case COFF: return "COFF";  
        case COIF: return "COIF"; 
        case COIS: return "COIS";
        case COIB: return "COIB";     
        case COII: return "COII";     
        case COSF: return "COSF";    
        case COSB: return "COSB";    
        case COSI: return "COSI";    
        case COEI: return "COEI";   
        case COEB: return "COEB";   
        case COEF: return "COEF";   
        case COES: return "COES";   
        case COEE: return "COEE";  
        case CON1: return "CON1";
        case CON2: return "CON2";
    }
    return "?";
};

static std::ostream &propc(std::ostream &out, fop const &fop) {
    bool use_ansi = ansi::use_escapes && (&out == &std::cerr || &out == &std::cout);
    switch(fop.code) {
        case BNOD: 
            out << ansi::escape(ANSI_CYAN, use_ansi) << op_name(fop.code) <<  ansi::escape(ANSI_RESET, use_ansi)  << "  ";
            if(fop.arg[0] != 0) 
                out << ansi::escape(ANSI_YELLOW, use_ansi) << fop.arg[0] << "x"<<  ansi::escape(ANSI_RESET, use_ansi)  << "  ";
            if(fop.d1 != nullptr)
                out << ansi::escape(ANSI_MAGENTA, use_ansi) << fop.d1->full_name() << ansi::escape(ANSI_RESET, use_ansi) << ":"  << ansi::escape(ANSI_BLUE, use_ansi) << fop.arg1 << ansi::escape(ANSI_RESET, use_ansi);
            if(fop.d2 != nullptr)
                out << " <- " << ansi::escape(ANSI_MAGENTA, use_ansi) << fop.d2->full_name() << ansi::escape(ANSI_RESET, use_ansi) << ":"  << ansi::escape(ANSI_BLUE, use_ansi) << fop.arg2 << ansi::escape(ANSI_RESET, use_ansi);
            out << ansi::escape(ANSI_YELLOW, use_ansi) << " @" << fop.arg[1] << "/" << fop.arg[2] << ansi::escape(ANSI_RESET, use_ansi);
            if(fop.arg[2] != 0) out << " first";
            if(fop.arg[3] != 0 && fop.arg[4] != 0) out << ", ";
            if(fop.arg[4] != 0) out << " output";
            if(fop.arg[5] != 0) out << " alt " << fop.arg[5];

            break;
        case IFNC:
            out << op_name(fop.code) << "  " << fop.arg1 << ansi::escape(ANSI_YELLOW, use_ansi) << " @" << fop.arg[0] << ansi::escape(ANSI_RESET, use_ansi);;
            if(fop.arg[1])
                out << ansi::escape(ANSI_YELLOW, use_ansi) << " *" << fop.arg[1] << ansi::escape(ANSI_RESET, use_ansi);
            
            for(unsigned a = 2, e = fop.arg.size(); a < e; ++a)
                out << ansi::escape(ANSI_YELLOW, use_ansi) << " !" << fop.arg[a] << ansi::escape(ANSI_RESET, use_ansi);
            break;
        case BSTG: 
            out << ansi::escape(ANSI_BOLD, use_ansi) << op_name(fop.code) <<  ansi::escape(ANSI_RESET, use_ansi)  << "  ";
            out << fop.arg[0] << " (" << fop.arg1 << ") ";
            for(unsigned a = 3, e = fop.arg.size(); a < e; ++a)
                out << ansi::escape(ANSI_YELLOW, use_ansi) << " @" << fop.arg[a] << ansi::escape(ANSI_RESET, use_ansi);

            break;
        case RVF:
            out << op_name(fop.code) << "  ";
            out << ansi::escape(ANSI_BLUE, use_ansi) << fop.arg1 << ansi::escape(ANSI_RESET, use_ansi);
            if(fop.arg.size() > 0 && fop.arg[0] > 0) 
                out << "[" << fop.arg[0] << "] ";
            else 
                out << " ";
            for(unsigned i = 1; i < fop.arg.size(); ++i) { 
                if(i > 1) out << ", ";
                out << fop.arg[i]; 
            }
            break;

        default:
            break;
    }
    return out;
}

std::ostream &operator<< (std::ostream &out, fop const &fop) {
    switch(fop.code) {
        case LOOP:
        case BNOD:
        case IFNC:
        case BSTG:
        case RVF:
            return propc(out, fop);
        default:
            break;
    }
    bool use_ansi = ansi::use_escapes && (&out == &std::cerr || &out == &std::cout);

    if(fop.code == MTHD || fop.code == BPRP) out << ansi::escape(ANSI_BOLD, use_ansi);
    out << op_name(fop.code) << "  ";
    if(fop.code == MTHD || fop.code == BPRP) out << ansi::escape(ANSI_RESET, use_ansi);

    bool escape_string1 = fop.code == RVC && fop.arg.size() > 1 && fop.arg[1] == (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING;
    bool escape_string2 = fop.code == RVC && fop.arg.size() > 2 && fop.arg[2] == (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING;
    if(!fop.arg2.empty()) {
        out << ansi::escape(ANSI_BLUE, use_ansi) << (escape_string1? stru1::c_escape(fop.arg1): fop.arg1)  << ansi::escape(ANSI_RESET, use_ansi) << ", " << ansi::escape(ANSI_BLUE, use_ansi) 
            << (escape_string2? stru1::c_escape(fop.arg2): fop.arg2) << ansi::escape(ANSI_RESET, use_ansi) << " ";
    } else if(!fop.arg1.empty()) {
        out << ansi::escape(ANSI_BLUE, use_ansi) << (escape_string1? stru1::c_escape(fop.arg1): fop.arg1) << ansi::escape(ANSI_RESET, use_ansi) << " ";
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
    if(fop.code > CON1 && fop.code < CON2) {
        if(fop.arg.size() > 0) 
            out << ansi::escape(ANSI_GREEN, use_ansi) << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[0]) << ansi::escape(ANSI_RESET, use_ansi) << " ";
        if(fop.arg.size() > 1)
            out << "<- " << ansi::escape(ANSI_GREEN, use_ansi) << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[1]) << ansi::escape(ANSI_RESET, use_ansi) << " ";
    }
    if(fop.el != nullptr) 
        out << "el: " << ansi::escape(ANSI_RED, use_ansi) << get_full_name(fop.el) << ansi::escape(ANSI_RESET, use_ansi) << " ";
    if(fop.er != nullptr) 
        out << "er: " << ansi::escape(ANSI_RED, use_ansi) << get_full_name(fop.er) << ansi::escape(ANSI_RESET, use_ansi) << " ";
    if(fop.ev1 != nullptr) 
        out << "ev1: " << ansi::escape(ANSI_RED, use_ansi) << fop.ev1->full_name() << ansi::escape(ANSI_RESET, use_ansi) << " ";
    bool first = true;
    for(auto a: fop.arg) { 
        if(!first) out << ", ";
        out << a; 
        first = false;
    }
    return out;
}

