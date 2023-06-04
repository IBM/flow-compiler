#include <string>
#include <iostream>

#include "flow-comp.H"
#include "stru.H"

namespace fc {

int compiler::eval(int node, std::string &value, value_type *vtp) {
    int rc = 0;
    switch(at(node).type) {
        case FTK_valx:
            switch(atc(node, 0).type) {
                case FTK_INTEGER:
                case FTK_FLOAT:
                    value = atc(node, 0).token.text;
                    break;
                case FTK_STRING:
                    value = stru::json_unescape(atc(node, 0).token.text);
                    break;
                default:
                    std::cerr << "unhandled valx: \n";
                    print_ast(node);
                    rc = 1; 
            }
            break;
        default:
            std::cerr << "eval called on other than valx: \n";
            print_ast(node);
            rc = 1;
    }
    if(rc == 0 && vtp != nullptr)
        *vtp = vtype(node);
    return rc;
}

}
