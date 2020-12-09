#include <iostream>
#include <string>
#include <limits>
#include <cmath>
#include "flow-ast.H"
#include "grpc-helpers.H"
#include "stru1.H"
#include "ansi-escapes.H"

using namespace stru1;

char const *node_name(int i) {
    switch(i) {
        case FTK_ID: return "identifier";
        case FTK_STRING: return "string";
        case FTK_INTEGER: return "integer";
        case FTK_FLOAT:  return "float";
        case FTK_ACCEPT: return "ACCEPT";
        case FTK_SYNTAX_ERROR: return "ERROR";
        case FTK_flow: return "flow";
        case FTK_stmt: return "stmt";
        case FTK_blck: return "hash-map";
        case FTK_lblk: return "lblk";
        case FTK_elem: return "elem";
        case FTK_bexp: return "bexp";
        case FTK_oexp: return "oexp";
        case FTK_rexp: return "rexp";
        case FTK_fldm: return "fldm";
        case FTK_fldr: return "fldr";
        case FTK_fldd: return "fldd";
        case FTK_fldx: return "fldx";
        case FTK_dtid: return "dtid";
        case FTK_node: return "node";
        case FTK_SEMICOLON: return ";";
        case FTK_DOT: return ".";
        case FTK_COMMA: return ",";
        case FTK_EQUALS: return "=";
        case FTK_COLON: return ":";
        case FTK_HASH: return "#";
        case FTK_AT: return "@";
        case FTK_OPENPAR: return "(";
        case FTK_CLOSEPAR: return ")";
        case FTK_OPENBRA: return "{";
        case FTK_CLOSEBRA: return "}";
        case FTK_OPENSQB: return "[";
        case FTK_CLOSESQB: return "]";
        case FTK_CARET: return "^";
        case FTK_LT: return "<";
        case FTK_GT: return ">";
        case FTK_BANG: return "!";
        case FTK_AMP: return "&";
        case FTK_BAR: return "|";
        case FTK_PLUS: return "+";
        case FTK_MINUS: return "-";
        case FTK_STAR:  return "*";
        case FTK_DOLLAR: return "$";
        case FTK_SLASH: return "/";
        case FTK_PERCENT: return "%";
        case FTK_QUESTION: return "?";
                        

        case FTK_NE: return "!=";
        case FTK_LE: return "<=";
        case FTK_GE: return ">=";
        case FTK_EQ: return "==";
        case FTK_OR: return "||";
        case FTK_AND: return "&&";
        
        case FTK_COMP: return "<=>";
        default: 
             return "symbol";
    }
}
int64_t flow_ast::get_integer(int node) const {
    assert(at(node).type == FTK_INTEGER);
    return at(node).token.integer_value;
}
double flow_ast::get_float(int node) const {
    assert(at(node).type == FTK_FLOAT);
    return at(node).token.float_value;
}
double flow_ast::get_numberf(int node) const {
    assert(at(node).type == FTK_FLOAT || at(node).type == FTK_INTEGER);
    return at(node).type == FTK_FLOAT? at(node).token.float_value: at(node).token.integer_value;
}
std::string flow_ast::get_number(int node, int grpc_type) const {
    auto const &n = at(node);
    assert(n.type == FTK_INTEGER || n.type == FTK_FLOAT);
    switch(grpc_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            return n.type == FTK_INTEGER? std::to_string((double) n.token.integer_value): std::to_string(n.token.float_value);

        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            return n.type == FTK_INTEGER? std::to_string((float) n.token.integer_value): std::to_string((float)n.token.float_value);

        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
            return n.type == FTK_INTEGER? std::to_string((int32_t) n.token.integer_value): std::to_string((int32_t)n.token.float_value);

        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
            return n.type == FTK_INTEGER? std::to_string(n.token.integer_value): std::to_string((int64_t)n.token.float_value);

        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            return n.type == FTK_INTEGER? std::to_string((uint32_t)n.token.integer_value): std::to_string((uint32_t)n.token.float_value);

        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            return n.type == FTK_INTEGER? std::to_string(*(uint64_t *)&n.token.integer_value)+"u": std::to_string((uint64_t)n.token.float_value);
        default:
            assert(false);
    }
}
bool flow_ast::can_cast(int node, int grpc_type) const {
    auto const &n = at(node);
    if(n.type != FTK_INTEGER && n.type != FTK_FLOAT) return false;
    switch(grpc_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            if(n.type == FTK_FLOAT) return true;
            return abs(n.token.integer_value) <= (1LL << std::numeric_limits<double>::digits);

        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            if(n.type == FTK_FLOAT) return true;
            return abs(n.token.integer_value) <= (1LL << std::numeric_limits<float>::digits);

        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
            if(n.type == FTK_INTEGER) return n.token.integer_value >= std::numeric_limits<int32_t>::min() && 
                n.token.integer_value <= std::numeric_limits<int32_t>::max();
            return ceil(n.token.float_value) == n.token.float_value &&
                n.token.float_value >= std::numeric_limits<int32_t>::min() &&
                n.token.float_value <= std::numeric_limits<int32_t>::max();

        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
            if(n.type == FTK_INTEGER) return true;
            return ceil(n.token.float_value) == n.token.float_value && 
                n.token.float_value >= std::numeric_limits<int64_t>::min() &&
                n.token.float_value <= std::numeric_limits<int64_t>::max();

        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            if(n.type == FTK_INTEGER) return n.token.integer_value >= 0 &&
                n.token.integer_value <= std::numeric_limits<uint32_t>::max();

            return ceil(n.token.float_value) == n.token.float_value &&
                n.token.float_value >= 0 &&
                n.token.float_value <= std::numeric_limits<uint32_t>::max();

        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            if(n.type == FTK_INTEGER) return true;
            return ceil(n.token.float_value) == n.token.float_value &&
                n.token.float_value >= 0 &&
                n.token.float_value <= std::numeric_limits<uint64_t>::max();

        default:
            break;
    }
    return false;
}
std::string const flow_ast::get_value(int node) const {
    auto type = at(node).type;
    assert(type == FTK_STRING || type == FTK_INTEGER || type == FTK_FLOAT);
    switch(type) {
        case FTK_STRING: return at(node).token.text;
        case FTK_INTEGER: return sfmt() << at(node).token.integer_value;
        case FTK_FLOAT: return sfmt() << at(node).token.float_value;
        default:
            assert(false);
    }
}
std::string const &flow_ast::get_string(int node) const {
    assert(at(node).type == FTK_STRING);
    return at(node).token.text;
}
std::string const &flow_ast::get_id(int node) const {
    assert(at(node).type == FTK_ID);
    return at(node).token.text;
}
std::string flow_ast::get_joined_id(int node, int start_pos, std::string const &j) const {
    auto const &n = at(node);
    if(n.type == FTK_ID) return get_id(node);
    assert(n.children.size() > start_pos && n.type == FTK_dtid || n.type == FTK_fldx);
    std::vector<std::string> ids(n.children.size()-start_pos);
    std::transform(n.children.begin()+start_pos, n.children.end(), ids.begin(), [this](int n)->std::string {return get_text(n);});
    return stru1::join(ids, j);
}
std::ostream &operator << (std::ostream &out, flow_ast_node const &node) {
    return out << "[" << node.type << "] " << node.token.text;
}
std::ostream &operator << (std::ostream &out, google::protobuf::FieldDescriptor const &d) {
    out << d.full_name();
    return out;
}
std::ostream &operator << (std::ostream &out, google::protobuf::Descriptor const &d) {
    out << d.full_name();
    return out;
}
std::ostream &operator << (std::ostream &out, google::protobuf::MethodDescriptor const &d) {
    out << d.full_name();
    return out;
}
std::ostream &operator << (std::ostream &out, google::protobuf::EnumValueDescriptor const &d) {
    out << d.full_name();
    return out;
}
int flow_ast::print_ast(std::ostream &sout, int node, int indent) const {
    if(node < 0) node = store.size();
    if(node > store.size() || node < 1) return indent;
    auto const &n = store[node-1];
    sout << std::string(indent, ' ');
   
    sout << node_name(n.type);
    if(flag.has(node)) sout << "|" << node_name(flag(node));
    sout << "[";
    if(condition.has_value(node)) sout << ANSI_RED << ANSI_BOLD << node << ANSI_RESET;
    else sout << node ;
    sout << "-" << n.type << " " << n.token.line << ":" << n.token.column << "]";

    if(n.token.text.length() > 0) sout << " " << n.token.text;
    if(n.token.type == FTK_FLOAT) sout << "  " << ANSI_YELLOW << n.token.float_value << ANSI_RESET;
    else if(n.token.type == FTK_INTEGER) sout << " " << ANSI_GREEN << n.token.integer_value << ANSI_RESET;
    if(has_attributes(node)) {
        sout << " {";
        if(description.has(node)) sout << ANSI_CYAN << " /* " << description(node) << " */" << ANSI_RESET;
        if(type.has(node)) sout << " type: " << ANSI_BLUE << ANSI_BOLD << type(node) << ANSI_RESET;
        if(condition.has(node)) sout << " condition: " << ANSI_RED << ANSI_BOLD << condition(node) << ANSI_RESET;
        if(name.has(node)) sout << " name: " << ANSI_GREEN << ANSI_BOLD << name(node) << ANSI_RESET;
        if(method_descriptor.has(node) && method_descriptor(node) != nullptr) sout << " method: " << ANSI_BLUE << *method_descriptor(node) << ANSI_RESET;
        if(message_descriptor.has(node) && message_descriptor(node) != nullptr) sout << " message: " << ANSI_MAGENTA << *message_descriptor(node) << ANSI_RESET;
        if(input_descriptor.has(node)) sout << " input: " << ANSI_MAGENTA << *input_descriptor(node) << ANSI_RESET;
        if(field_descriptor.has(node)) {
            sout << " field: " << ANSI_GREEN << *field_descriptor(node);
            if(field_descriptor(node)->is_repeated()) sout << "*";
            sout << ANSI_RESET;
        }
        if(enum_descriptor.has(node)) sout << " enum: " << ANSI_RED << *enum_descriptor(node) << ANSI_RESET;
        if(dimension.has(node)) sout << " dim: " << ANSI_CYAN << ANSI_BOLD << dimension(node) << ANSI_RESET;
        sout << " }";
    }
    sout << "\n";
    for(auto child: n.children) 
        print_ast(sout, child, indent+4);
    return indent; 
}

