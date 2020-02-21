#ifndef H_FLOW_OPCODES_H
#define H_FLOW_OPCODES_H

#include <iosfwd>
#include <string>
#include <google/protobuf/compiler/importer.h>

using namespace google::protobuf;

enum op { 
    NOP,            // no operation comment
    /** 
     *      arg1    request name
     *      arg2    response name 
     *      d1      descriptor for the request
     *      d2      descriptor for the response 
     *      m1      method descriptor
     *      arg[]   nodes used by the entry
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
    /**
     *      arg1    stage name
     *      arg[0]  stage number
     */
    ESTG,           // stage end
    /**
     *      arg1    response name 
     *      arg2    request name
     *      d1      output descriptor
     *      d2      input descriptor
     *      arg[]   dimension, node, stages, base-node
     */
    BNOD,           // node begin
    RNOD,           // setup result structures for this node type
    NSET,           // node setup
    /**
     *      arg1    node name
     *      arg[]   node, base-node, condition-node
     */
    NDIF,           // check node condition
    /**
     *      arg1    node name
     *      arg[]   node, base-node, 0
     */
    BNIF,           // check node if node was visited (for base nodes)
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

    SETL,           // Set field 
    RVA,            // Right value field accessor
    RVC,            // Right value constant

    CON1,           // Conversion NOP 1

    COFI,           // Convert right value from float to int    arg[0]: grpc type to
    COFS,           // Convert right value from float to string
    COFB,           // Convert right value from float to bool
    COFF,           // Convert float to float                   argp[0]: grpc type to, arg[1] grpc type from

    COIF,           // Convert right value from int to float    arg[0]: grpc type to 
    COIS,           // Convert right value from int to string
    COIB,           // Convert right value from int to bool
    COII,           // Convert intger to integer                arg[0]: grpc type to, arg[1]: grpc type from

    COSF,           // Convert right value from string to float arg[0]: grpc type to
    COSI,           // Convert right value from string to int   arg[0]: grpc type to
    COSB,           // Convert right value from string to bool

    COEI,           // Convert right value from enum to int     arg[0] grpc type to
    COEB,           // Convert right value from enum to int     arg[0] grpc type to
    COEF,           // Convert right value from enum to float   arg[0] grpc type to    
    COES,           // Convert right value from enum to string
    COEE,           // Convert right value from enum to another enum

    CON2,           // Conversion NOP 2

    SET,            // set field from another field of same type

    SETT,           // set field from temporary variable of same type

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
     *      arg     ast-node-ids for all fields up to the last repeated field
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
    EnumDescriptor const *el = nullptr;
    EnumDescriptor const *er = nullptr;
    EnumValueDescriptor const *ev1 = nullptr;
    fop(op c=NOP): code(c) {}
    fop(op c, std::string const &a1, std::string const &a2=""): code(c), arg1(a1), arg2(a2) {}
    fop(op c, std::string const &a1, std::string const &a2, Descriptor const *ad1, Descriptor const *ad2=nullptr): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2) {}
    fop(op c, std::string const &a1, Descriptor const *ad1): code(c), arg1(a1), d1(ad1)  {}
    fop(op c, std::string const &a1, Descriptor const *ad1, int i): code(c), arg1(a1), d1(ad1)  {arg.push_back(i);}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1): code(c), arg1(a1), arg2(a2), m1(am1) {}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1, Descriptor const *ad1, Descriptor const *ad2): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2), m1(am1) {}
    fop(op c, int i): code(c) { arg.push_back(i); }
    fop(op c, int i, int j): code(c) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i): code(c), arg1(a1) { arg.push_back(i); }
    fop(op c, std::string const &a1, int i, int j): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i, int j, int k): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j);  arg.push_back(k); }
};
inline static char const *op_name(op o) { 
    switch(o) {
        case NOP:   return "NOP";
        case LOOP:  return "LOOP";
        case MTHD:  return "MTHD";
        case BNOD:  return "BNOD";
        case RNOD:  return "RNOD";
        case NSET:  return "NSET";
        case BNIF:  return "BNIF";
        case NDIF:  return "NDIF";
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
};
std::ostream &operator<< (std::ostream &out, fop const &fop);
#endif
