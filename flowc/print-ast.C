#include <fstream>
#include "ast.H"
#include "ansi-escapes.H"
#include "flow-comp.H"
#include "stru.H"

namespace {
    
std::ostream &p_token(std::ostream &out, ast::token const &token) {
   switch(token.type) {
       case FTK_INTEGER:
           out << ANSI_CYAN << token.text << ANSI_RESET;
           break;
       case FTK_FLOAT:
           out << ANSI_YELLOW << token.text << ANSI_RESET;
           break;
       case FTK_STRING:
           out << ANSI_GREEN << token.text << ANSI_RESET;
           break;
       case FTK_ID:
           out << ANSI_MAGENTA << token.text << ANSI_RESET;
           break;
       default:
           out << ANSI_CYAN << token.text << ANSI_RESET;
   }
   return out;
}
/*
std::ostream &p_node(std::ostream &out, ast::node const &node) {
    std::string type = fc::compiler::ftk_to_string(node.type);
    out << node.token.file << ":" << node.token.line << ":" << node.token.column << " ";
    //out << type << "(" << node.type << ")";
    out << ANSI_BOLD << type << ANSI_RESET;
    if(node.children.size()) out << "[" << node.children.size() << "]";
    out << " "; p_token(out, node.token);
    return out;
}
*/
void print_node_info(std::ostream &out, ast::node const &node) {
    std::string type = fc::compiler::ftk_to_string(node.type);
    out << node.token.file << ":" << node.token.line << ":" << node.token.column << " ";
    out << ANSI_BOLD << type << ANSI_RESET;
    if(node.children.size()) out << "[" << node.children.size() << "]";
}

}
/*
std::ostream &operator <<(std::ostream &out, ast::token const &token) {
    return p_token(out, token);
}
std::ostream &operator <<(std::ostream &out, ast::node const &node) {
    return p_node(out, node);
}
*/
namespace fc {
/*
void compiler::ast_to_json(std::ostream &out, int node) const {
    if(node == 0) {
        for(auto p = begin(node), e = end(); p != e; ++p)
            if(at(*p).type == FTK_wfgsrc) {
                node = *p;
                break;
            }
    }
    if(node != 0) {
        out << "{\"id\": " << node << ",";
        auto n = at(node);
        out << "\"line\":" << n.token.line << ",\"col\":" << n.token.column << ",";
        out << "\"type\":" << stru::json_escape(fc::compiler::stoken(n.type));
        switch(n.type) {
            case FTK_INTEGER: 
                out << ",\"value\":" << atoll(n.token.text.c_str());
                break;
            case FTK_FLOAT:
                out << ",\"value\":" << atof(n.token.text.c_str());
                break;
            case FTK_ID: case FTK_STRING:
                out << ",\"value\":" << stru::json_escape(n.token.text);
                break;
            default:
                break;
        }
        if(n.children.size() > 0) {
            out << ",\"children\":[";
            for(int cx = 0, ce = n.children.size(); cx != ce; ++cx) {
                if(cx > 0) out << ",";
                json_ast(out, n.children[cx]);
            }
            out << "]";
        }
        out << "}\n";
    }
}
*/
static
std::ostream &operator << (std::ostream &s, value_type const &vt) {
    switch(vt.type) {
        case fvt_none:
            s << "??";
            break;
        case fvt_int:
            s << "int";
            break;
        case fvt_flt:
            s << "flt";
            break;
        case fvt_str:
            s << "str";
            break;

        case fvt_enum:
            s << "enu";
            if(vt.gtype != nullptr)
                s << ": " << vt.gtype;
            break;

        case fvt_array:
            s << "[" << vt.inf[0] << "]";
            break;

        case fvt_struct:
            s << "(";
            for(unsigned u = 0, e = vt.inf.size(); u < e; ++u) {
                if(u > 0) s << ", ";
                s << vt.inf[u];
            }
            if(vt.gtype != nullptr)
                s << ": " << vt.gtype;
            s << ")";
            break;
    }
    return s;
}
void compiler::print_ast(std::ostream &out, int node) const {
    for(auto p = begin(node), e = end(); p != e; ++p) {
        out << std::string((p.level()-1)*4, ' ') << ANSI_BOLD << *p << ANSI_RESET << "-";
        print_node_info(out, at(*p));
        int attrs = 0;
        if(vtype.has(*p) || ref.has(*p))
            out << "(";
        if(vtype.has(*p)) {
            out << "vtype: " << ANSI_HBLUE << vtype.get(*p) << ANSI_RESET;
            ++attrs;
        }
        if(ref.has(*p)) {
            if(attrs) out << ", ";
            out << "ref: " << ANSI_BOLD+ANSI_RED << ref.get(*p) << ANSI_RESET;
            ++attrs;
        }
        if(attrs)
            out << ")";
        out << " ";
        p_token(out, at(*p).token) << "\n";

    }
}
}

