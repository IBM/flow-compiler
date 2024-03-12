#include <fstream>
#include <iostream>
#include "ast.H"
#include "ansi-escapes.H"
#include "flow-comp.H"
#include "stru.H"
#include "value-type.H"

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
           if(!token.text.empty())
            out << ANSI_CYAN << '\'' << token.text << '\'' << ANSI_RESET;
   }
   return out;
}
std::ostream &to_json(std::ostream &s, fc::value_type const &vt) {
    switch(vt.type) {
        case fc::fvt_none:
            s << "\"??\"";
            break;
        case fc::fvt_any:
            s << "\"*\"";
            break;
        case fc::fvt_num:
            s << "\"n\"";
            break;
        case fc::fvt_basic:
            s << "\"b\"";
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
            s << "["; to_json(s, vt.inf[0]); s << "]";
            break;

        case fc::fvt_struct:
            s << "{\"struct\":[";
            for(unsigned u = 0, e = vt.inf.size(); u < e; ++u) {
                if(u > 0) s << ", ";
                to_json(s, vt.inf[u]);
            }
            s << "]";
            if(!vt.gname.empty()) 
                s << ",\"name\":" << stru::json_escape(vt.gname);
            s << "}";
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
            << ",\"type\":" << stru::json_escape(tk_to_string(node.type));
        if(node.type == FTK_STRING) 
            out << ",\"lexeme\":" << stru::json_escape(stru::json_unescape(node.token.text));
        else if(!node.token.text.empty())
            out << ",\"lexeme\":" << stru::json_escape(node.token.text);
        if(vtype.has(*p)) 
            out << ",\"vtype\":";  to_json(out, vtype.get(*p));
        if(rpc.has(*p)) 
            out << ",\"rpc\":" << stru::json_escape(rpc.get(*p));
        if(cmsg.has(*p)) 
            out << ",\"cmsg\":" << stru::json_escape(cmsg.get(*p));
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
        auto node = at(*p);
        out << node.token.file << ":" << node.token.line << ":" << node.token.column << " ";
        out << ANSI_BOLD << tk_to_string(node.type) << ANSI_RESET;
        if(node.children.size()) out << "[" << node.children.size() << "]";
        if(unid.has(*p))
            out << ":" << ANSI_BOLD+ANSI_HRED << unid.get(*p) << ANSI_RESET;

        if(ref.has(*p)) 
            out << ANSI_BOLD+ANSI_RED << "->" << ref.get(*p) << ANSI_RESET;

        if(precedence.has(*p)) 
            out << ANSI_BOLD+ANSI_BLUE << "^" << precedence.get(*p) << ANSI_RESET;
        
        if(is_operator(*p)) 
            out << ANSI_BOLD+ANSI_BLUE << "º" << ANSI_RESET;

        if(vtype.has(*p) || const_level(*p)) {
            out << " ";
            switch(const_level(*p)) {
                case 3:
                    out << ANSI_BOLD+ANSI_GREEN << "⟪" << ANSI_RESET;
                    break;
                case 2:
                    out << ANSI_BOLD+ANSI_BLUE << "⟨" << ANSI_RESET;
                    break;
                case 1:
                    out << ANSI_BOLD+ANSI_YELLOW << "⦗" << ANSI_RESET;
                    break;
                default:
                    out << ANSI_BOLD+ANSI_YELLOW << "⧼" << ANSI_RESET;
                    break;
            }
            if(vtype.has(*p))
                out << vtype.get(*p);
            else
                out << ".";
            switch(const_level(*p)) {
                case 3:
                    out << ANSI_BOLD+ANSI_GREEN << "⟫" << ANSI_RESET;
                    break;
                case 2:
                    out << ANSI_BOLD+ANSI_BLUE << "⟩" << ANSI_RESET;
                    break;
                case 1:
                    out << ANSI_BOLD+ANSI_YELLOW << "⦘" << ANSI_RESET;
                    break;
                default:
                    out << ANSI_BOLD+ANSI_YELLOW << "⧽" << ANSI_RESET;
                    break;
            }
        }

        int attrs = 0; char close = 0;
        if(rpc.has(*p) || cmsg.has(*p)) {
            out << " {"; close = '}';
        }
        if(rpc.has(*p)) {
            if(attrs) out << ", ";
            out << "rpc: " << ANSI_BOLD+ANSI_YELLOW << rpc.get(*p) << ANSI_RESET;
            ++attrs;
        }
        if(cmsg.has(*p)) {
            if(attrs) out << ", ";
            out << "cmsg: " << ANSI_BOLD+ANSI_YELLOW << cmsg.get(*p) << ANSI_RESET;
            ++attrs;
        }
        if(close != 0)
            out << close;
        out << " ";
        p_token(out, at(*p).token) << "\n";
    }
}
}

