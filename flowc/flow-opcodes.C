
#include <iostream>
#include "ansi-escapes.H"
#include "flow-opcodes.H"
#include "grpc-helpers.H"

static
char const *op_name(op o) { 
    switch(o) {
        case NOP:   return "NOP";
        case LOOP:  return "LOOP";
        case MTHD:  return "MTHD";
        case BNOD:  return "BNOD";
        case NSET:  return "NSET";
        case IFNC:  return "IFNC";
        case BPRP:  return "BPRP";
        case EPRP:  return "EPRP";
        case BSTG:  return "BSTG";
        case CALL:  return "CALL";
        case FUNC:  return "FUNC";
        case COPY:  return "COPY";
        case ELP:   return "ELP";
        case END:   return "END";
        case ENOD:  return "ENOD";
        case ESTG:  return "ESTG";
        case INDX:  return "INDX";
        case SET:   return "SET";

        case SETT:  return "SETT";
        case ERR:   return "ERR";

        case SETL: return "SETL";      
        case RVA: return "RVA";      
        case RVC: return "RVC";      
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
static char const *NOP_fmt[] = {"NOP", nullptr};

static char const *MTHD_fmt[] = {"MTHD", "input name", "output name", "input type", "output type", "method", "node", nullptr};
static char const *END_fmt[] = {"END", nullptr};

static char const *BSTG_fmt[] = {"BSTG", "name", "", "", "", "", "number", "node count", "max dim", "node", nullptr};
static char const *ESTG_fmt[] = {"ESTG", "name", "", "", "", "", "number", nullptr};

static char const *BNOD_fmt[] = {"BNOD", "output name", "input name", "output type", "input type", "method", "dim", "node", "stage", "foak", "foak-wo", nullptr};

static char const *NSET_fmt[] = {"NSET", nullptr};
static char const *IFNC_fmt[] = {"IFNC", "name", "", "", "", "", "node", "t-exp", "f-exp", nullptr};
static char const *ENOD_fmt[] = {"ENOD", "output name", ""};
static char const *BPRP_fmt[] = {"BPRP", nullptr};
static char const *EPRP_fmt[] = {"EPRP", nullptr};
            
static char const *LOOP_fmt[] = {"LOOP", nullptr};
static char const *ELP_fmt[] = {"ELP", nullptr};

static char const *ERR_fmt[] = {"ERR", "message", nullptr};
static char const *CALL_fmt[] = {"CALL", "input", "output", "", "", "method", nullptr};
static char const *COPY_fmt[] = {"COPY", "to", "from", "to type", "from type", nullptr};
static char const *FUNC_fmt[] = {"FUNC", nullptr};

static char const *INDX_fmt[] = {"INDX", nullptr};
/*
        case SET:   return "SET";
        case SETT:  return "SETT";
        case SETL: return "SETL";      
        case RVA: return "RVA";      
        case RVC: return "RVC";      
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
        */

char const **op_arg_names(op o) {
    char const **fmt = nullptr;
    switch(o) {
        case NOP:   fmt = NOP_fmt;
        case MTHD:  fmt = MTHD_fmt;
        case END:   fmt = END_fmt;

        case BSTG:  fmt = BSTG_fmt;
        case ESTG:  fmt = ESTG_fmt;

        case BNOD:  fmt = BNOD_fmt;
       
        case NSET:  fmt = NSET_fmt; 
        case IFNC:  fmt = IFNC_fmt;
        case ENOD:  fmt = ENOD_fmt;
        case BPRP:  fmt = BPRP_fmt;
        case EPRP:  fmt = EPRP_fmt;
                    
        case LOOP:  fmt = LOOP_fmt;
        case ELP:   fmt = ELP_fmt;

        case ERR:   fmt = ERR_fmt;
        case CALL:  fmt = CALL_fmt;
        case COPY:  fmt = COPY_fmt;
        case FUNC:  fmt = FUNC_fmt;
        case INDX: fmt = INDX_fmt;
        default: break;
    }
    return fmt;
}
std::ostream &operator<< (std::ostream &out, fop const &fop) {
    bool use_ansi = ansi::use_escapes && (&out == &std::cerr || &out == &std::cout);

    if(fop.code == MTHD || fop.code == BSTG || fop.code == BPRP) out << ansi::escape(ANSI_BOLD, use_ansi);
    else if(fop.code == INDX) out << ansi::escape(ANSI_YELLOW, use_ansi);

    out << op_name(fop.code) << "  ";
    if(fop.code == MTHD || fop.code == BSTG || fop.code == BPRP || fop.code == INDX) out << ansi::escape(ANSI_RESET, use_ansi);

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
    bool first = true;
    if((fop.code == LOOP || fop.code == NSET) && fop.arg.size() > 0) 
        out << ansi::escape(ANSI_YELLOW, use_ansi) << "INDX @" << ansi::escape(ANSI_RESET, use_ansi);
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
    for(auto a: fop.arg) { 
        if(!first) out << ", ";
        out << a; 
        first = false;
    }
    return out;
}
