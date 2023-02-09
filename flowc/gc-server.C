#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include <sys/stat.h>

#include "cot.H"
#include "flow-compiler.H"
#include "grpc-helpers.H"
#include "strsli.H"
#include "stru1.H"
#include "vex.H"

using namespace stru1;
#define OUT indenter

static 
int check_enum(EnumDescriptor const *ledp, int value, std::string *symname=nullptr) {
    for(int i = 0, c = ledp->value_count(); i != c; ++i) {
        auto *evdp = ledp->value(i);
        //std::cout << "Checking " << evdp->name() << " ["<< evdp->full_name() << "]: " << evdp->number() << " ==> " << get_full_name(evdp) << "\n";
        if(evdp->number() == value) {
            if(symname) *symname = get_full_name(evdp);
            return 0;
        }
    }
    return 1;
}
static 
bool check_enum_includes(EnumDescriptor const *ledp, int value) {
    for(int i = 0, c = ledp->value_count(); i != c; ++i) {
        auto *evdp = ledp->value(i);
        if(evdp->number() == value) 
            return true;
    }
    return false;
}
static 
bool check_enum_includes(EnumDescriptor const *ledp, EnumDescriptor const *redp) {
    for(int i = 0, c = redp->value_count(); i != c; ++i) {
        auto *evdp = redp->value(i);
        if(!check_enum_includes(ledp, evdp->number()))
            return false;
    }
    return true;
}
/***
 * Get the field descriptor for this message.
 * The field is specified by a + separated string.
 */
static 
FieldDescriptor const *fd_accessor(std::string const &field, Descriptor const *d) {
    std::string base, fields;
    if(split(&base, &fields, field, "+") == 1)
        return nullptr;
    while(split(&base, &fields, fields, "+") == 2) {
        FieldDescriptor const *fd = d->FindFieldByName(base);
        d = fd->message_type();
    }
    return d->FindFieldByName(base);
}
/***
 * Convert an enum to the type of field (left_fd)
 * Returns false if the conversion failed
 */
