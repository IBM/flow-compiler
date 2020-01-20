#ifndef H_FLOW_OPCODES_H
#define H_FLOW_OPCODES_H

#include <iostream>
#include <string>
#include <google/protobuf/compiler/importer.h>

using namespace google::protobuf;

#include "ansi-escapes.H"


enum op { 
    NOP,            // no operation comment
    /** 
     *      arg1    request name
     *      arg2    response name 
     *      d1      descriptor for the request
     *      d2      descriptor for the response 
     *      m1      method descriptor
     *      arg[]   nodes used by thie emntry
     */      
    MTHD,           // method begin
    END,            // method end
    /**
     *      arg1    stage name
     *      arg[0]  stage number
     *      arg[1]  the number of nodes in this stage
     *      arg[2]  max stage dim
     *      arg 3.. node ids
     */
    BSTG,           // stage begin
    ESTG,           // stage end
    /**
     *      arg1    response name 
     *      arg2    request name
     *      d1      output descriptor
     *      d2      input descriptor
     *      arg[]   dimension node stages base-node
     */
    BNOD,           // node begin
    RNOD,           // setup result structures for this node type
    NSET,           // node setup
    CHK,            // check condition
    ENOD,           // node end
    BPRP,           // result preparation begin
    EPRP,           // result preparation end
    LOOP,           // begin loop
    ELP,            // end loop                         
    /**
     * All set fileds have:
     *      arg1    left value string representation as message-type[[+]field-name]*
     *      d1      descriptor for message-type
     *      arg[0]  right value node-id
     */
    SET,            // set field from another field of same type

    SETI,           // set field with integer constant
    SETS,           // set field with string constant
    SETE,           // set field with enum constant
    SETF,           // set field with float constant

    SETT,           // set field from temporary variable of same type

    SETFFI,         // set field from another field, converting float to int
    SETFFS,         // set field from another field, converting float to string
    SETFIF,         // set field from another field, converting integer to float
    SETFIS,         // set field from another field, converting integer to string
    SETFSF,         // set field from another field, converting string to float
    SETFSI,         // set field from another field, converting string to integer
    SETFEI,         // set field from another field, converting enum to integer
    SETFES,         // set field from another field, converting enum to string
    SETFEF,         // set field from another field, converting enum to float
    SETFEE,         // set field from another field, converting enum to another compatible enum

    SETTFI,         // set field from variable, converting float to int
    SETTFS,         // set field from variable, converting float to string
    SETTIF,         // set field from variable, converting integer to float
    SETTIS,         // set field from variable, converting integer to string
    SETTSF,         // set field from variable, converting string to float
    SETTSI,         // set field from variable, converting string to integer
    SETTEI,         // set field from variable, converting enum to integer
    SETTES,         // set field from variable, converting enum to string
    SETTEF,         // set field from variable, converting enum to float
    SETTEE,         // set field from variable, converting enum to another compatible enum

    /** 
     *      arg1    left value string representation as message-type[[+]field-name]*
     *      arg2    right value string representation as message-type[[+]field-name]*
     *      d1      descriptor for the left value message type
     *      d2      descriptor for the right value message type
     */      
    COPY,           // copy field
    /** 
     *      arg1    request message name
     *      arg2    response message name
     *      m1      method descriptor
     */
    CALL,           // call method
    /**
     *      arg1    error message
     */
    ERR,            // generate a grpc error
    FUNC,           // call internal function
    /**
     *      arg1    right value name
     *      d1      right value descriptor
     *      arg     node ids for all fields up to the last repeated field
     */
    INDX            // index data
};
struct fop {
    op code;
    std::string arg1, arg2;                
    Descriptor const *d1 = nullptr;
    Descriptor const *d2 = nullptr;
    MethodDescriptor const *m1 = nullptr;
    std::vector<int> arg;
    fop(op c=NOP): code(c) {}
    fop(op c, std::string const &a1, std::string const &a2=""): code(c), arg1(a1), arg2(a2) {}
    fop(op c, std::string const &a1, std::string const &a2, Descriptor const *ad1, Descriptor const *ad2=nullptr): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2) {}
    fop(op c, std::string const &a1, Descriptor const *ad1, int i): code(c), arg1(a1), d1(ad1)  {arg.push_back(i);}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1): code(c), arg1(a1), arg2(a2), m1(am1) {}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1, Descriptor const *ad1, Descriptor const *ad2): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2), m1(am1) {}
    fop(op c, int i): code(c) { arg.push_back(i); }
    fop(op c, int i, int j): code(c) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i, int j): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i, int j, int k): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j);  arg.push_back(k); }
};
inline static char const *op_name(op o) { 
    switch(o) {
        case NOP:  return "NOP";
        case LOOP:  return "LOOP";
        case MTHD:  return "MTHD";
        case BNOD:  return "BNOD";
        case RNOD:  return "RNOD";
        case NSET:  return "NSET";
        case CHK:   return "CHK";
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
        case SETF:  return "SETF";
        case SETE:  return "SETE";
        case SETS:  return "SETS";
        case SETI:  return "SETI";
        case SETT:  return "SETT";
        case ERR:   return "ERR";

        case SETFFI: return "SETFFI";         
        case SETFFS: return "SETFFS";         
        case SETFIF: return "SETFIF";         
        case SETFIS: return "SETFIS";         
        case SETFSF: return "SETFSF";         
        case SETFSI: return "SETFSI";         
        case SETFEI: return "SETFEI";         
        case SETFES: return "SETFES";         
        case SETFEF: return "SETFEF";         
        case SETFEE: return "SETFEE";        
        case SETTFI: return "SETTFI";         
        case SETTFS: return "SETTFS";         
        case SETTIF: return "SETTIF";         
        case SETTIS: return "SETTIS";         
        case SETTSF: return "SETTSF";         
        case SETTSI: return "SETTSI";        
        case SETTEI: return "SETTEI";       
        case SETTES: return "SETTES";       
        case SETTEF: return "SETTEF";      
        case SETTEE: return "SETTEE";     
    }
    return "";
};
inline static std::ostream &operator<< (std::ostream &out, fop const &fop) {
    bool use_ansi = ansi::use_escapes && (&out == &std::cerr || &out == &std::cout);

    if(fop.code == MTHD || fop.code == BSTG || fop.code == BPRP) out << ansi::escape(ANSI_BOLD, use_ansi);
    out << op_name(fop.code) << "  ";
    if(fop.code == MTHD || fop.code == BSTG || fop.code == BPRP) out << ansi::escape(ANSI_RESET, use_ansi);
    if(!fop.arg2.empty()) {
        out << ansi::escape(ANSI_BLUE, use_ansi) << "\"" << fop.arg1 << "\"" << ansi::escape(ANSI_RESET, use_ansi) << "," << ansi::escape(ANSI_BLUE, use_ansi) 
            << " \"" << fop.arg2 << "\" " << ansi::escape(ANSI_RESET, use_ansi);
    } else if(!fop.arg1.empty()) {
        out << ansi::escape(ANSI_BLUE, use_ansi) << "\"" << fop.arg1 << "\" " << ansi::escape(ANSI_RESET, use_ansi);
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
    for(auto a: fop.arg) { 
        if(!first) out << ", ";
        out << a; 
        first = false;
    }
    return out;
}
#endif