bool check_bexp_op_priority(int op1, int op2);

void flow_ast::to_text_r(std::ostream &out, int bexp, int op) const {
    auto const &bx = at(bexp);
    bool need_parens = false;
    switch(bx.type) {
        case FTK_bexp:
            switch(bx.children.size()) {
                case 1:
                    to_text_r(out, bx.children[0], op);
                    break;
                case 2:
                    need_parens = check_bexp_op_priority(op, at(bx.children[0]).type);
                    if(need_parens) out << "(";
                    out << " " << node_name(at(bx.children[0]).type);
                    to_text_r(out, bx.children[1], at(bx.children[0]).type);
                    if(need_parens) out << ")";
                    break;
                case 3:
                    need_parens = check_bexp_op_priority(op, at(bx.children[1]).type);
                    if(need_parens) out << "(";
                    to_text_r(out, bx.children[0], at(bx.children[1]).type);
                    out << " " << node_name(at(bx.children[1]).type) << " ";
                    to_text_r(out, bx.children[2], at(bx.children[1]).type);
                    if(need_parens) out << ")";
                    break;
            }
            break;
        case FTK_fldx:
            out << get_id(bx.children[0]) + "@" + get_joined_id(bexp, 1, ".");
            break;
        case FTK_INTEGER:
            out << get_value(bexp);
            break;
        case FTK_FLOAT:
            out << get_value(bexp);
            break;
        case FTK_STRING:
            out << c_escape(get_string(bexp));
            break;
        case FTK_ID:
            out << get_id(bexp);
            break;
        case FTK_dtid:
            out << get_dotted_id(bexp);
            break;
    }
}
std::string flow_ast::to_text(int node) const {
    std::ostringstream out;
    to_text_r(out, node, 0);
    return out.str();
}
std::string flow_ast::get_full_typename(int node) const {
    int t = at(node).type;
    switch(t) {
        case FTK_INTEGER:
        case FTK_STRING:
        case FTK_FLOAT:
            return node_name(t);
        case FTK_dtid:
            return get_full_name(enum_descriptor(node));
        case FTK_fldx:
            return get_full_name(message_descriptor(node));
        default:
            return "";
    }
}


