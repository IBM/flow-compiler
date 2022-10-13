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
     *      arg[]   dimension, node, stages, foak, foak-with-output
     */
    BNOD,           // node begin
    /**    
     *      arg1    node name
     *      arg[]   node, condition-node, neg condition-node...
     */
    IFNC,           // check node condition 
    ENOD,           // node end
    BPRP,           // result preparation begin
    EPRP,           // result preparation end
    LOOP,           // loop
    LPC,            // loop size
    NLP,            // node loop 
    BNL,            // node begin
    BLP,            // begin loop body
    ELP,            // end loop body                       
    /**
     * All set fileds have:
     *      arg1    left value string representation as message-type[[+]field-name]*
     *      d1      descriptor for message-type
     *      arg[0]  right value node-id
     */

    SETF,           // Set field 
    SETE,           // Check and set enum field
    RVF,            // Right value field
    SVF,            // Right value repeated field size
    RVC,            // Right value constant
    RVV,            // Right value variable
    DACC,           // Dimension accessor arg[0] = dimensions 
    FLAT,           // Flat accessor
    STOL,           // Stack field length
    CLLS,           // Clear field length stack

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
    BERC, EERC,     // begin and end error pseudo node
    ERR,            // generate a grpc error
    /**
     * arg1 function name or operand
     * arg[0] number of arguments to use
     */
    FUNC,           // operand call internal function
    IOP,            // infix operator
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
    int ext = 0;
    fop(op c=NOP): code(c) {}
    fop(op c, std::string const &a1, std::string const &a2=""): code(c), arg1(a1), arg2(a2) {}
    fop(op c, std::string const &a1, std::string const &a2, Descriptor const *ad1, Descriptor const *ad2=nullptr): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2) {}
    fop(op c, std::string const &a1, Descriptor const *ad1): code(c), arg1(a1), d1(ad1)  {}
    fop(op c, std::string const &a1, Descriptor const *ad1, int i): code(c), arg1(a1), d1(ad1)  {arg.push_back(i);}
    //fop(op c, std::string const &a1, std::string const &a2, Descriptor const *ad1, int i): code(c), arg1(a1), arg2(a2), d1(ad1)  {arg.push_back(i);}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1): code(c), arg1(a1), arg2(a2), m1(am1) {}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1, Descriptor const *ad1, Descriptor const *ad2): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2), m1(am1) {}
    fop(op c, int i): code(c) { arg.push_back(i); }
    fop(op c, int i, int j): code(c) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i): code(c), arg1(a1) { arg.push_back(i); }
    fop(op c, std::string const &a1, std::string const &a2, int i): code(c), arg1(a1), arg2(a2) { arg.push_back(i); }
    fop(op c, std::string const &a1, int i, int j): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, std::string const &a2, int i, int j): code(c), arg1(a1), arg2(a2) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i, int j, int k): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j);  arg.push_back(k); }
};
// Enum to string
char const *op_name(op o);
std::ostream &operator<< (std::ostream &out, fop const &fop);
#endif
