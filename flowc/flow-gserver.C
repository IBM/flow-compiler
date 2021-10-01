#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>

#include "flow-compiler.H"
#include "stru1.H"
#include "grpc-helpers.H"
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
    if(contains(reserved_cc, n)) return n + "_";
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

#define L_VISITED       NODE_VN2("Visited", name(cur_node))

// Generate C++ code for a given Entry Method
int flow_compiler::gc_server_method(std::ostream &os, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry) {
    indented_stream indenter(os, 1);
    auto eipp = entry_ip.find(blck_entry);
    OUT << "//  from " << main_file << ":" << at(blck_entry).token.line << " " << entry_dot_name << "\n";
    if(eipp == entry_ip.end()) {
        OUT  << "/**  no code gnerated */\n";
        return 1;
    }

    // Temporary storage for value covert call
    std::pair<std::string, std::string> convert_code; 
    // Temporary field reference stack
    std::vector<std::string> cur_loop_tmp; cur_loop_tmp.push_back("");
    std::vector<std::set<std::string>> loop_c;
    std::string rvl, lvl;   // Left and right value expressions
    std::vector<std::pair<std::string, int>> tvl; // Values stack
    std::vector<std::string> flvs; // Field length values stack
    // 
    int cur_stage = 0, cur_node = 0, node_dim = 0, stage_nodes = 0;
    std::string cur_stage_name, cur_input_name, cur_output_name, cur_node_name;
    std::string input_name, output_name;
    // dimension for every node response and current loop sizes
    accessor_info acinf;
    //std::map<std::string, int> rs_dims;
    std::vector<int> stage_node_ids;
    std::map<std::string, std::string> nodes_rv;
    bool first_node = false;        // whether the current node is the first in an alias set
    bool node_has_calls = false;    // whether this node makes grpc calls
    bool first_with_output = false; // whether this node is the first in an alias set that has output
    bool node_cg_done = false;      // done generating code for the node
    int alternate_nodes = 0;        // count of alternate nodes 
    EnumDescriptor const *ledp, *redp;   // left and right enum descriptor needed for conversion check
    int error_count = 0;

    std::string loop_chk, loop_end;
   
    for(int i = eipp->second, e = icode.size(), done = 0; i != e && !done; ++i) {
        fop const &op = icode[i];
        //OUT << "// " << i+1 << " " << op << "\n";
        switch(op.code) {
            case MTHD:
                input_name = op.arg1;
                nodes_rv[input_label] = input_name;
                output_name = op.arg2;
                OUT << "::grpc::Status " << get_name(op.m1) << "(flowc::call_info const &CIF, ::grpc::ServerContext *CTX, " << get_full_name(op.d1) << " const *p" << input_name << ", " << get_full_name(op.d2) << " *p" << output_name << ") {\n";
                ++indenter;
                OUT << "GRPC_ENTER_" << entry_name << "(\"" << entry_dot_name << "\", CIF, *CTX, p" << input_name << ")\n";
                //OUT << "auto CID = CIF.call_id;\n";
                //OUT << "auto const &Client_metadata = CTX->client_metadata();\n";
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

                OUT << "/*\n";
                OUT << " * stage: " << cur_stage << "\n";
                OUT << " * stage nodes: " << stage_nodes << "\n";
                OUT << " * stage name: " << cur_stage_name << "\n";
                OUT << " */\n";

                OUT << "auto " << L_STAGE_START << " = std::chrono::steady_clock::now();\n";
                OUT << "int " << L_STAGE_CALLS << " = 0;\n";
                    // The completion queue is shared between all the nodes in a stage
                    OUT << "::grpc::CompletionQueue " << L_QUEUE << ";\n";
                    // All the Status and ClientContext objects for the stage are kept in one vector
                    // Each node has a range of indices BX_xxxx and EX_xxxx 
                    // Additionally the number of calls currently sent (i.e. active, in the queue) is kept in SENT_xxxx
                    OUT << "std::vector<std::unique_ptr<grpc::ClientContext>> " << L_CONTEXT << ";\n";
                    OUT << "std::vector<grpc::Status> " << L_STATUS << ";\n";
                stage_node_ids.clear();
                break;
            case ESTG:
                {
                    OUT << "if(CIF.async_calls && 0 < " << L_STAGE_CALLS << ") {\n";
                    ++indenter;
                    OUT << "bool abort_stage = false;\n";
                    OUT << "::grpc::Status abort_status;\n";
                    for(auto nnj: stage_node_ids) if(method_descriptor(nnj) != nullptr) {
                        std::string nn(to_lower(to_identifier(referenced_nodes.find(nnj)->second.xname)));

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
                        std::string nn(to_lower(to_identifier(referenced_nodes.find(nni)->second.xname)));
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
                            OUT << "GRPC_RECEIVED(\"" << nn << "\", CIF, NRX, flowc::" << to_upper(to_identifier(nn)) << ", LL_Status, LL_Ctx, " << LN_OUTPTR(nn) << "[NRX-1])\n";
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

                    //OUT << "} else {\n";

                    --indenter;
                    OUT << "}\n";
                    OUT << "flowc::closeq(" << L_QUEUE << ");\n";
                    OUT << "if(abort_stage) {\n";
                    ++indenter;

                    for(auto nnj: stage_node_ids) {
                        std::string nn(to_lower(to_identifier(referenced_nodes.find(nnj)->second.xname)));
                        OUT << nn << "_ConP->release(CIF, " << LN_CONN(nn) << ".begin(), " << LN_CONN(nn) << ".end());\n";
                    }
                    OUT << "return abort_status;\n";
                    --indenter;
                    OUT << "}\n";

                    OUT << L_CONTEXT << ".clear();\n";
                    OUT << L_STATUS << ".clear();\n";
                    --indenter;
                    OUT << "}\n";
                }
                OUT << "Total_calls += " << L_STAGE_CALLS << ";\n";
                OUT << "PRINT_TIME(CIF, "<< cur_stage << ", \""<< cur_stage_name << "\", "<<L_STAGE_START<<" - ST, std::chrono::steady_clock::now() - "<< L_STAGE_START <<", "<< L_STAGE_CALLS<<");\n";
                OUT << "\n";
                break;
            case BNOD:
                node_cg_done = false;
                node_dim = op.arg[0];
                cur_input_name = op.arg2; 
                cur_output_name = op.arg1;
                cur_node_name = to_lower(to_identifier(referenced_nodes.find(cur_node = op.arg[1])->second.xname));
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
                OUT << " */\n";
                // input is not needed for no-call nodes
                if(op.d2 != nullptr) 
                    OUT << reps("std::vector<", node_dim) << get_full_name(op.d2) << reps(">", node_dim)  << " " << reps("v", node_dim) << cur_input_name << ";\n";
                
                // output must be set even when the node makes no calls if this is a first node with output
                if(first_with_output) 
                    OUT << reps("std::vector<", node_dim) << get_full_name(op.d1) << reps(">", node_dim)  << " " << reps("v", node_dim) << cur_output_name << ";\n";
                
                if(first_node) 
                    OUT << reps("std::vector<", node_dim) << "int"                << reps(">", node_dim)  << " " << reps("v", node_dim) << L_VISITED << (node_dim == 0? " = 0": "") << ";\n";
                
                if(node_has_calls) 
                    OUT << "auto " << cur_node_name << "_ConP = " << cur_node_name << "_get_connector();\n";

                    // Each node has a vector of response readers, input message poiners and output message pointers
                    if(op.d1 != nullptr) {
                        OUT << "std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReader<" << get_full_name(op.d1) << ">>> " << L_CARR << ";\n";
                        OUT << "std::vector<int> " << L_CONN << ";\n";
                        OUT << "std::vector<" << get_full_name(op.d2) << " *> " << L_INPTR << ";\n";
                        OUT << "std::vector<" << get_full_name(op.d1) << " *> " << L_OUTPTR <<  ";\n";
                    }
                    OUT << "int " << L_BEGIN << " = " << L_STAGE_CALLS << ", " << L_SENT << " = 0;\n";

                break;
            case BNL: { // node level loop begin
                std::string loop_size_varname = sfmt() << "NS_" << cur_node_name << "_" << loop_c.size();
                OUT << "auto " << loop_size_varname << " = " << loop_end << ";\n";

                OUT << reps("v", node_dim-acinf.loop_level()+1) << cur_input_name << ".resize("<< loop_size_varname << ");\n";
                if(first_with_output) 
                    OUT << reps("v", node_dim-acinf.loop_level()+1) << cur_output_name << ".resize("<< loop_size_varname << ");\n";
                if(first_node) 
                    OUT << reps("v", node_dim-acinf.loop_level()+1) << L_VISITED << ".resize("<< loop_size_varname << (node_dim-loop_c.size()==0? ", 0": "") << ");\n";
                OUT << "for(unsigned " << cpp_index_prefix << loop_c.size() << " = 0; " << cpp_index_prefix << loop_c.size() << " < NS_" << cur_node_name << "_"  << loop_c.size() << "; ++" << cpp_index_prefix << loop_c.size() << ") {\n" << indent();

                OUT << "auto &" << std::string(node_dim-loop_c.size(), 'v') << cur_input_name << " = " << std::string(node_dim-loop_c.size()+1, 'v') << cur_input_name << "[" << cpp_index_prefix << loop_c.size() <<  "];\n";
                OUT << "auto &" << std::string(node_dim-loop_c.size(), 'v') << cur_output_name << " = " << std::string(node_dim-loop_c.size()+1, 'v') << cur_output_name << "[" << cpp_index_prefix << loop_c.size() << "];\n";
                OUT << "auto &" << std::string(node_dim-loop_c.size(), 'v') << L_VISITED << " = " <<  std::string(node_dim-loop_c.size()+1, 'v') << L_VISITED << "[" << cpp_index_prefix << loop_c.size() << "];\n";

            } break;
            case IFNC:
                if(op.arg[1] == 0) {
                    OUT << "if(!" << L_VISITED << ") {" << "\n";
                } else {
                    assert(tvl.size() > 0);
                    OUT << "if(!" << L_VISITED << " && " << (tvl.back().second > 5? "(": "") << tvl.back().first << (tvl.back().second > 5? ")": "") << ") {" << "\n";
                    tvl.pop_back();
                }
                ++indenter;
                if(alternate_nodes)
                    OUT << "FLOGC(CIF.trace_call) << CIF << \"condition triggered for node " << cur_node_name << "\\n\";\n";
                break;
            case ENOD:
                // if visited
                if(!node_cg_done)
                    OUT <<  L_VISITED << " = " << cur_node << ";\n";
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
                OUT << "for(unsigned " << cpp_index_prefix << loop_c.size() << " = 0, LS" << loop_c.size() << " = " << loop_end << "; " << cpp_index_prefix  << loop_c.size() << " < LS" << loop_c.size() << "; ++" << cpp_index_prefix << loop_c.size() << ") {\n" << indent();
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
            case FUNC:
                assert(tvl.size() >= op.arg[0]);
                rvl = sfmt() << "flowrt::" << op.arg1 << "(";
                for(int i = tvl.size() - op.arg[0]; i < tvl.size(); ++i) {
                    rvl += tvl[i].first; 
                    if(i + 1 != tvl.size()) rvl += ", ";
                }
                rvl += ")";
                for(int i = 0; i < op.arg[0]; ++i)
                    tvl.pop_back();
                tvl.push_back(std::make_pair(rvl, 0));
                break;
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

            case RVC: 
                if(op.ev1 != nullptr) rvl = get_full_name(op.ev1);
                else if(op.arg.size() > 1 && op.arg[1] == (int) google::protobuf::FieldDescriptor::Type::TYPE_STRING) rvl =  c_escape(op.arg1); 
                else rvl = op.arg1;
                tvl.push_back(std::make_pair(rvl, 0));
                break;

            case RVV: 
                tvl.push_back(std::make_pair("flowdef::"+op.arg1, 0));
                break;

            case SVF:
                rvl = cpp_var(loop_c, op.arg1, op.arg[0], SIZE);
                tvl.push_back(std::make_pair(rvl, 0));
                break;

            case RVF: 
                rvl = cpp_var(loop_c, op.arg1, op.arg[0], RIGHT_VALUE);
                tvl.push_back(std::make_pair(rvl, 0));
                //OUT << "auto GJ" << i << " = " << rvl << ";\n";
                break;

            case DACC: {
                    std::set<std::string> lxset;
                    while(flvs.size() > 0 && flvs.back() != "") {
                        lxset.insert(flvs.back()); flvs.pop_back();
                    }
                    std::string lenx = lxset.size() == 1? *lxset.begin(): stru1::join(lxset.begin(), lxset.end(), ", ", ", ", "flowrt::min(", "", "", ")");
                    if(flvs.size() > 0) 
                        flvs.pop_back();

                    rvl = sfmt() << lenx << ", [&] (int I1) -> auto { return " << tvl[tvl.size()-1].first << "; }";
                    tvl.pop_back();
                    tvl.push_back(std::make_pair(rvl, 0));
                }
                break;

            case STOL: 
                flvs.push_back( cpp_var(loop_c, op.arg1, op.arg[0], SIZE, nullptr) );
                break;

            case CLLS: 
                flvs.push_back("");
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

            case ERR:
                OUT << "FLOG << \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << "): node error\\n\";\n";
                OUT << "return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, " << c_escape(op.arg1) << ");\n";
                // Prevent generating unreachable code
                node_cg_done = true;
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
int flow_compiler::get_blck_timeout(int blck, int default_timeout) {
    int timeout_value = 0;
    get_block_value(timeout_value, blck, "timeout", false, {});
    if(timeout_value == 0) return default_timeout;
    int timeout = 0;
    switch(at(timeout_value).type) {
        case FTK_FLOAT:
            timeout = int(get_float(timeout_value)*1000);
            break;
        case FTK_INTEGER:
            timeout = get_integer(timeout_value)*1000;
            break;
        case FTK_STRING:
            timeout = get_time_value(get_string(timeout_value));
            break;
        default:
            break;
    }
    if(timeout <= 0) {
        pcerr.AddWarning(main_file, at(timeout_value), sfmt() << "ignoring invalid value for \"timeout\", using the default of \""<<default_timeout<<"ms\"");
        return default_timeout;
    }
    return timeout;
}
int flow_compiler::set_entry_vars(decltype(global_vars) &vars) {
    int error_count = 0;
    std::set<int> entry_node_set;
    // Sort the entries in source order
    for(auto const &ne: named_blocks) if(ne.second.first == "entry")
        entry_node_set.insert(ne.second.second);
    
    ServiceDescriptor const *sdp = nullptr;
    int entry_count = 0;
    std::set<MethodDescriptor const *> entry_mdps, all_mdps;
    for(int entry_node: entry_node_set) {
        ++entry_count;
        MethodDescriptor const *mdp = method_descriptor(entry_node);
        if(sdp != nullptr && sdp != mdp->service()) {
            error_count += 1;
            pcerr.AddError(main_file, -1, 0, "all entries must be methods of the same service");
        }
        sdp = mdp->service();
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), description(entry_node), true, true);
        std::map<std::string, int> defsn;
        error_count += get_nv_block(defsn, entry_node, "defaults", {FTK_STRING, FTK_FLOAT, FTK_INTEGER});
        std::map<std::string, std::string> defs;
        for(auto d: defsn)
            defs[d.first] = get_value(d.second);
        std::string input_schema = json_schema(defs, mdp->input_type(), to_upper(to_option(main_name)), main_description, true, true);
        entry_mdps.insert(mdp); all_mdps.insert(mdp);
        append(vars, "ENTRY_PROTO", gen_proto(mdp));
        append(vars, "ENTRY_FULL_NAME", mdp->full_name());
        append(vars, "ENTRY_NAME", mdp->name());
        append(vars, "ENTRY_URL", sfmt() << "/" << mdp->name());
        append(vars, "ENTRY_SERVICE_SHORT_NAME", get_name(mdp->service()));
        append(vars, "ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "ENTRY_OUTPUT_SHORT_TYPE", get_name(mdp->output_type()));
        append(vars, "ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "ENTRY_INPUT_SHORT_TYPE", get_name(mdp->input_type()));
        append(vars, "ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
        append(vars, "ENTRY_TIMEOUT", std::to_string(get_blck_timeout(entry_node, default_entry_timeout)));
        append(vars, "ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
        append(vars, "ENTRY_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "ENTRY_DESCRIPTION", description(entry_node));
        append(vars, "ENTRY_ORDER", sfmt() << entry_count);
        std::vector<int> values;
        std::string hidden_fields;
        error_count += get_block_value(values, entry_node, "hide", false, {FTK_STRING});
        for(int n: values) hidden_fields = hidden_fields + (hidden_fields.empty()? "": ", ") + get_string(n);
        append(vars, "ENTRY_HIDDEN_FIELDS", hidden_fields);
        values.clear();
        std::string hidden_labels;
        error_count += get_block_value(values, entry_node, "hide_label", false, {FTK_STRING});
        for(int n: values) hidden_labels = hidden_labels + (hidden_labels.empty()? "": ", ") + get_string(n);
        append(vars, "ENTRY_HIDDEN_LABELS", hidden_labels);

        if(entry_count == 1) {
            append(vars, "MAIN_ENTRY_PROTO", gen_proto(mdp));
            append(vars, "MAIN_ENTRY_FULL_NAME", mdp->full_name());
            append(vars, "MAIN_ENTRY_NAME", mdp->name());
            append(vars, "MAIN_ENTRY_URL", sfmt() << "/" << mdp->name());
            append(vars, "MAIN_ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
            append(vars, "MAIN_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
            append(vars, "MAIN_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
            append(vars, "MAIN_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(entry_node, default_entry_timeout)));
            append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
            append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON", input_schema);
            append(vars, "MAIN_ENTRY_DESCRIPTION", description(entry_node));
            append(vars, "MAIN_ENTRY_HIDDEN_FIELDS", hidden_fields);
            append(vars, "MAIN_ENTRY_HIDDEN_LABELS", hidden_labels);
        } else {
            append(vars, "ALT_ENTRY_PROTO", gen_proto(mdp));
            append(vars, "ALT_ENTRY_FULL_NAME", mdp->full_name());
            append(vars, "ALT_ENTRY_NAME", mdp->name());
            append(vars, "ALT_ENTRY_URL", sfmt() << "/" << mdp->name());
            append(vars, "ALT_ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
            append(vars, "ALT_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
            append(vars, "ALT_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
            append(vars, "ALT_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(entry_node, default_entry_timeout)));
            append(vars, "ALT_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
            append(vars, "ALT_ENTRY_INPUT_SCHEMA_JSON", input_schema);
            append(vars, "ALT_ENTRY_DESCRIPTION", description(entry_node));
            append(vars, "ALT_ENTRY_HIDDEN_FIELDS", hidden_fields);
            append(vars, "ALT_ENTRY_HIDDEN_LABELS", hidden_labels);
        }
    }
    if(entry_count > 1)
        set(vars, "HAVE_ALT_ENTRY", "");
    set(vars, "ENTRY_COUNT", sfmt() << entry_count);
    set(vars, "ALT_ENTRY_COUNT", sfmt() << entry_count-1);
    set(vars, "ENTRIES_PROTO", gen_proto(entry_mdps));
    for(auto &rn: referenced_nodes) {
        MethodDescriptor const *mdp = method_descriptor(rn.first);
        if(mdp != nullptr)
            all_mdps.insert(mdp);
    }
    for(auto mdp: entry_mdps) {
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), "", true, true);
        std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), decamelize(mdp->input_type()->name()), "", true, true);
        append(vars, "MDP_PROTO", gen_proto(mdp));
        append(vars, "MDP_FULL_NAME", mdp->full_name());
        append(vars, "MDP_NAME", mdp->name());
        append(vars, "MDP_SERVICE_SHORT_NAME", get_name(mdp->service()));
        append(vars, "MDP_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "MDP_OUTPUT_SHORT_TYPE", get_name(mdp->output_type()));
        append(vars, "MDP_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "MDP_INPUT_SHORT_TYPE", get_name(mdp->input_type()));
        append(vars, "MDP_INPUT_TYPE", get_full_name(mdp->input_type()));
        append(vars, "MDP_OUTPUT_SCHEMA_JSON", output_schema);
        append(vars, "MDP_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "MDP_IS_ENTRY", "1");
    }
    for(auto mdp: all_mdps) if(!contains(entry_mdps, mdp)) {
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), "", true, true);
        std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), decamelize(mdp->input_type()->name()), "", true, true);
        append(vars, "MDP_PROTO", gen_proto(mdp));
        append(vars, "MDP_FULL_NAME", mdp->full_name());
        append(vars, "MDP_NAME", mdp->name());
        append(vars, "MDP_SERVICE_SHORT_NAME", get_name(mdp->service()));
        append(vars, "MDP_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "MDP_OUTPUT_SHORT_TYPE", get_name(mdp->output_type()));
        append(vars, "MDP_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "MDP_INPUT_SHORT_TYPE", get_name(mdp->input_type()));
        append(vars, "MDP_INPUT_TYPE", get_full_name(mdp->input_type()));
        append(vars, "MDP_OUTPUT_SCHEMA_JSON", output_schema);
        append(vars, "MDP_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "MDP_IS_ENTRY", "0");
    }
    set(vars, "MDP_COUNT", sfmt() << all_mdps.size());
    set(vars, "ALL_NODES_PROTO", gen_proto(all_mdps));
    return error_count;
}
int flow_compiler::set_cli_active_node_vars(decltype(global_vars) &vars, int cli_node) {
    int error_count = 0;
    auto rn = referenced_nodes.find(cli_node);
    assert(rn != referenced_nodes.end());
    std::string const &node_name = rn->second.xname;

    clear(global_vars, "ALT_ENTRY_NAME");

    append(vars, "HAVE_ACTIVE_NODE", "");
    append(vars, "MAIN_ENTRY_DESCRIPTION", description(cli_node));
    append(vars, "MAIN_ENTRY_NAME", node_name);
    append(vars, "MAIN_ENTRY_URL", sfmt() << "/-node/" << node_name);
    auto mdp = method_descriptor(cli_node);
    append(vars, "MAIN_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
    append(vars, "MAIN_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
    std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), description(cli_node), true, true);
    append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
    std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), node_name, description(cli_node), true, true);
    append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON", input_schema);
    append(vars, "MAIN_ENTRY_METHOD_NAME", mdp->name());
    append(vars, "MAIN_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(cli_node, default_node_timeout)));
    append(vars, "MAIN_ENTRY_PROTO", gen_proto(mdp));
    return error_count;
}
int flow_compiler::set_cli_node_vars(decltype(global_vars) &vars) {
    int error_count = 0, node_count = 0;
    for(auto &rn: referenced_nodes) {
        ++node_count;
        auto cli_node = rn.first;
        if(type(cli_node) == "container" || method_descriptor(cli_node) == nullptr) 
            continue;
        std::string const &node_name = rn.second.xname;

        std::string set_metadata;
        if(rn.second.headers.size() > 0) {
            std::vector<std::string> metadata;
            for(auto const &hnv: rn.second.headers) {
                if(name.has(hnv.second)) 
                    metadata.push_back(sfmt() << "(context).AddMetadata(" << c_escape(to_option(hnv.first)) << ", flowdef::" << name(hnv.second) << ");");
                else
                    metadata.push_back(sfmt() << "(context).AddMetadata(" << c_escape(to_option(hnv.first)) << ", " << c_escape(get_string(hnv.second)) << ");");
            }
            
            set_metadata = join(metadata, " ", " ", "{", "", "", "}");
        }
        append(vars, "CLI_NODE_METADATA", set_metadata);
        append(vars, "CLI_NODE_LINE", sfmt() << at(cli_node).token.line);
        append(vars, "CLI_NODE_DESCRIPTION", description(cli_node));
        append(vars, "CLI_NODE_NAME", node_name);
        append(vars, "CLI_NODE_URL", sfmt() << "/-node/" << node_name);
        auto mdp = method_descriptor(cli_node);
        append(vars, "CLI_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "CLI_GRPC_SERVICE_NAME", mdp->service()->name());
        append(vars, "CLI_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "CLI_INPUT_TYPE", get_full_name(mdp->input_type()));
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), description(cli_node), true, false);
        append(vars, "CLI_OUTPUT_SCHEMA_JSON", output_schema);
        std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), node_name, description(cli_node), true, false);
        append(vars, "CLI_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "CLI_PROTO", gen_proto(mdp));
        append(vars, "CLI_METHOD_NAME", mdp->name());
        append(vars, "CLI_NODE_TIMEOUT", std::to_string(get_blck_timeout(cli_node, default_node_timeout)));
        append(vars, "CLI_NODE_GROUP", rn.second.group);
        append(vars, "CLI_NODE_ENDPOINT", rn.second.external_endpoint);
        int cc_value = 0;
        error_count += get_block_value(cc_value, cli_node, "replicas", false, {FTK_INTEGER});
        cc_value = cc_value == 0? default_maxcc: get_integer(cc_value);
        if(cc_value <= 0)
            pcerr.AddWarning(main_file, at(cli_node), sfmt() << "ignoring invalid value for the number of concurrent clients: \""<<cc_value<<"\"");
        append(vars, "CLI_NODE_MAX_CONCURRENT_CALLS", std::to_string(cc_value));
    }
    if(node_count > 0) 
        set(vars, "HAVE_CLI", "");
    return error_count;
}
int flow_compiler::genc_server_source(std::string const &server_src) {
    int error_count = 0;
    DEBUG_ENTER;
    OFSTREAM_SE(out, server_src);

    decltype(global_vars) local_vars;

    std::set<int> entry_node_set;
    // Sort the entries in source order
    for(auto const &ne: named_blocks) if(ne.second.first == "entry") 
        entry_node_set.insert(ne.second.second);

    for(int entry_node: entry_node_set) {
        std::stringstream sbuf;
        error_count += gc_local_vars(sbuf,  method_descriptor(entry_node)->full_name(), method_descriptor(entry_node)->name(), entry_node);
        error_count += gc_server_method(sbuf,  method_descriptor(entry_node)->full_name(), method_descriptor(entry_node)->name(), entry_node);
        append(local_vars, "ENTRY_CODE", sbuf.str());
    }
    ServiceDescriptor const *sdp =  method_descriptor(*entry_node_set.begin())->service();
    set(local_vars, "CPP_SERVER_BASE", get_full_name(sdp));

    extern char const *template_server_C;
    auto mgv = vex::make_smap(global_vars);
    auto mlv = vex::make_smap(local_vars);

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

    vex::expand(out, template_server_C, vex::make_cmap(mlv, mgv));
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::gc_local_vars(std::ostream &out, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry) const {
    out << "// Locals for [" << entry_dot_name << "] " << entry_name << " at " << blck_entry << "\n";
    auto const &g = flow_graph.find(blck_entry)->second;
    std::map<std::string, std::set<int>> gnodes;
    for(auto const &ss: g)
        for(int n: ss) if(n != blck_entry) {
            gnodes[name(n)].insert(n);
        }
    out << "// Nodes: " << gnodes  << "\n";
    for(auto const &gn: gnodes) {
        out << "// " << gn.first << "@\n";
        std::set<google::protobuf::Descriptor const *> ids;
        google::protobuf::Descriptor const *od = nullptr, *id = nullptr;
        std::map<google::protobuf::Descriptor const *, std::string> declared;
        for(int n: gn.second) {
            if(method_descriptor(n) == nullptr)
                continue;
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
        if(od != nullptr) 
            out << "// " << reps("std::vector<", dim) << od->full_name() << reps(">", dim) << " " << node_type_id(*gn.second.begin(), "rs") << ";\n";
        out << "// " << reps("std::vector<", dim) << "int" << reps(">", dim) << " " << node_type_id(*gn.second.begin(), "vi") << ";\n";
        out << "// " << "std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReader<" << od->full_name() << "> " << node_type_id(*gn.second.begin(), "ar") << ";\n";
        out << "// " << "std::vector<" << "int" << "> " << node_type_id(*gn.second.begin(), "cn") << ";\n";
        out << "// " << "std::vector<" << "void *" << "> " << node_type_id(*gn.second.begin(), "ip") << ";\n";
        out << "// " << "std::vector<" << od->full_name() << "*> " << node_type_id(*gn.second.begin(), "op") << ";\n";
    }
    out << "// Stages: \n";
    int si = 0;
    for(auto const &ss: g) {
        ++si;
        out << "// " << "int cc_" << si << " = 0;\n";
        out << "// " << "::grpc::CompletionQueue cq_" << si << ";\n";
        out << "// " << "std::vector<std::unique_ptr<grpc::ClientContext>> cx_" << si << ";\n";
        out << "// " << "std::vector<grpc::Status> st_" << si << ";\n";
        std::set<std::string> node_types;
        for(int n: ss) if(n != blck_entry) 
            if(node_types.find(name(n)) == node_types.end()) {
                node_types.insert(name(n));
            }
    }

    return 0;
}
