#include <fstream>
#include <iostream>
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
std::ostream &operator << (std::ostream &s, fc::value_type const &vt) {
    switch(vt.type) {
        case fc::fvt_none:
            s << "??";
            break;
        case fc::fvt_int:
            s << "int";
            break;
        case fc::fvt_flt:
            s << "flt";
            break;
        case fc::fvt_str:
            s << "str";
            break;

        case fc::fvt_enum:
            s << "enu";
            if(vt.gtype != nullptr)
                s << ": " << vt.gtype;
            break;

        case fc::fvt_array:
            s << "[" << vt.inf[0] << "]";
            break;

        case fc::fvt_struct:
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
std::ostream &to_json(std::ostream &s, fc::value_type const &vt) {
    switch(vt.type) {
        case fc::fvt_none:
            s << "\"??\"";
            break;
        case fc::fvt_int:
            s << "\"int\"";
            break;
        case fc::fvt_flt:
            s << "\"flt\"";
            break;
        case fc::fvt_str:
            s << "\"str\"";
            break;

        case fc::fvt_enum:
            s << "\"enum\"";
            break;

        case fc::fvt_array:
            s << "[" << vt.inf[0] << "]";
            break;

        case fc::fvt_struct:
            s << "{\"struct\":[";
            for(unsigned u = 0, e = vt.inf.size(); u < e; ++u) {
                if(u > 0) s << ", ";
                s << vt.inf[u];
            }
            s << "]}";
            break;
    }
    return s;
}
}
namespace fc {
void compiler::ast_to_json(std::ostream &out, int node) const {
    std::map<int, int> nm;
    int n = 0;
    for(auto p = begin(node), e = end(); p != e; ++p) 
        nm[*p] = ++n;
    
    out << "[";
    n = 0;
    for(auto p = begin(node), e = end(); p != e; ++p) {
        if(++n > 1) out << ",";
        auto node = at(*p);
        out << "{\"id\":" << nm[*p] 
            << ",\"file\":" << node.token.file << ",\"line\":" << node.token.line << ",\"column\":" << node.token.column
            << ",\"type\":" << stru::json_escape(fc::compiler::ftk_to_string(node.type));
        if(node.type == FTK_STRING) 
            out << ",\"lexeme\":" << stru::json_escape(stru::json_unescape(node.token.text));
        else if(!node.token.text.empty())
            out << ",\"lexeme\":" << stru::json_escape(node.token.text);
        if(vtype.has(*p)) 
            out << ",\"vtype\":";  to_json(out, vtype.get(*p));
        if(node.children.size() > 0) {
            int cc = 0;
            out << ",\"children\": [";
            for(int c: node.children) {
                if(++cc > 1) out << ",";
                out << nm[c];
            }
            out << "]";
        }
        out << "}\n";
    }
    out << "]";
}
void compiler::print_ast(int node) const {
    print_ast(std::cerr, node);
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

