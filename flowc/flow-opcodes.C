#include <iostream>
#include <unistd.h>

#include "ansi-escapes.H"
#include "flow-opcodes.H"
#include "grpc-helpers.H"
#include "helpo.H"

extern helpo::opts opts;

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
        case BERC:  return "BERC";
        case EERC:  return "EERC";

        case SETF: return "SETF";      
        case SETE: return "SETE";
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
        case FLAT: return "FLAT";
    }
    return "?";
};

static std::ostream &propc(std::ostream &out, fop const &fop) {
    bool use_ansi = opts.optb("color", &out == &std::cerr && isatty(fileno(stderr)) || &out == &std::cout && isatty(fileno(stdout)));
    switch(fop.code) {
        case BNOD: 
            out << ANSI_CYAN+ANSI_BOLD << op_name(fop.code) << ANSI_RESET  << "  ";
            if(fop.arg[0] != 0) 
                out << ANSI_YELLOW << fop.arg[0] << "x" <<  ANSI_RESET  << "  ";
            if(fop.d1 != nullptr)
                out << ANSI_MAGENTA << fop.d1->full_name() << ANSI_RESET << ":"  << ANSI_BLUE << fop.arg1 << ANSI_RESET;
            if(fop.d2 != nullptr)
                out << " <- " << ANSI_MAGENTA << fop.d2->full_name() << ANSI_RESET << ":"  << ANSI_BLUE << fop.arg2 << ANSI_RESET;
            out << ANSI_YELLOW << " @" << fop.arg[1] << "/" << fop.arg[2] << ANSI_RESET;
            if(fop.arg[3] != 0) out << " first";
            if(fop.arg[3] != 0 && fop.arg[4] != 0) out << ", ";
            if(fop.arg[4] != 0) out << " output";
            if(fop.arg[5] != 0) out << " alt " << fop.arg[5];
            break;
        case BERC: 
            out << ANSI_CYAN << ANSI_BOLD << op_name(fop.code) <<  ANSI_RESET  << "  ";
            if(fop.arg[0] != 0) 
                out << ANSI_YELLOW << fop.arg[0] << "x"<<  ANSI_RESET  << "  ";
            
            out << ANSI_YELLOW << " @" << fop.arg[1] << "/" << fop.arg[2] << ANSI_RESET;
            out << ", " << fop.arg[3];
            break;
        case IFNC:
            out << op_name(fop.code) << "  " << fop.arg1 << ANSI_YELLOW << " @" << fop.arg[0] << ANSI_RESET;;
            if(fop.arg[1])
                out << ANSI_YELLOW << " *" << fop.arg[1] << ANSI_RESET;
            
            for(unsigned a = 2, e = fop.arg.size(); a < e; ++a)
                out << ANSI_YELLOW << " !" << fop.arg[a] << ANSI_RESET;
            break;
        case BSTG: 
            out << ANSI_BOLD << op_name(fop.code) <<  ANSI_RESET  << "  ";
            out << fop.arg[0] << " (" << fop.arg1 << ") ";
            for(unsigned a = 3, e = fop.arg.size(); a < e; ++a)
                out << ANSI_YELLOW << " @" << fop.arg[a] << ANSI_RESET;

            break;
        case RVF:
            out << op_name(fop.code) << "  ";
            out << ANSI_BLUE << fop.arg1 << ANSI_RESET;
            if(fop.arg.size() > 0 && fop.arg[0] > 0) 
                out << "[" << fop.arg[0] << "] ";
            else 
                out << " ";
            for(unsigned i = 1; i < fop.arg.size(); ++i) { 
                if(i > 1) out << ", ";
                out << fop.arg[i]; 
            }
            break;
        case COFI:           // Convert right value from float to int    arg[0]: grpc type to
        case COFS:           // Convert right value from float to string
        case COFB:           // Convert right value from float to bool
        case COFF:           // Convert float to float                   argp[0]: grpc type to, arg[1] grpc type from
        case COIF:           // Convert right value from int to float    arg[0]: grpc type to 
        case COIS:           // Convert right value from int to string
        case COIB:           // Convert right value from int to bool
        case COII:           // Convert intger to integer                arg[0]: grpc type to, arg[1]: grpc type from
        case COSF:           // Convert right value from string to float arg[0]: grpc type to
        case COSI:           // Convert right value from string to int   arg[0]: grpc type to
        case COSB:           // Convert right value from string to bool
        case COEI:           // Convert right value from enum to int     arg[0] grpc type to
        case COEB:           // Convert right value from enum to int     arg[0] grpc type to
        case COEF:           // Convert right value from enum to float   arg[0] grpc type to    
        case COES:           // Convert right value from enum to string
        case COEE:           // Convert right value from enum to another enum
            out << ANSI_GREEN << ANSI_BOLD
                << op_name(fop.code)  << ANSI_RESET << "  ";
            if(fop.arg.size() > 0) 
                out << ANSI_GREEN << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[0]) << ANSI_RESET << " ";
            if(fop.arg.size() > 1)
                out << "<- " << ANSI_GREEN << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[1]) << ANSI_RESET << " ";
            break;
            if(fop.er != nullptr) 
                out << "er: " << ANSI_RED << get_full_name(fop.er) << ANSI_RESET << " ";
        default:
            break;
    }
    return out;
}