static 
bool convert_enum(std::string &out, FieldDescriptor const *left_fd, std::string const &value, EnumDescriptor const *ed, EnumValueDescriptor const *vd=nullptr) {
    auto pb_type = left_fd->type();

    switch(pb_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            // Enum has an underlying int32 type so the conversion is to numeric type is just a cast
            out = sfmt() << "((int32_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            out = sfmt() << "std::to_string((int32_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            if(left_fd->enum_type() == ed) {
                // the enum types match -- no conversion is needed
                out = value;
            } else {
                // check if have a known value
                // check if ed is included in fd
                if((vd != nullptr && check_enum_includes(left_fd->enum_type(), vd->number())) || check_enum_includes(left_fd->enum_type(), ed)) 
                    out = sfmt() <<  get_full_name(left_fd->enum_type()) + "((int32_t)" << value << ")";
                else 
                    return false;
            }
            break;
        default:
            return false;
    }
    return true;
}
/***
 * Convert an integer to the type of field (left_fd)
 * Returns false if the conversion failed
 */
static 
bool convert_integer(std::string &out, FieldDescriptor const *left_fd, std::string const &value, FieldDescriptor const *right_fd, int v) {
    auto pb_type = left_fd->type();
    out = value;
    if(right_fd != nullptr && pb_type == right_fd->type()) 
        return true;
    
    if(right_fd == nullptr) switch(pb_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            out = value;
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            out = sfmt() << "\"" << value << "\""; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
                // check if have a known value
                if(right_fd == nullptr && check_enum_includes(left_fd->enum_type(), v)) 
                    out = sfmt() <<  get_full_name(left_fd->enum_type()) + "(" << value << ")";
                else 
                    return false;
            break;
        default:
            return false;
    } else switch(pb_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            out = sfmt() << "((double) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            out = sfmt() << "((float) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
            out = sfmt() << "((int64_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
            out = sfmt() << "((int32_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            out = sfmt() << "((uint32_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            out = sfmt() << "((uint64_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            out = value;
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            out = sfmt() << "std::to_string(" << value << ")"; 
            break;
        default:
            return false;
    }
    return true;
}
/***
 * Convert a float to the type of field (left_fd)
 * Returns false if the conversion failed
 */
static 
bool convert_float(std::string &out, FieldDescriptor const *left_fd, std::string const &value, FieldDescriptor const *right_fd) {
    auto pb_type = left_fd->type();
    out = value;
    if(right_fd != nullptr && pb_type == right_fd->type()) 
        return true;
    
    if(right_fd == nullptr) switch(pb_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            out = value;
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            out = sfmt() << "\"" << value << "\""; 
            break;
        default:
            return false;
    } else switch(pb_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            out = sfmt() << "((double) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            out = sfmt() << "((float) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
            out = sfmt() << "((int64_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
            out = sfmt() << "((int32_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            out = sfmt() << "((uint32_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            out = sfmt() << "((uint64_t) " << value << ")"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            out = value;
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            out = sfmt() << "std::to_string(" << value << ")"; 
            break;
        default:
            return false;
    }
    return true;
}
/***
 * Convert a string to the type of field (left_fd)
 * Returns false if the conversion failed
 */
static 
bool convert_string(std::string &out, FieldDescriptor const *left_fd, std::string const &value, FieldDescriptor const *right_fd, std::string const &raw) {
    auto pb_type = left_fd->type();
    out = value;
    if(right_fd != nullptr && pb_type == right_fd->type()) 
        return true;
    
    if(right_fd == nullptr) switch(pb_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            out = sfmt() << strtod(raw.c_str(), nullptr);
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
            out = sfmt() << strtoll(raw.c_str(), nullptr, 10);
            break;

        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            out = sfmt() << strtoull(raw.c_str(), nullptr, 10);
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            out = sfmt() << (strtoll(raw.c_str(), nullptr, 10) != 0);
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            out = value;
            break;
        default:
            return false;
    } else switch(pb_type) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            out = sfmt() << "strtod(" << value << ".c_str(), nullptr)"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            out = sfmt() << "strtof(" << value << ".c_str(), nullptr)"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
            out = sfmt() << "strtoll(" << value << ".c_str(), nullptr, 10)"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
            out = sfmt() << "strtol(" << value << ".c_str(), nullptr, 10)"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            out = sfmt() << "strtoul(" << value << ".c_str(), nullptr, 10)"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            out = sfmt() << "strtoull(" << value << ".c_str(), nullptr, 10)"; 
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            out = sfmt() << "(strtol(" << value << ".c_str(), nullptr, 10) != 0)"; 
            break;
        default:
            return false;
    }
    return true;
}
/*
 * Generate the conversion strings for C++ assignment. 
 * If the left value is of type enum, put the descriptor in ledp.
 */
static 
bool convert_value(EnumDescriptor const *&ledp, std::pair<std::string, std::string> &cc, FieldDescriptor const *left_fd, int right_ftk_type, bool right_literal, EnumDescriptor const *redp=nullptr) {
    ledp = nullptr;
    auto lt = left_fd->type();
    auto ftk_lt = grpc_type_to_ftk(lt);
    if(ftk_lt == right_ftk_type) {
        // No conversion necessary 
        if(ledp != redp)
            return false;
        cc.first.clear(); cc.second.clear();
        return true;
    }
    if(ftk_lt == FTK_STRING) {
        if(right_literal) {
            cc.first = "\""; cc.second = "\"";
        } else {
            cc.first = "std::to_string("; cc.second = ")";
        }
        return true;
    }
    if(right_ftk_type == FTK_STRING) switch(lt) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            cc.first = "strtod("; cc.second = right_literal? ", nullptr)": ".c_str(), nullptr)";
            return true;
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            cc.first = "strtof("; cc.second = right_literal? ", nullptr)": ".c_str(), nullptr)";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
            cc.first = "strtoll("; cc.second = right_literal? ", nullptr, 10)": ".c_str(), nullptr, 10)";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            cc.first = "strtol("; cc.second = right_literal? ", nullptr, 10)": ".c_str(), nullptr, 10)";
            ledp = left_fd->enum_type();
            cc.first += get_full_name(ledp) + "((int32_t)"; cc.second += ")";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
            cc.first = "strtol("; cc.second = right_literal? ", nullptr, 10)": ".c_str(), nullptr, 10)";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            cc.first = "strtoul("; cc.second = right_literal? ", nullptr, 10)": ".c_str(), nullptr, 10)";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            cc.first = "strtoull("; cc.second = right_literal? ", nullptr, 10)": ".c_str(), nullptr, 10)";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            cc.first = "strtobool("; cc.second = right_literal? ", nullptr)": ".c_str(), nullptr)";
            return true;

        default:
            cc.first.clear(); cc.second.clear();
            return false;
    }
    if(right_ftk_type == FTK_enum) switch(lt) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            cc.first = "(int32_t)"; cc.second = "";
            return true;

        default:
            cc.first.clear(); cc.second.clear();
            return false;
    }
    // Both sides are numeric
    switch(lt) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            cc.first = "(double)"; cc.second = "";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            cc.first = "(float)"; cc.second = "";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
            cc.first = "(int64_t)"; cc.second = "";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            ledp = left_fd->enum_type();
            cc.first = get_full_name(ledp) + "((int32_t)"; cc.second = ")";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
            cc.first = "(int32_t)"; cc.second = "";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            cc.first = "(uint32_t)"; cc.second = "";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            cc.first = "(uint64_t)"; cc.second = "";
            return true;

        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            cc.first = "!!("; cc.second = ")";
            return true;

        default:
            cc.first.clear(); cc.second.clear();
            return false;
    }
    return true;
}
enum accessor_type {
    LEFT_VALUE, RIGHT_VALUE, SIZE
};

std::ostream &operator <<(std::ostream &out, accessor_type at) {
    switch(at) {
        case LEFT_VALUE:
            return out << "LEFT-VALUE";
        case RIGHT_VALUE:
            return out << "RIGHT-VALUE";
        case SIZE:
            return out << "SIZE";
    }
    return out;
}

static std::set<std::string> reserved_cc = {
"and",
"and_eq",
"asm",
"auto",
"bitand",
"bitor",
"bool",
"break",
"case",
"catch",
"char",
"class",
"compl",
"const",
"const_cast",
"continue",
"default",
"delete",
"do",
"double",
"dynamic_cast",
"else",
"enum",
"explicit",
"export",
"extern",
"false",
"float",
"for",
"friend",
"goto",
"if",
"inline",
"int",
"long",
"mutable",
"namespace",
"new",
"not",
"not_eq",
"operator",
"or",
"or_eq",
"private",
"protected",
"public",
"register",
"reinterpret_cast",
"return",
"short",
"signed",
"sizeof",
"static",
"static_cast",
"struct",
"switch",
"template",
"this",
"throw",
"true",
"try",
"typedef",
"typeid",
"typename",
"union",
"unsigned",
"using",
"virtual",
"void",
"volatile",
"wchar_t",
"while",
"xor",
"xor_eq"
};

static std::string base_name(std::string const &name) {
    std::string n = to_lower(name);
    if(cot::contains(reserved_cc, n)) return n + "_";
    return n;
}

struct accessor_info {
    std::map<std::string, int> rs_dims;
    std::vector<std::set<std::string>> loop_sizes;

    accessor_info() {
    }
    void add_rs(std::string const &name, int dim) { 
        rs_dims[name] = dim; 
    }
    int loop_level() const {
        return (int) loop_sizes.size();
    }
    std::string loop_iter_name(int loop_level) const {
        return sfmt() << "I" << loop_level;
    }
    std::string loop_iter_name() const {
        return loop_iter_name(loop_level());
    }
    std::string loop_end_name(int loop_level) const {
        return sfmt() << "IE" << loop_level;
    }
    std::string loop_end_name() const {
        return loop_end_name(loop_level());
    }
    int incr_loop_level() {
        loop_sizes.emplace_back(std::set<std::string>());
        return loop_level();
    }
    int decr_loop_level() {
        if(loop_level() > 0) loop_sizes.pop_back();
        return loop_level();
    }
};

inline static 
std::ostream &operator<<(std::ostream &out, accessor_info const &ai) {
    return out << "<" << ai.rs_dims << ", " << ai.loop_sizes << ">";
}
std::string const cpp_index_prefix = "I";
static std::string cpp_left_stem(std::string const &name) {
    if(name.find_first_of('.') == std::string::npos) 
        return name+".";
    return ".";
}
static 
std::string cpp_var(std::vector<std::set<std::string>> const &loop_c, std::string const &pseu_name, int node_dim, accessor_type at, std::string *stem = nullptr) {
    if(pseu_name.length() <= 1) return pseu_name;
    std::string name = pseu_name;
    std::string var = std::string(node_dim, 'v');
    std::string fname;
    int ic = 0, fc = 0;
    unsigned fi = 0, lx = 0;  
    for(auto cp = name.begin(), ce = name.end(); cp != ce; ++cp) switch(*cp) {
        case ':':
            var += sfmt() << fname << "[" << cpp_index_prefix << ++ic << "]";
            fname = "";
            break;
        case '+':
            if(at == LEFT_VALUE && cp+1 != ce)
                var += sfmt() << "mutable_" << base_name(fname) << "()-";
            else if(at == LEFT_VALUE && cp+1 == ce)
                var += sfmt() << "set_" << base_name(fname) << "(";
            else
                var += sfmt() << base_name(fname) << "()";
            fname = "";
            break;
        case '*':
            ++ic;
            if(at == LEFT_VALUE && cp+1 == ce) {
                var += sfmt() << "add_" << base_name(fname) << "(";
                
            } else {
                var += base_name(fname);
                fi = 0;
                for(unsigned l = loop_c.size(); l != 0; --l) {
                    for(std::string stem: loop_c[l-1])
                        if(stem == var) {
                            fi = l; break;
                        }
                    if(fi) break;
                }
                var += sfmt() << "(" << cpp_index_prefix << (fi? fi: ic) << ")";
                lx = var.length();
            }
            fname = "";
            break;
        case '.':
            ++fc;
            var += fname;
            if(var.back() == '-')
                var += ">";
            else 
                var += ".";
            fname = "";
            break;
        default:
            fname += *cp;
    }
    if(!fname.empty()) var += fc == 0? fname: base_name(fname);
    switch(at) {
        case SIZE:
            if(stem != nullptr) *stem = var;
            if(fc == 0)
                var += ".size()";
            else
                var = sfmt() << "(unsigned long)" << var << "_size()";
            break;
        case RIGHT_VALUE:
            break;
        case LEFT_VALUE:
            var = var.substr(lx);
            break;
        default:
            break;
    }
    return var;
}
static std::string node_variable_name(std::string const &label, std::string const &node_name) {
    return sfmt() << label << "_" << node_name;
}
static std::string stage_variable_name(std::string const &label, int stage_num) {
    return sfmt() << label << "_" << stage_num;
}
#define NODE_VN2(label, node_name) node_variable_name((label), (node_name))
#define STAGE_VN(label) stage_variable_name((label), cur_stage)
/**
 * Local variable labels -- stage level
 */
#define L_STATUS        STAGE_VN("Status")
#define L_CONTEXT       STAGE_VN("Context")
#define L_STAGE_CALLS   STAGE_VN("Call_Count")
#define L_STAGE_START   STAGE_VN("Start_Time")
#define L_QUEUE         STAGE_VN("Queue")
#define L_RECV          STAGE_VN("Received_Count")
/**
 * Local variable labels -- node level
 */
#define LN_CARR(n)      NODE_VN2("CARR", (n))
#define L_CARR          LN_CARR(cur_node_name)
#define LN_CONN(n)      NODE_VN2("ConN", (n))
#define L_CONN          LN_CONN(cur_node_name)
#define LN_INPTR(n)     NODE_VN2("In_Ptr", (n))
#define L_INPTR         LN_INPTR(cur_node_name)

#define LN_BEGIN(n)     NODE_VN2("Base_X", (n))
#define L_BEGIN         LN_BEGIN(cur_node_name)
#define LN_END_X(n)     NODE_VN2("End_X", (n))
#define L_END_X         LN_END_X(cur_node_name)
#define LN_SENT(n)      NODE_VN2("Sent", (n))
#define L_SENT          LN_SENT(cur_node_name)

#define LN_OUTPTR(n)    NODE_VN2("Out_Ptr", (n))
#define L_OUTPTR        LN_OUTPTR(cur_node_name)

#define L_VISITED       NODE_VN2("Visited", type(cur_node))

// Generate C++ code for a given Entry Method
int flow_compiler::gc_server_method(std::ostream &os, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry) {
    indented_stream indenter(os, 1);
    auto eipp = entry_ip.find(blck_entry);
    OUT << "//  from " << main_file << ":" << at(blck_entry).token.line << " " << entry_dot_name << "\n";
    if(eipp == entry_ip.end()) {
        OUT  << "/**  no code gnerated */\n";
        return 1;
    }
    std::string no_node_id = "flowc::n_" + get(global_vars, "NO_NODE_NAME", 0);
    // Temporary field reference stack
    std::vector<std::string> cur_loop_tmp; cur_loop_tmp.push_back("");
    std::vector<std::set<std::string>> loop_c;
    std::string rvl, lvl;   // Left and right value expressions
    std::vector<std::pair<std::string, int>> tvl; // Values stack of <value, priority>
    // Field length values stack -- 
    //      for each subexpression there is a set of lengths that need to be considered
    //      in the case of dimension resuction we keep all lower dimensions in order
    std::vector<std::set<std::vector<std::string>>> flvs; 
    // 
    int cur_stage = 0, cur_node = 0, node_dim = 0, stage_nodes = 0;
    std::string cur_stage_name, cur_input_name, cur_output_name, cur_node_name;
    std::string input_name, output_name;
    // dimension for every node response and current loop sizes
    accessor_info acinf;
    std::vector<int> stage_node_ids;
    std::map<std::string, std::string> nodes_rv;
    bool first_node = false;        // whether the current node is the first in an alias set
    bool node_has_calls = false;    // whether this node makes grpc calls
    bool stage_has_calls = false;   // whether this stage has any node with calls
    bool first_with_output = false; // whether this node is the first in an alias set that has output
    bool node_cg_done = false;      // done generating code for the node
    bool is_node = false;           // whether this is node or an error check loop/point
    int alternate_nodes = 0;        // count of alternate nodes 
    EnumDescriptor const *ledp, *redp;   // left and right enum descriptor needed for conversion check
    int error_count = 0;
    int dacc_level = 0;
    int tacc_count = 0;

    std::string loop_chk, loop_end;
   
    for(int i = eipp->second, e = icode.size(), done = 0; i != e && !done; ++i) {
        fop const &op = icode[i];
        // std::cerr << "// " << i+1 << " " << op << "\n";
        switch(op.code) {
            case MTHD:
                input_name = op.arg1;
                nodes_rv[input_label] = input_name;
                output_name = op.arg2;
                OUT << "::grpc::Status " << get_name(op.m1) << "(flowc::call_info const &CIF, ::grpc::ServerContext *CTX, " << get_full_name(op.d1) << " const *p" << input_name << ", " << get_full_name(op.d2) << " *p" << output_name << ") {\n";
                ++indenter;
                OUT << "GRPC_ENTER_" << entry_name << "(\"" << entry_dot_name << "\", CIF, *CTX, p" << input_name << ")\n";
                OUT << get_full_name(op.d1) << " const &" << input_name << " = *p" << input_name << ";\n";
                OUT << get_full_name(op.d2) << " &" << output_name << " = *p" << output_name << ";\n";
                OUT << "::grpc::Status L_status = ::grpc::Status::OK;\n";
                OUT << "auto ST = std::chrono::steady_clock::now();\n";
                OUT << "int Total_calls = 0;\n";

                OUT << "FLOGC(CIF.trace_call) << CIF << \"enter " << entry_dot_name << "/\" << (CIF.async_calls? \"a\": \"\") << \"synchronous calls \" << flowc::log_abridge(" << input_name << ") << \"\\n\";\n";
                OUT << "\n"; 
                break;
            case END:
                OUT << "PRINT_TIME(CIF, 0, \"total\", ST - ST, std::chrono::steady_clock::now() - ST, Total_calls);\n";
                OUT << "GRPC_LEAVE_" << entry_name << "(\"" << entry_dot_name << "\", CIF, L_status, *CTX, &" << output_name << ")\n"; 
                OUT << "FLOGC(CIF.trace_call) << CIF << \"leave " << entry_dot_name << ": \" << flowc::log_abridge(" << output_name << ") << \"\\n\";\n";

                OUT << "return L_status;\n";
                --indenter; 
                OUT << "}\n";
                done = 1;
                break;
            case BSTG:
                cur_stage = op.arg[0];
                stage_nodes = op.arg[1];
                cur_stage_name = op.arg1;
                stage_has_calls = false;

                OUT << "/*\n";
                OUT << " * stage: " << cur_stage << "\n";
                OUT << " * stage nodes: " << stage_nodes << "\n";
                OUT << " * stage name: " << cur_stage_name << "\n";
                for(unsigned i = 3, e = op.arg.size(); i < e; ++i) {
                    //OUT << " * stage node: " << op.arg[i] << "\n";
                    stage_has_calls = stage_has_calls || method_descriptor(op.arg[i]) != nullptr;
                }
                OUT << " * stage has calls: " << (stage_has_calls? "yes": "no")  << "\n";
                OUT << " */\n";

                OUT << "auto " << L_STAGE_START << " = std::chrono::steady_clock::now();\n";
                if(stage_has_calls) {
                    OUT << "int " << L_STAGE_CALLS << " = 0;\n";
                    // The completion queue is shared between all the nodes in a stage
                    OUT << "::grpc::CompletionQueue " << L_QUEUE << ";\n";
                    // All the Status and ClientContext objects for the stage are kept in one vector
                    // Each node has a range of indices BX_xxxx and EX_xxxx 
                    // Additionally the number of calls currently sent (i.e. active, in the queue) is kept in SENT_xxxx
                    OUT << "std::vector<std::unique_ptr<grpc::ClientContext>> " << L_CONTEXT << ";\n";
                    OUT << "std::vector<grpc::Status> " << L_STATUS << ";\n";
                }
                stage_node_ids.clear();
                break;
            case ESTG:
                if(stage_has_calls) {
                    OUT << "if(CIF.async_calls && 0 < " << L_STAGE_CALLS << ") {\n";
                    ++indenter;
                    OUT << "bool abort_stage = false;\n";
                    OUT << "::grpc::Status abort_status;\n";
                    for(auto nnj: stage_node_ids) if(method_descriptor(nnj) != nullptr) {
                        std::string nn(to_lower(to_identifier(name(nnj))));

                            OUT << "auto " << nn << "_maxcc = (int)" << nn << "_ConP->count();\n";
                            OUT << "if(!abort_stage && " << LN_END_X(nn) <<  " > " << LN_BEGIN(nn) << " && " << nn << "_maxcc == 0) {\n";
                            ++indenter;
                            OUT << "abort_status = ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, ::flowc::sfmt() << \"No addresses found for " << nn << ": \" << flowc::ns_" << nn << ".endpoint  << \"\\n\");\n";
                            OUT << "abort_stage = true;\n";
                            --indenter;
                            OUT << "}\n";
                            // The calls for each node are from BX_xxxx to EX_xxxx but we only keep xxxx_maxcc active at a time
                            // We use the (one based) index in the status/context vector as a tag for the call
                            OUT << "if(!abort_stage) for(int Ax = " << LN_BEGIN(nn) << ", Ex = std::min(" << LN_END_X(nn) << ", " << LN_BEGIN(nn) << " + " << nn << "_maxcc); Ax < Ex; ++Ax) {\n";
                            ++indenter;
                            OUT << "auto Rx = Ax-" << LN_BEGIN(nn)  << ";\n";
                            OUT << "auto &RPCx = " << LN_CARR(nn) << "[Rx];\n";
                            OUT << "RPCx = " << nn << "_prep(" << LN_CONN(nn) << "[Rx], CIF, Ax+1," << nn << "_ConP, " << L_QUEUE << ", *" << L_CONTEXT << "[Ax], " << LN_INPTR(nn)  << "[Rx]);\n";
                            OUT << "RPCx->StartCall();\n";
                            OUT << "RPCx->Finish(" << LN_OUTPTR(nn) << "[Rx], &" << L_STATUS << "[Ax], (void *) (long) (Ax+1));\n";
                            OUT << "++" << LN_SENT(nn) << ";\n";
                            --indenter;
                            OUT << "}\n";
                    }
                    OUT << "void *TAG; bool NextOK = false; int " << L_RECV << " = 0;\n";
                    OUT << "//\n";
                    OUT << "FLOGC(!abort_stage && CIF.trace_call) << CIF << \"begin waiting for \" << " << L_STAGE_CALLS << " << \" in " << entry_dot_name << " stage " << cur_stage << " (" << cur_stage_name << ")\\n\";\n";
                    OUT << "while(!abort_stage && " << L_RECV << " < " << L_STAGE_CALLS << ") {\n";
                    ++indenter;
                    OUT << "auto ns = " << L_QUEUE << ".AsyncNext(&TAG, &NextOK, CIF.deadline);\n";
                    OUT << "if(ns != ::grpc::CompletionQueue::NextStatus::GOT_EVENT || !NextOK || CTX->IsCancelled()) {\n";
                    ++indenter;
                    OUT << "abort_stage = true;\n";
                    OUT << "if(ns == ::grpc::CompletionQueue::NextStatus::TIMEOUT || CTX->IsCancelled()) {\n";
                    ++indenter;
                    OUT << "abort_status = ::grpc::Status(::grpc::StatusCode::CANCELLED, \"Call exceeded deadline or was cancelled by the client\");\n";
                    OUT << "FLOG << \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << "): call cancelled\\n\";\n";
                    --indenter;
                    OUT << "} else {\n";
                    ++indenter;
                    OUT << "abort_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, \"Cannot complete RPC call\");\n";
                    OUT << "FLOG << \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << "): NextOK: \" << NextOK << \"\\n\";\n";
                    --indenter;
                    OUT << "}\n";
                    OUT << "break;\n";

                    --indenter;
                    OUT << "}\n";
                    OUT << "int X = (int) (long) TAG;\n";
                    OUT << "FLOGC(CIF.trace_call) << std::make_tuple(&CIF, X) << \"woke up in " << entry_dot_name << " stage " << cur_stage << " (" << cur_stage_name << ")\\n\";\n";
                    OUT << "auto &LL_Status = " << L_STATUS << "[X-1];\n";
                    OUT << "auto &LL_Ctx = *" << L_CONTEXT << "[X-1];\n";
                    OUT << "auto &LL_Ctxup = " << L_CONTEXT << "[X-1];\n";
                    int nc = 0;
                    int call_nodes = 0;
                    for(auto nni: stage_node_ids) if(method_descriptor(nni) != nullptr) ++call_nodes;
                    for(auto nni: stage_node_ids) if(method_descriptor(nni) != nullptr) {
                        std::string nn(to_lower(to_identifier(name(nni))));
                        // Find out what node this index belongs to by comparing with the EX_xxxx markers
                        if(nc > 0) OUT << "else ";
                        if(++nc == call_nodes)
                            indenter << "{\n";
                        else 
                            indenter << "if(X <= " << LN_END_X(nn) << ") {\n";
                        ++indenter;
                        // The index in the node vectors (result and result reader)
                        OUT << "int NRX = X - " << LN_BEGIN(nn) << ";\n";
                        // Mark this connection as finished
                        OUT << nn << "_ConP->finished(" << LN_CONN(nn) << "[NRX-1], CIF, X, LL_Status.error_code() == ::grpc::StatusCode::UNAVAILABLE);\n";
                        OUT << "if(" << LN_SENT(nn) << " != " << LN_END_X(nn) <<  " - " << LN_BEGIN(nn) << ") {\n";
                        ++indenter;
                        // If there are still requests to be sent, add a new one, of the same kind, to the queue
                        OUT << "int Nx = " << LN_SENT(nn) << ";\n";
                        OUT << "int Sx = Nx + " << LN_BEGIN(nn) << ";\n";

                        OUT << "auto &RPCx = " << LN_CARR(nn) << "[Nx];\n";
                        OUT << "RPCx = " << nn << "_prep(" << LN_CONN(nn) << "[Nx], CIF, Sx+1," << nn << "_ConP, " << L_QUEUE << ", *" << L_CONTEXT << "[Sx], " << LN_INPTR(nn)  << "[Nx]);\n";
                        OUT << "RPCx->StartCall();\n";
                        OUT << "RPCx->Finish(" << LN_OUTPTR(nn) << "[Nx], &" << L_STATUS << "[Sx], (void *) (long) (Sx+1));\n";
                        OUT << "++" << LN_SENT(nn) << ";\n";
                        --indenter;
                        OUT << "}\n";
                        if(method_descriptor(nni) != nullptr)
                            OUT << "GRPC_RECEIVED(\"" << nn << "\", CIF, NRX, flowc::cn_" << to_upper(to_identifier(nn)) << ", LL_Status, LL_Ctx, " << LN_OUTPTR(nn) << "[NRX-1])\n";
                        OUT << "FLOGC(CIF.trace_call && LL_Status.ok()) << std::make_tuple(&CIF, X) << \"" << nn << " response: \" << flowc::log_abridge(*" << LN_OUTPTR(nn) << "[NRX-1]) << \"\\n\";\n";

                        OUT << "if(!LL_Status.ok()) {\n";
                        ++indenter;
                        OUT << "GRPC_ERROR(CIF, X, \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << ") " << nn << "\", LL_Status, LL_Ctx);\n";
                        OUT << "abort_stage = true;\n";
                        OUT << "abort_status = LL_Status;\n";
                        OUT << "break;\n";
                        --indenter;
                        OUT << "}\n";
                        OUT << "// LL_Ctxup.reset(nullptr);\n";
                        --indenter;
                        OUT << "}\n";
                    }
                    OUT << "if(++" << L_RECV << " < " << L_STAGE_CALLS << ") {\n";
                    ++indenter;
                    OUT << "FLOGC(CIF.trace_call) << CIF << \"back waiting for \" << (" << L_STAGE_CALLS << " - " << L_RECV << ") << \" in " << entry_dot_name << " stage " << cur_stage << " (" << cur_stage_name << ")\\n\";\n";
                    --indenter;
                    OUT << "}\n";

                    --indenter;
                    OUT << "}\n";
                    OUT << "flowc::closeq(" << L_QUEUE << ");\n";
                    OUT << "if(abort_stage) {\n";
                    ++indenter;

                    for(auto nnj: stage_node_ids) {
                        std::string nn(to_lower(to_identifier(name(nnj))));
                        OUT << nn << "_ConP->release(CIF, " << LN_CONN(nn) << ".begin(), " << LN_CONN(nn) << ".end());\n";
                    }
                    OUT << "return abort_status;\n";
                    --indenter;
                    OUT << "}\n";

                    OUT << L_CONTEXT << ".clear();\n";
                    OUT << L_STATUS << ".clear();\n";
                    --indenter;
                    OUT << "}\n";
                    OUT << "Total_calls += " << L_STAGE_CALLS << ";\n";
                    OUT << "PRINT_TIME(CIF, "<< cur_stage << ", \""<< cur_stage_name << "\", "<<L_STAGE_START<<" - ST, std::chrono::steady_clock::now() - "<< L_STAGE_START <<", "<< L_STAGE_CALLS<<");\n";
                } else {
                    OUT << "PRINT_TIME(CIF, "<< cur_stage << ", \""<< cur_stage_name << "\", "<<L_STAGE_START<<" - ST, std::chrono::steady_clock::now() - "<< L_STAGE_START <<", 0);\n";
                }
                OUT << "\n";
                break;
            case BERC:
                is_node = false;
                node_dim = op.arg[0];
                cur_node_name = sfmt() << "Errck" << op.arg[3];
                cur_input_name = ""; 
                cur_output_name = "";
                node_has_calls = false;
                first_node = false;
                first_with_output = false;
                OUT << "/*\n";
                OUT << " * Error check " << op.arg[3] << "\n";
                OUT << " */\n";
                break;
            case BNOD:
                is_node = true;
                node_cg_done = false;
                node_dim = op.arg[0];
                cur_input_name = op.arg2; 
                cur_output_name = op.arg1;
                cur_node_name = to_lower(to_identifier(name(cur_node = op.arg[1])));
                node_has_calls = method_descriptor(cur_node) != nullptr;
                if(node_has_calls)
                    stage_node_ids.push_back(cur_node);

                first_node = op.arg[3] != 0;
                first_with_output = op.arg[4] != 0;
                alternate_nodes = op.arg[5];
                if(first_with_output) 
                    nodes_rv[name(cur_node)] = cur_output_name;

                OUT << "/*\n";
                OUT << " * stage: (" << cur_stage << ") " << cur_stage_name << "\n";
                OUT << " * node: " << cur_node_name << " (" << cur_node << "/" << name(cur_node) << ") " << (node_has_calls? "" : "no calls") << " dim " << node_dim << "  \n";
                OUT << " * input/output: " << cur_input_name << "/" << cur_output_name << "\n";
                if(alternate_nodes) {
                    OUT << " * is first: " << (first_node? "yes": "no") << "\n";
                    OUT << " * is first with output: " << (first_with_output? "yes": "no") << "\n";
                    OUT << " * alternate nodes: " << alternate_nodes << "\n";
                }
                OUT << " * node has calls: " << (node_has_calls? "yes": "no") << "\n";
                OUT << " */\n";
                // input is not needed for no-call nodes
                if(op.d2 != nullptr) 
                    OUT << reps("std::vector<", node_dim) << get_full_name(op.d2) << reps(">", node_dim)  << " " << reps("v", node_dim) << cur_input_name << ";\n";
                
                // output must be set even when the node makes no calls if this is a first node with output
                if(first_with_output) 
                    OUT << reps("std::vector<", node_dim) << get_full_name(op.d1) << reps(">", node_dim)  << " " << reps("v", node_dim) << cur_output_name << ";\n";
                
                if(first_node) 
                    OUT << reps("std::vector<", node_dim) << "flowc::nodes"                << reps(">", node_dim)  << " " << reps("v", node_dim) << L_VISITED << (node_dim == 0? (std::string(" = ") + no_node_id): std::string()) << ";\n";
                
                if(node_has_calls) {
                    OUT << "auto " << cur_node_name << "_ConP = " << cur_node_name << "_get_connector();\n";

                    // Each node has a vector of response readers, input message poiners and output message pointers
                    if(op.d1 != nullptr) {
                        OUT << "std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReader<" << get_full_name(op.d1) << ">>> " << L_CARR << ";\n";
                        OUT << "std::vector<int> " << L_CONN << ";\n";
                        OUT << "std::vector<" << get_full_name(op.d2) << " *> " << L_INPTR << ";\n";
                        OUT << "std::vector<" << get_full_name(op.d1) << " *> " << L_OUTPTR <<  ";\n";
                    }
                    OUT << "int " << L_BEGIN << " = " << L_STAGE_CALLS << ", " << L_SENT << " = 0;\n";
                }
                break;
            case BNL: { // node level loop begin
                std::string loop_size_varname = sfmt() << "NS_" << cur_node_name << "_" << loop_c.size();
                OUT << "auto " << loop_size_varname << " = " << loop_end << ";\n";

                if(!cur_input_name.empty()) 
                    OUT << reps("v", node_dim-acinf.loop_level()+1) << cur_input_name << ".resize("<< loop_size_varname << ");\n";
                if(first_with_output) 
                    OUT << reps("v", node_dim-acinf.loop_level()+1) << cur_output_name << ".resize("<< loop_size_varname << ");\n";
                if(first_node) 
                    OUT << reps("v", node_dim-acinf.loop_level()+1) << L_VISITED << ".resize("<< loop_size_varname << (node_dim-loop_c.size()==0? (std::string(", ") + no_node_id): std::string()) << ");\n";
                OUT << "for(unsigned " << cpp_index_prefix << loop_c.size() << " = 0; " << cpp_index_prefix << loop_c.size() << " < NS_" << cur_node_name << "_"  << loop_c.size() << "; ++" << cpp_index_prefix << loop_c.size() << ") {\n" << indent();
                if(!cur_input_name.empty())
                    OUT << "auto &" << std::string(node_dim-loop_c.size(), 'v') << cur_input_name << " = " << std::string(node_dim-loop_c.size()+1, 'v') << cur_input_name << "[" << cpp_index_prefix << loop_c.size() <<  "];\n";
                if(!cur_output_name.empty())
                    OUT << "auto &" << std::string(node_dim-loop_c.size(), 'v') << cur_output_name << " = " << std::string(node_dim-loop_c.size()+1, 'v') << cur_output_name << "[" << cpp_index_prefix << loop_c.size() << "];\n";
                if(is_node)
                    OUT << "auto &" << std::string(node_dim-loop_c.size(), 'v') << L_VISITED << " = " <<  std::string(node_dim-loop_c.size()+1, 'v') << L_VISITED << "[" << cpp_index_prefix << loop_c.size() << "];\n";

            } break;
            case IFNC:
                if(op.arg[1] == 0) {
                    OUT << "if(" << L_VISITED << " == " << no_node_id << ") {" << "\n";
                } else {
                    assert(tvl.size() > 0);
                    OUT << "if(" << L_VISITED << " == " << no_node_id << " && " << (tvl.back().second > 5? "(": "") << tvl.back().first << (tvl.back().second > 5? ")": "") << ") {" << "\n";
                    tvl.pop_back();
                }
                ++indenter;
                if(alternate_nodes)
                    OUT << "FLOGC(CIF.trace_call) << CIF << \"condition triggered for node " << cur_node_name << "\\n\";\n";
                break;
            case ERR:
                assert(tvl.size() > 1);
                OUT << "if(" << tvl[tvl.size()-2].first << ") {" << "\n";
                ++indenter;
                OUT << "std::string ERR_message(" << tvl.back().first << ");\n";
                OUT << "FLOG << \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << ") return error: \" << ERR_message << \"\\n\";\n";
                OUT << "return ::grpc::Status(::grpc::StatusCode::" << op.arg1 << ", ERR_message);\n";
                --indenter;
                OUT << "}\n";
                tvl.pop_back(); tvl.pop_back();
                break;
            case EERC:
                while(acinf.loop_level() > 0) {
                    acinf.decr_loop_level();
                    --indenter; 
                    OUT << "}\n";
                    cur_loop_tmp.pop_back();
                }
                for(int i = 0; i < op.arg[0]; ++i) 
                    loop_c.pop_back();
                cur_node = node_dim = 0; 
                break;
            case ENOD:
                // if visited
                if(!node_cg_done)
                    OUT <<  L_VISITED << " = flowc::n_" << to_upper(to_identifier(cur_node_name)) << ";\n";
                --indenter;
                OUT << "}\n";
                while(acinf.loop_level() > 0) {
                    acinf.decr_loop_level();
                    --indenter; 
                    OUT << "}\n";
                    cur_loop_tmp.pop_back();
                }
                for(int i = 0; i < op.arg[0]; ++i) 
                    loop_c.pop_back();
                if(node_has_calls)
                    OUT << "int " << L_END_X  << " = " << L_STAGE_CALLS << ";\n";
                acinf.add_rs(cur_output_name, node_dim);
                cur_node = node_dim = 0; cur_input_name.clear(); cur_output_name.clear();
                first_node = false;
                first_with_output = false;
                break;
            case EPRP:
                while(acinf.loop_level() > 0) {
                    acinf.decr_loop_level();
                    --indenter; 
                    OUT << "}\n";
                    cur_loop_tmp.pop_back();
                }
                acinf.add_rs(cur_output_name, node_dim);
                cur_node = node_dim = 0; cur_input_name.clear(); cur_output_name.clear();
                first_node = false;
                first_with_output = false;
                break;
            case BPRP:
                OUT << "// prepare the "<< op.d1->full_name() << " result for " << entry_dot_name << "\n";
                break;
            case LOOP:
                acinf.incr_loop_level();
                loop_c.push_back(std::set<std::string>());
                loop_end = "";
                break;
            case NLP: { // node level loop 
                acinf.incr_loop_level();
                cur_loop_tmp.push_back("");
                loop_c.push_back(std::set<std::string>());
                loop_end = "";
            } break;
            case LPC: {
                std::string stem; 
                if(loop_end.empty())
                    loop_end = cpp_var(loop_c, op.arg1, op.arg[0], SIZE, &stem);
                else 
                    loop_end = sfmt() << "std::min(" << loop_end << ", " << cpp_var(loop_c, op.arg1, op.arg[0], SIZE, &stem) << ")";
                loop_c.back().insert(stem);
            } break;
            case BLP:
                OUT << "for(unsigned " << cpp_index_prefix << loop_c.size() << " = 0, LS" << loop_c.size() << " = " << loop_end << "; " 
                    << cpp_index_prefix  << loop_c.size() << " < LS" << loop_c.size() << "; ++" << cpp_index_prefix << loop_c.size() << ") {\n" << indent();
                if(op.arg[0]) {
                    OUT << "auto &T" << loop_c.size() << " = *" <<  cur_loop_tmp.back() << cpp_var(loop_c, op.arg1, 0, LEFT_VALUE) << ");\n"; 
                    cur_loop_tmp.push_back(sfmt() << "T" << loop_c.size());
                } else {
                    cur_loop_tmp.push_back(cur_loop_tmp.back()); 
                }
                break;
            case ELP:
                loop_c.pop_back();
                cur_loop_tmp.pop_back();
                acinf.decr_loop_level();
                OUT << unindent() << "}\n";
                break;
            case FUNC: { // args: number of args, function's dimension, is inline 
                    assert(tvl.size() >= op.arg[0]);
                    std::string tmpvar = sfmt() << "TACC" << tacc_count;
                    rvl = sfmt() << "flowrt::" << op.arg1 << " " << (!op.arg[2]? tmpvar: std::string()) << "(";
                    for(int i = tvl.size() - op.arg[0]; i < tvl.size(); ++i) {
                        rvl += tvl[i].first; 
                        if(i + 1 != tvl.size()) rvl += ", ";
                    }
                    rvl += ")";
                    for(int i = 0; i < op.arg[0]; ++i)
                        tvl.pop_back();
                    if(!op.arg[2]) {
                        OUT << rvl << ";\n";
                        rvl = sfmt() << tmpvar << "[" << cpp_index_prefix << op.arg[1] << "]";
                        ++tacc_count;
                        // remove all the sizes used by the subexpressions and replace them with
                        // the size of this object
                        std::vector<std::string> ss; ss.push_back(sfmt() << tmpvar << ".size()");
                        if(flvs.size() > 0) {
                            flvs.back().clear();
                            flvs.back().insert(ss);
                        }
                    }
                    tvl.push_back(std::make_pair(rvl, 0));
                } break;
            case IOP: // arg[0]: number of operands, arg[1]: operator priority
                assert(tvl.size() >= op.arg[0]);
                switch(op.arg[0]) { 
                    case 1:
                        if(op.arg[1] < tvl.back().second)
                            rvl = sfmt() << op.arg1 << "(" << tvl.back().first << ")";
                        else
                            rvl = sfmt() << op.arg1 << tvl.back().first;
                        tvl.pop_back();
                        tvl.push_back(std::make_pair(rvl, op.arg[1]));
                        break;
                    case 2:
                        if(op.arg[1] < tvl[tvl.size()-2].second)
                            rvl = sfmt() << "(" << tvl[tvl.size()-2].first << ")";
                        else
                            rvl = tvl[tvl.size()-2].first;

                        rvl += sfmt() << " " << op.arg1 << " ";

                        if(op.arg[1] < tvl[tvl.size()-1].second)
                            rvl += sfmt() << "(" << tvl[tvl.size()-1].first << ")";
                        else
                            rvl += tvl[tvl.size()-1].first;

                        tvl.pop_back(); tvl.pop_back();
                        tvl.push_back(std::make_pair(rvl, op.arg[1]));
                        break;
                    case 3:
                        rvl = sfmt() << "((" << tvl[tvl.size()-3] << ")? " << tvl[tvl.size()-2] << ": " << tvl[tvl.size()-1] << ")";
                        tvl.pop_back(); tvl.pop_back(); tvl.pop_back();
                        tvl.push_back(std::make_pair(rvl, op.arg[1]));
                        break;
                }
                break;
            case COPY: 
                OUT << cur_loop_tmp.back() << cpp_left_stem(op.arg1) << "CopyFrom(" << tvl.back().first << ");\n";
                tvl.pop_back();
                break;

            case SETF:
                OUT << cur_loop_tmp.back() << cpp_var(loop_c, op.arg1, 0, LEFT_VALUE) << "" << tvl.back().first << ");\n";
                tvl.pop_back();
                break;

            case SETE:
                OUT << "// check for valid value in " << get_full_name(op.el) << "\n";
                OUT << "{\n" << indent();
                if(op.ext == FTK_STRING) {
                    OUT << "std::string TS(" << tvl.back().first << ");\n";
                    OUT << "auto TE = ::google::protobuf::GetEnumDescriptor<"<<get_full_name(op.el)<<">()->FindValueByName(TS);\n";
                } else {
                    OUT << "auto TI = " << tvl.back().first << ";\n";
                    OUT << "auto TE = ::google::protobuf::GetEnumDescriptor<"<<get_full_name(op.el)<<">()->FindValueByNumber(TI);\n";
                }
                OUT << "if(TE != nullptr) {\n" << indent();
                OUT << cur_loop_tmp.back() << cpp_var(loop_c, op.arg1, 0, LEFT_VALUE) << "static_cast<" << get_full_name(op.el) << ">(TE->number()));\n";
                OUT << unindent() << "} else {\n" << indent();
                if(op.ext == FTK_STRING) {
                    OUT << "std::string ERR_message(\"attempting to assign invalid value '\" + TS + \"' to field of type " << get_name(op.el) << "\");\n";
                } else {
                    OUT << "std::string ERR_message(\"attempting to assign invalid value \" + std::to_string(TI) + \" to field of type " << get_name(op.el) << "\");\n";
                }
                OUT << "FLOG << \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << ") return error: \" << ERR_message << \"\\n\";\n";
                OUT << "return ::grpc::Status(::grpc::StatusCode::OUT_OF_RANGE, ERR_message);\n";
                OUT << unindent() << "}\n";
                OUT << unindent() << "}\n";
                tvl.pop_back();
                break;

            case RVC: 
                if(op.ev1 != nullptr) rvl = get_full_name(op.ev1);
                else if(op.arg.size() > 1 && op.arg[1] == (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING) rvl =  sfmt() << "std::string(" << c_escape(op.arg1) << ")"; 
                else rvl = op.arg1;
                tvl.push_back(std::make_pair(rvl, 0));
                break;

            case RVV: 
                tvl.push_back(std::make_pair("flowdef::v"+op.arg1, 0));
                break;

            case SVF:
                rvl = cpp_var(loop_c, op.arg1, op.arg[0], SIZE);
                tvl.push_back(std::make_pair(rvl, 0));
                break;

            case RVF: 
                rvl = cpp_var(loop_c, op.arg1, op.arg[0], RIGHT_VALUE);
                tvl.push_back(std::make_pair(rvl, 0));
                break;

            case DACC: {
                    assert(flvs.size() > 0);
                    // args: dimension change of this aggregator, dimension of the right value
                    std::set<std::string> lxset;
                    std::set<std::vector<std::string>> saves;
                    for(auto &sv: flvs.back()) {
                        // keep the length expressions for dimensions after reduction
                        if(sv.size() > -op.arg[0]) {
			    auto pb = sv.begin();
			    auto pe = pb + (sv.size()+op.arg[0]);
                            saves.insert(std::vector<std::string>(pb, pe));
			}
                        lxset.insert(sv.back());
                    }
                    std::string lenx = lxset.size() == 1? *lxset.begin(): stru1::join(lxset.begin(), lxset.end(), ", ", ", ", "flowrt::vmin(", "", "", ")");
                    std::string tmpvar = sfmt() << "TSZ" << tacc_count;
                    ++tacc_count;
                    OUT << "auto " << tmpvar << " = " << lenx << ";\n";
                    rvl = sfmt() << tmpvar << ", [&](int " << cpp_index_prefix << op.arg[1] << ") -> auto {return " << tvl.back().first << ";}";
                    // Remove the marker from the stack
                    flvs.pop_back();
                    // If there is a marker in the stack, propagate the sizes we used
                    if(flvs.size() > 0) 
                        flvs.back().insert(saves.begin(), saves.end());
                    tvl.pop_back();
                    tvl.push_back(std::make_pair(rvl, 0));
                    --dacc_level;
                }
                break;
            case FLAT: {
                }
                break;
            case STOL: 
                // args: field's dimension, node's dimension, dimension change 
                {
                    std::vector<std::string> sizes;
                    for(auto vn: stru1::splitter(op.arg1, "#"))
                        sizes.push_back( cpp_var(loop_c, vn, op.arg[0], SIZE, nullptr) );
                    flvs.back().insert(sizes);
                }
                break;
            case CLLS: 
                // Create a new size set
                flvs.push_back(std::set<std::vector<std::string>>());
                ++dacc_level;
                break;
            case CALL:
                node_has_calls = true;
                OUT << "++" << L_STAGE_CALLS << ";\n";
                OUT << "if(CIF.async_calls) {\n" << indent(); 

                OUT << "if(" << cur_node_name << "_ConP->count() == 0) \n" << indent() <<
                    "return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, flowc::sfmt() << \"Failed to connect\");\n" << unindent();
                OUT << L_CONTEXT << ".emplace_back(std::unique_ptr<grpc::ClientContext>(new ::grpc::ClientContext));\n";
                OUT << L_STATUS << ".emplace_back(::grpc::Status());\n";
                OUT << L_OUTPTR << ".emplace_back(&" << cur_output_name << ");\n";
                OUT << L_INPTR << ".emplace_back(&" << cur_input_name << ");\n";
                OUT << L_CARR << ".emplace_back(nullptr);\n";
                OUT << L_CONN << ".push_back(-1);\n";

                OUT << unindent() << "} else {\n" << indent();
                OUT << "if(CTX->IsCancelled()) {\n" << indent();
                OUT << "FLOG << \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << "): call cancelled\\n\";\n";
                OUT << "return ::grpc::Status(::grpc::StatusCode::CANCELLED, \"Call exceeded deadline or was cancelled by the client\");\n";
                OUT << unindent() << "}\n";
                OUT << "L_status = " << cur_node_name << "_call(CIF, " << L_STAGE_CALLS << ", " << cur_node_name << "_ConP, &" << cur_output_name << ", &" << cur_input_name << ");\n";
                OUT << "if(!L_status.ok()) return L_status;\n";

                OUT << unindent() << "}\n";
                break;
            case NOP: case CON1: case CON2:
                if(!op.arg1.empty())
                    OUT << "// " << op.arg1 << "\n";
                if(!op.arg2.empty())
                    OUT << "// " << op.arg2 << "\n";
                break;
            case COFB: 
            case COIB: 
                assert(tvl.size() > 0);
                rvl = sfmt() << "!!(" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COEB:
                rvl = sfmt() << "!!int(" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COSB: 
                rvl = sfmt() << "stringtobool(" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COII: 
                if(op.arg.size() == 1 || (op.arg.size() > 1 && 
                        strcmp(grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[0])),
                               grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[1]))) != 0))
                        rvl = sfmt() <<  "(" << grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[0])) << ") (" <<  tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COFF: 
            case COIF: 
                if(op.arg.size() < 1)
                    rvl = sfmt() <<  "(double) (" << tvl.back().first << ")";
                else
                    rvl = sfmt() <<  "(" << grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[0])) << ") (" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COEI: 
            case COFI:
                if(op.arg.size() < 1)
                    rvl = sfmt() <<  "(int64_t) (" << tvl.back().first << ")";
                else
                    rvl = sfmt() <<  "(" << grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[0])) << ") (" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COIS: 
            case COFS: 
                rvl = sfmt() << "std::to_string(" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COEE: 
                rvl = sfmt() << "static_cast<" << get_full_name(op.el) << ">((int) (" << tvl.back().first << "))";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COEF: 
                rvl = sfmt() << "(" << grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[0])) << ") (int) (" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COSF: 
                rvl = sfmt() << "(" << grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[0])) << ") std::atof(" << tvl.back().first << ".c_str())";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COSI: 
                rvl = sfmt() << "(" << grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type(op.arg[0])) << ") atol(" << tvl.back().first << ".c_str())";
                tvl.back() = std::make_pair(rvl, 0);
                break;
            case COES: 
                rvl = sfmt() << get_full_name(op.er) << "_Name(" << tvl.back().first << ")";
                tvl.back() = std::make_pair(rvl, 0);
                break;
        }
    }
    return 0;
}
int flow_compiler::genc_cc_server(std::string const &server_src) {
    int error_count = 0;
    DEBUG_ENTER;
    OFSTREAM_SE(out, server_src);

    decltype(global_vars) local_vars;

    std::set<int> entry_node_set;
    // Sort the entries in source order
    for(int n: *this) if(at(n).type == FTK_ENTRY)  
        entry_node_set.insert(n);

    for(int entry_node: entry_node_set) {
        std::stringstream sbuf;
        error_count += gc_local_vars(sbuf,  method_descriptor(entry_node)->full_name(), method_descriptor(entry_node)->name(), entry_node);
        error_count += gc_server_method(sbuf,  method_descriptor(entry_node)->full_name(), method_descriptor(entry_node)->name(), entry_node);
        append(local_vars, "ENTRY_CODE", sbuf.str());
    }
    ServiceDescriptor const *sdp =  method_descriptor(*entry_node_set.begin())->service();
    set(local_vars, "CPP_SERVER_BASE", get_full_name(sdp));

    extern std::string get_template_server_C();

    if(DEBUG_GENC) {
        std::string ofn = server_src + "-global.json";
        OFSTREAM_SE(outj, ofn);
        stru1::to_json(outj, global_vars);
    }
    if(DEBUG_GENC) {
        std::string ofn = server_src + "-local.json";
        OFSTREAM_SE(outj, ofn);
        stru1::to_json(outj, local_vars);
    }

    vex::expand(out, get_template_server_C(), "server.C", local_vars, global_vars);
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::gc_local_vars(std::ostream &out, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry) const {
    out << "// Locals for [" << entry_dot_name << "] " << entry_name << " at " << blck_entry << "\n";
    auto const &g = flow_graph.find(blck_entry)->second;
    std::map<std::string, std::set<int>> gnodes;
    for(auto const &ss: g)
        for(int n: ss) if(n != blck_entry) {
            gnodes[type(n)].insert(n);
        }
    out << "// Nodes: " << gnodes  << "\n";
    for(auto const &gn: gnodes) {
        out << "// " << gn.first << "@\n";
        std::set<google::protobuf::Descriptor const *> ids;
        google::protobuf::Descriptor const *od = nullptr, *id = nullptr;
        std::map<google::protobuf::Descriptor const *, std::string> declared;
        for(int n: gn.second) {
            if(od == nullptr) od = message_descriptor(n);
            if(id == nullptr) id = input_descriptor(n);
            ids.insert(input_descriptor(n));
        }

        int dim = -1;
        for(int n: gn.second) {
            if(dim != -1 && dim != dimension(n)) 
                out << "// *** invalid dimension for " << n << ": " << dimension(n) << " instead of " << dim << "\n";
            if(dim == -1) 
                dim = dimension(n);
            
            if(method_descriptor(n) == nullptr || ids.size() <= 0) 
                continue;

            auto df = declared.find(input_descriptor(n));
            std::string declared_id = node_name_id(n, "rq");
            if(df == declared.end()) {
                out << "// " << reps("std::vector<", dim) << input_descriptor(n)->full_name() << reps(">", dim) << " " << declared_id << ";\n";
                declared[input_descriptor(n)] = declared_id;
            } else {
                out << "// " << "auto &" << declared_id << " = " << df->second << ";\n";
            } 
        }
        if(od != nullptr) {
            out << "// " << reps("std::vector<", dim) << od->full_name() << reps(">", dim) << " " << node_type_id(*gn.second.begin(), "rs") << ";\n";
            out << "// " << reps("std::vector<", dim) << "int" << reps(">", dim) << " " << node_type_id(*gn.second.begin(), "vi") << ";\n";
            out << "// " << "std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReader<" << od->full_name() << "> " << node_type_id(*gn.second.begin(), "ar") << ";\n";
            out << "// " << "std::vector<" << "int" << "> " << node_type_id(*gn.second.begin(), "cn") << ";\n";
            out << "// " << "std::vector<" << "void *" << "> " << node_type_id(*gn.second.begin(), "ip") << ";\n";
            out << "// " << "std::vector<" << od->full_name() << "*> " << node_type_id(*gn.second.begin(), "op") << ";\n";
        }
    }
    out << "// Stages: \n";
    int si = 0;
    for(auto const &ss: g) {
        ++si;
        out << "// " << "int cc_" << si << " = 0;\n";
        out << "// " << "::grpc::CompletionQueue cq_" << si << ";\n";
        out << "// " << "std::vector<std::unique_ptr<grpc::ClientContext>> cx_" << si << ";\n";
        out << "// " << "std::vector<grpc::Status> st_" << si << ";\n";
    }

    return 0;
}