std::ostream &operator<< (std::ostream &out, fop const &fop) {
    switch(fop.code) {
        case LOOP:
        case BNOD:
        case BERC:
        case IFNC:
        case BSTG:
        case RVF:
        case COFI:           // Convert right value from float to int    arg[0]: grpc type to
        case COFS:           // Convert right value from float to string
        case COFB:           // Convert right value from float to bool
        case COFF:           // Convert float to float                   argp[0]: grpc type to, arg[1] grpc type from
        case COIF:           // Convert right value from int to float    arg[0]: grpc type to 
        case COIS:           // Convert right value from int to string
        case COIB:           // Convert right value from int to bool
        case COII:           // Convert intger to integer                arg[0]: grpc type to, arg[1]: grpc type from
        case COSF:           // Convert right value from string to float arg[0]: grpc type to
        case COSI:           // Convert right value from string to int   arg[0]: grpc type to
        case COSB:           // Convert right value from string to bool
        case COEI:           // Convert right value from enum to int     arg[0] grpc type to
        case COEB:           // Convert right value from enum to int     arg[0] grpc type to
        case COEF:           // Convert right value from enum to float   arg[0] grpc type to    
        case COES:           // Convert right value from enum to string
        case COEE:           // Convert right value from enum to another enum
            return propc(out, fop);
        default:
            break;
    }

    if(fop.code == MTHD || fop.code == BPRP || fop.code == ERR) out << ANSI_BOLD;
    if(fop.code == ERR) out << ANSI_RED;
    out << op_name(fop.code) << "  ";
    if(fop.code == MTHD || fop.code == BPRP || fop.code == ERR) out << ANSI_RESET;

    bool escape_string1 = fop.code == RVC && fop.arg.size() > 1 && fop.arg[1] == (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING;
    bool escape_string2 = fop.code == RVC && fop.arg.size() > 2 && fop.arg[2] == (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING;
    if(!fop.arg2.empty()) {
        out << ANSI_BLUE << (escape_string1? stru1::c_escape(fop.arg1): fop.arg1)  << ANSI_RESET << ", " << ANSI_BLUE 
            << (escape_string2? stru1::c_escape(fop.arg2): fop.arg2) << ANSI_RESET << " ";
    } else if(!fop.arg1.empty()) {
        out << ANSI_BLUE << (escape_string1? stru1::c_escape(fop.arg1): fop.arg1) << ANSI_RESET << " ";
    }
    if(fop.d2 != nullptr) {
        if(fop.d1 == nullptr) out << "0, ";
        else out << ANSI_MAGENTA << "d1: " << fop.d1->full_name() << ANSI_RESET << ", ";
        out << ANSI_MAGENTA << "d2: " << fop.d2->full_name() << " " << ANSI_RESET;
    } else if(fop.d1 != nullptr) {
        out << ANSI_MAGENTA << "d1: " << fop.d1->full_name() << " " << ANSI_RESET;
    }
    if(fop.m1 != nullptr) 
        out << ANSI_GREEN << "m1: " << fop.m1->full_name() << ANSI_RESET << " ";
    if(fop.code > CON1 && fop.code < CON2) {
        if(fop.arg.size() > 0) 
            out << ANSI_GREEN << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[0]) << ANSI_RESET << " ";
        if(fop.arg.size() > 1)
            out << "<- " << ANSI_GREEN << grpc_type_name((google::protobuf::FieldDescriptor::Type) fop.arg[1]) << ANSI_RESET << " ";
    }
    if(fop.el != nullptr) 
        out << "el: " << ANSI_RED << get_full_name(fop.el) << ANSI_RESET << " ";
    if(fop.er != nullptr) 
        out << "er: " << ANSI_RED << get_full_name(fop.er) << ANSI_RESET << " ";
    if(fop.ev1 != nullptr) 
        out << "ev1: " << ANSI_RED << fop.ev1->full_name() << ANSI_RESET << " ";
    if(fop.ext != 0) 
        out << "ext: " << ANSI_RED << node_name(fop.ext) << ANSI_RESET << " ";
    bool first = true;
    for(auto a: fop.arg) { 
        if(!first) out << ", ";
        out << a; 
        first = false;
    }
    return out;
}

