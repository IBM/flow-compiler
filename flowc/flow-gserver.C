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
#include "varsub.H"
#include "grpc-helpers.H"

using namespace varsub;
using namespace stru1;

#define OUT out << std::string(indent*4, ' ')

static 
std::string reps(std::string rep, unsigned count) {
    std::stringstream buf;
    for(; count != 0; --count) buf << rep;
    return buf.str();
}
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
    if(right_ftk_type == FTK_dtid) switch(lt) {
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
    LEFT_VALUE, RIGHT_VALUE, LEFT_STEM, RIGHT_STEM, SIZE
};

std::ostream &operator <<(std::ostream &out, accessor_type at) {
    switch(at) {
        case LEFT_VALUE:
            return out << "LEFT-VALUE";
        case RIGHT_VALUE:
            return out << "RIGHT-VALUE";
        case LEFT_STEM:
            return out << "LEFT-STEM";
        case RIGHT_STEM:
            return out << "RIGHT-STEM";
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

static 
std::string field_accessor_d(bool trace_on, std::string const &field, Descriptor const *d, std::map<std::string, int> const &rs_dims, accessor_type kind, int cur_level=-1) {
    TRACE << "field_accessor(" << field << ", " << rs_dims << ", " << kind << ", " << cur_level << "):\n";
    std::stringstream buf;
    std::string base, fields;
    // The first field is the local variable name
    if(split(&base, &fields, field, "+") == 1) {
        buf << base;
        if(kind == LEFT_STEM) buf << ".";
        return buf.str();
    }

    bool left = kind == LEFT_VALUE || kind == LEFT_STEM;
    bool right = kind == RIGHT_VALUE || kind == RIGHT_STEM || kind == SIZE;

    int level = right && rs_dims.find(base) != rs_dims.end()? rs_dims.find(base)->second: 0;

    if(right) {
        buf << reps("v", level) << base;
        for(int i = 0; i < level; ++i)
            buf << "[I" << (i+1) << "]";
    } else {
        buf << base;
    }
    buf << ".";

    while(split(&base, &fields, fields, "+") == 2) {
        FieldDescriptor const *fd = d->FindFieldByName(base);
        base = base_name(base);

        d = fd->message_type();

        if(fd->is_repeated()) {
            ++level;
            if(right) {
                buf << base << "(I" << level << ").";
            } else if(left) {
                buf.str("");
                buf << ".";
            }
        } else {
            if(d != nullptr && left) buf << "mutable_" << base << "()->";
            else buf << base << "().";
        }
        if(cur_level > 0 && level == cur_level && left) {
            cur_level = -1; 
            buf.str("");
            buf << ".";
        }
    }

    FieldDescriptor const *fd = d->FindFieldByName(base);
    base = base_name(base);
    switch(kind) {
        case LEFT_VALUE:
            if(fd->is_repeated()) 
                buf << "add_" << base << "(";
            else 
                buf << "set_" << base << "(";
            break;
        case RIGHT_VALUE:
            if(fd->is_repeated()) 
                buf << base << "(I"<<++level<<")";
            else 
                buf << base << "()";
            break;
        case SIZE:
            buf << base << "_size()";
            break;
        case LEFT_STEM: 
            if(cur_level > 0 && level <= cur_level) {
                // No access needed: temp var
                buf.str(".");
            } else {
                buf << "mutable_" << base << "()->";
            }
            break; 
        case RIGHT_STEM:
            buf << base;
            break;
    }
    return buf.str();
}
static 
std::string field_accessor(std::string const &field, Descriptor const *d, std::map<std::string, int> const &rs_dims, accessor_type kind, int cur_level=-1) {
    bool trace_on = false;
    std::string fa = field_accessor_d(trace_on, field, d, rs_dims, kind, cur_level);
    TRACE << "  " << fa << "\n";
    return fa;
}
static
std::string get_loop_size(flow_compiler const *fc, std::vector<fop> const &icode, std::vector<int> const &index_set, std::map<std::string, int> const &rs_dims, int cur_level=100) {
    //std::cerr << "BEGIN get_loop_size, cur_level: "<< cur_level << "\n\tindex_set: " << index_set << "\n\trs_dims: " << rs_dims << "\n";
    std::vector<std::pair<std::string, std::string>> indices;
    for(int ixi: index_set) {
        fop const &ix = icode[ixi-1];
        std::vector<std::string> names(&ix.arg1, &ix.arg1+1);
        for(int i = 1; i < ix.arg.size(); ++i) names.push_back(fc->get_id(ix.arg[i]));  
        std::string index_size(field_accessor(join(names, "+"), ix.d1, rs_dims, SIZE, cur_level)); 
        indices.push_back(std::make_pair(join(names, "+"), index_size));
    }
    std::sort(indices.begin(), indices.end());
    //std::cerr << "\tindices: " << indices << "\n";
    // Eliminate all indices that are prefixes of other indices
    
    for(auto fp = indices.begin(), sp = fp+1, ep = indices.end(); sp != ep; ++fp, ++sp) 
        if(stru1::starts_with(sp->first, fp->first+"+")) fp->first.clear();
    
    std::string current_loop_size;
    for(auto ni: indices) if(!ni.first.empty()) {
        if(current_loop_size.empty())
            current_loop_size = ni.second;
        else 
            current_loop_size = std::string("std::min(")+current_loop_size + ", " + ni.second + ")";
    }
    //std::cerr << "\tindices: " << indices << "\n";
    //std::cerr << "END get_loop_size: " << current_loop_size <<  "\n";
    return current_loop_size;
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
#define LN_BEGIN(n)     NODE_VN2("Base_X", (n))
#define L_BEGIN         LN_BEGIN(cur_node_name)
#define LN_END_X(n)     NODE_VN2("End_X", (n))
#define L_END_X         LN_END_X(cur_node_name)
#define LN_SENT(n)      NODE_VN2("Sent", (n))
#define L_SENT          LN_SENT(cur_node_name)

#define LN_OUTPTR(n)    NODE_VN2("Out_Ptr", (n))
#define L_OUTPTR        LN_OUTPTR(cur_node_name)

#define L_VISITED       NODE_VN2("Visited", name(cur_node))

static std::map<int, int> bexp_op_priority = {
    { FTK_OR, 0 },
    { FTK_AND, 1 },
    { FTK_NE, 2 }, { FTK_EQ, 2 }, 
    { FTK_GT, 3 }, { FTK_LT, 3 }, { FTK_GE, 3 }, { FTK_LE, 3 }, 
    { FTK_BANG, 4},
    { FTK_HASH, 5}
    
};
/*
 * return true if op2 has higher priority than op1
 */
bool check_bexp_op_priority(int op1, int op2) {
    return bexp_op_priority.find(op1)->second > bexp_op_priority.find(op2)->second;
}

std::ostream &flow_compiler::gc_bexp(std::ostream &out, std::map<std::string, std::string> const &nip, std::map<std::string, int> const &rs_dims, int bexp, int op) const{ 
    std::pair<std::string, std::string> convert_code;
    auto const &bx = at(bexp);
    bool need_parens = false;
    switch(bx.type) {
        case FTK_bexp:
            switch(bx.children.size()) {
                case 1:
                    gc_bexp(out, nip, rs_dims, bx.children[0], op);
                    break;
                case 2:
                    need_parens = check_bexp_op_priority(op, at(bx.children[0]).type);
                    if(need_parens) out << "(";
                    out << " " << node_name(at(bx.children[0]).type);
                    // TODO length operator should return 1 for non repeated field?
                    gc_bexp(out, nip, rs_dims, bx.children[1], at(bx.children[0]).type);
                    if(need_parens) out << ")";
                    break;
                case 3:
                    need_parens = check_bexp_op_priority(op, at(bx.children[1]).type);
                    if(need_parens) out << "(";
                    gc_bexp(out, nip, rs_dims, bx.children[0], at(bx.children[1]).type);
                    out << " " << node_name(at(bx.children[1]).type) << " ";
                    gc_bexp(out, nip, rs_dims, bx.children[2], at(bx.children[1]).type);
                    if(need_parens) out << ")";
                    break;
            }
            break;
        case FTK_fldx:
            out << ::field_accessor(nip.find(get_id(bx.children[0]))->second + "+" + get_joined_id(bexp, 1, "+"), message_descriptor(bx.children[0]), rs_dims, RIGHT_VALUE);
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
        case FTK_dtid:
            // std::cerr << get_full_name(enum_descriptor(bexp)) << "\n";
            out << get_full_name(enum_descriptor(bexp));
            //out << enum_descriptor(bexp)->number();
            break;
    }
    return out;
}
// Generate C++ code for a given Entry Method
int flow_compiler::gc_server_method(std::ostream &out, std::string const &entry_dot_name, int blck_entry) {
    int indent = 1;
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
    // 
    int loop_level = 0, cur_stage = 0, cur_node = 0, node_dim = 0, stage_nodes = 0, cur_base_node = 0;
    std::string cur_stage_name, cur_input_name, cur_output_name, cur_node_name;
    std::string input_name, output_name;
    // dimension for every node response 
    std::map<std::string, int> rs_dims;
    std::vector<std::string> stage_node_names;
    std::map<std::string, std::string> nodes_rv;
    bool async_enabled = false;
    bool first_node = false;        // whether the current node is the first in an alias set
    EnumDescriptor const *ledp, *redp;   // left and right enum descriptor needed for conversion check
    int error_count = 0;
   
    for(int i = eipp->second, e = icode.size(), done = 0; i != e && !done; ++i) {
        fop const &op = icode[i];
        OUT << "// " << i << " " << op << "\n";
        switch(op.code) {
            case MTHD:
                input_name = op.arg1;
                nodes_rv[input_label] = input_name;
                output_name = op.arg2;
                OUT << "::grpc::Status " << get_name(op.m1) << "(::grpc::ServerContext *CTX, " << get_full_name(op.d1) << " const *p" << input_name << ", " << get_full_name(op.d2) << " *p" << output_name << ") override {\n";
                ++indent;
                OUT << "auto const &Client_metadata = CTX->client_metadata();\n";
                OUT << "bool Trace_call = Get_metadata_bool(Client_metadata, \"trace-call\");\n";
                OUT << "bool Time_call = Get_metadata_bool(Client_metadata, \"time-call\");\n";
                OUT << "bool Async_call = Get_metadata_bool(Client_metadata, \"overlapped-calls\", Async_Flag);\n";
                OUT << get_full_name(op.d1) << " const &" << input_name << " = *p" << input_name << ";\n";
                OUT << get_full_name(op.d2) << " &" << output_name << " = *p" << output_name << ";\n";
                OUT << "::grpc::Status L_status = ::grpc::Status::OK;\n";
                OUT << "TIME_INFO_BEGIN(Time_call);\n";
                OUT << "auto ST = std::chrono::steady_clock::now();\n";
                OUT << "int Total_calls = 0;\n";

                OUT << "auto CID = Call_Counter.fetch_add(1, std::memory_order_seq_cst);\n";
                OUT << "TRACEA((Async_call? \"enter " << entry_dot_name << "/asynchronous calls: \": \"enter " << entry_dot_name << "/synchronous calls: \"), &" << input_name << ");\n";
                OUT << "\n"; 
                break;
            case END:
                OUT << "PTIME2(\"" << entry_dot_name << "\", 0, \"total\", ST - ST, std::chrono::steady_clock::now() - ST, Total_calls);\n";
                OUT << "if(Time_call) CTX->AddTrailingMetadata(\"times-bin\", TIME_INFO_GET(Time_call)); \n";
                OUT << "TIME_INFO_END(Time_call);\n";
                OUT << "TRACEA(\"reply " << entry_dot_name << ": \", &" << output_name << ");\n";
                OUT << "TRACEA(Time_info.str(), nullptr);\n";
                OUT << "TRACEAF(\"" << entry_dot_name << " done\", nullptr);\n";
                OUT << "return L_status;\n";
                --indent; 
                OUT << "}\n";
                done = 1;
                break;
            case BSTG:
                cur_stage = op.arg[0];
                stage_nodes = op.arg[1];
                cur_stage_name = op.arg1;
                async_enabled = stage_nodes > 1 ||  op.arg[2] > 0;

                OUT << "/*\n";
                OUT << " * stage: " << cur_stage << "\n";
                OUT << " * stage nodes: " << stage_nodes << "\n";
                OUT << " * stage name: " << cur_stage_name << "\n";
                OUT << " * async enabled: " << async_enabled << "\n";
                OUT << " */\n";

                OUT << "auto " << L_STAGE_START << " = std::chrono::steady_clock::now();\n";
                OUT << "int " << L_STAGE_CALLS << " = 0;\n";
                if(async_enabled) {
                    // The completion queue is shared between all the nodes in a stage
                    OUT << "::grpc::CompletionQueue " << L_QUEUE << ";\n";
                    // All the Status and ClientContext objects for the stage are kept in one vector
                    // Each node has a range of indices BX_xxxx and EX_xxxx 
                    // Additionally the number of calls currently sent (i.e. active, in the queue) is kept in SENT_xxxx
                    OUT << "std::vector<std::unique_ptr<grpc::ClientContext>> " << L_CONTEXT << ";\n";
                    OUT << "std::vector<std::unique_ptr<grpc::Status>> " << L_STATUS << ";\n";
                }
                stage_node_names.clear();
                break;
            case ESTG:
                if(async_enabled) {
                    OUT << "if(Async_call && 0 < " << L_STAGE_CALLS << ") {\n";
                    ++indent;
                    OUT << "void *TAG; bool NextOK = false; int " << L_RECV << " = 0;\n";
                    OUT << "while(" << L_QUEUE << ".Next(&TAG, &NextOK)) {\n";
                    ++indent;
                    OUT << "if(CTX->IsCancelled()) {\n";
                    ++indent;
                    OUT << "ERROR(\"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << "): call cancelled\");\n";
                    OUT << L_QUEUE << ".Shutdown();\n";
                    OUT << "return ::grpc::Status(::grpc::StatusCode::CANCELLED, \"Call exceeded deadline or was cancelled by the client\");\n";
                    --indent;
                    OUT << "}\n";
                    OUT << "if(NextOK) {\n";
                    ++indent;
                    OUT << "int X = (int) (long) TAG;\n";
                    OUT << "TRACEC(X, \"woke up in " << entry_dot_name << " stage " << cur_stage << " (" << cur_stage_name << ")\");\n";
                    OUT << "auto &LL_Status = *" << L_STATUS << "[X-1];\n";
                    OUT << "auto &LL_Ctx = *" << L_CONTEXT << "[X-1];\n";
                    int nc = 0;
                    for(auto nn: stage_node_names) {
                        // Find out what node this index belongs to by comparing with the EX_xxxx markers
                        if(nc > 0) OUT << "else ";
                        else OUT << "";
                        if(++nc == stage_nodes)
                            out << "{\n";
                        else 
                            out << "if(X <= " << LN_END_X(nn) << ") {\n";
                        ++indent;
                        // The index in the node vectors (result and result reader)
                        OUT << "int NRX = X - " << LN_BEGIN(nn) << ";\n";
                        OUT << "if(" << LN_SENT(nn) << " != " << LN_END_X(nn) <<  " - " << LN_BEGIN(nn) << ") {\n";
                        ++indent;
                        // If there are still requests to be sent, add a new one, of the same kind, to the queue
                        OUT << "int Nx = " << LN_SENT(nn) << ";\n";
                        OUT << "int Sx = Nx + " << LN_BEGIN(nn) << ";\n";
                        OUT << "auto &RPCx = " << LN_CARR(nn) << "[Nx];\n";
                        OUT << "RPCx->StartCall();\n";
                        OUT << "RPCx->Finish(" << LN_OUTPTR(nn) << "[Nx], " << L_STATUS << "[Sx].get(), (void *) (long) (Sx+1));\n";
                        OUT << "++" << LN_SENT(nn) << ";\n";
                        --indent;
                        OUT << "}\n";
                        OUT << "TRACECM(LL_Status.ok(), NRX, \"" << nn << " response: \", " << LN_OUTPTR(nn) <<"[NRX-1]);\n";

                        OUT << "if(!LL_Status.ok()) {\n";
                        ++indent;
                        OUT << "GRPC_ERROR(NRX, \"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << ") " << nn << "\", LL_Status, LL_Ctx, nullptr, nullptr);\n";
                        OUT << L_QUEUE << ".Shutdown();\n";
                        OUT << "return LL_Status;\n";
                        --indent;
                        OUT << "}\n";
                        --indent;
                        OUT << "}\n";
                    }
                    OUT << "if(++" << L_RECV << " >= " << L_STAGE_CALLS << ") " << L_QUEUE << ".Shutdown();\n";
                    --indent;
                    OUT << "} else {\n";
                    ++indent;
                    OUT << "ERROR(\"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << "): Next() not OK\");\n";
                    OUT << L_QUEUE << ".Shutdown();\n";
                    OUT << "return ::grpc::Status(::grpc::INTERNAL, \"Next() not OK\");\n";
                    --indent;
                    OUT << "}\n";
                    --indent;
                    OUT << "}\n";
                    --indent;
                    OUT << "}\n";
                }
                OUT << "Total_calls += " << L_STAGE_CALLS << ";\n";
                OUT << "PTIME2(\"" << entry_dot_name << "\", "<< cur_stage << ", \""<< cur_stage_name << "\", "<<L_STAGE_START<<" - ST, std::chrono::steady_clock::now() - "<< L_STAGE_START <<", "<< L_STAGE_CALLS<<");\n";
                OUT << "\n";
                async_enabled = false;
                break;
            case BNOD:
                node_dim = op.arg[0];
                cur_input_name = op.arg2; cur_output_name = op.arg1;
                cur_node_name = to_lower(to_identifier(referenced_nodes.find(cur_node = op.arg[1])->second.name));
                cur_base_node = names.find(name(cur_node))->second.second;
                if(!condition.has(cur_node)) nodes_rv[name(cur_node)] = cur_output_name;
                stage_node_names.push_back(cur_node_name);
                
                OUT << "/*\n";
                OUT << " * node: " << cur_node << "\n";
                OUT << " * name: " << cur_node_name << "\n";
                OUT << " * type: " << name(cur_node) << "\n";
                OUT << " * dimension: " << node_dim << "\n";
                OUT << " * input name: " << cur_input_name << "\n";
                OUT << " * output name: " << cur_output_name << "\n";
                OUT << " * stage: " << cur_stage_name << "\n";
                OUT << " * is first: " << (op.arg[3] != 0? "yes": "no") << "\n";
                OUT << " */\n";

                OUT << reps("std::vector<", node_dim) << get_full_name(op.d2) << reps(">", node_dim)  << " " << reps("v", node_dim) << cur_input_name << ";\n";
                first_node = op.arg[3] != 0;
                if(first_node) { // non-zero if this node is the first in the set of aliases
                    OUT << reps("std::vector<", node_dim) << get_full_name(op.d1) << reps(">", node_dim)  << " " << reps("v", node_dim) << cur_output_name << ";\n";
                    OUT << reps("std::vector<", node_dim) << "int"                << reps(">", node_dim)  << " " << reps("v", node_dim) << L_VISITED << (node_dim == 0? " = 0": "") << ";\n";
                }
                if(async_enabled) {
                    // Each node has a vector of response readers CARR_xxx and a vector of response pointers OUTP_xxxx
                    OUT << "std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReader<" << get_full_name(op.d1) << ">>> " << L_CARR << ";\n";
                    OUT << "std::vector<" << get_full_name(op.d1) << " *> " << L_OUTPTR <<  ";\n";
                    OUT << "int " << L_BEGIN << " = " << L_STAGE_CALLS << ", " << L_SENT << " = 0;\n";
                }
                break;
            case NSET:
                if(op.arg.size() == 0) break;   // ignore empty index 
                ++loop_level;
                cur_loop_tmp.push_back("");
                {
                    std::string current_loop_size = get_loop_size(this, icode, op.arg, rs_dims, loop_level); 
                    if(node_dim > 0) {
                        std::string size_varname(sfmt() << "Size_" << cur_node_name << "_" << cur_stage << "_" << loop_level);
                        OUT << "auto " << size_varname << " = "  <<  current_loop_size << ";\n";
                        current_loop_size = size_varname;
                        OUT << reps("v", node_dim-loop_level+1) << cur_input_name << ".resize("<< current_loop_size << ");\n";
                        if(first_node) {
                            OUT << reps("v", node_dim-loop_level+1) << cur_output_name << ".resize("<< current_loop_size << ");\n";
                            OUT << reps("v", node_dim-loop_level+1) << L_VISITED << ".resize("<< current_loop_size << (node_dim-loop_level==0? ", 0": "") << ");\n";
                        }
                    }
                    OUT << "for(int I" << loop_level << " = 0, IE" << loop_level << " = " << current_loop_size << "; I" << loop_level << " != IE" << loop_level << "; ++I" << loop_level << ") {\n";
                    ++indent;
                    if(node_dim > 0) {
                        OUT << "auto &" << reps("v", node_dim-loop_level) << cur_input_name << " = " << reps("v", node_dim-loop_level+1) << cur_input_name << "[I" << loop_level << "];\n";
                        OUT << "auto &" << reps("v", node_dim-loop_level) << cur_output_name << " = " << reps("v", node_dim-loop_level+1) << cur_output_name << "[I" << loop_level << "];\n";
                        OUT << "auto &" << reps("v", node_dim-loop_level) << L_VISITED << " = " << reps("v", node_dim-loop_level+1) << L_VISITED << "[I" << loop_level << "];\n";
                    }
                }
                break;
            case CHK:
                if(condition.has(cur_node)) {
                    OUT << "if(" << L_VISITED << " == 0 && ";
                    gc_bexp(out, nodes_rv, rs_dims, condition(cur_node), FTK_AND) << ") {\n";
                    ++indent;
                    OUT << "TRACEC(0, \"Condition triggered for node "<< cur_node_name << "\");\n";
                } else {
                    OUT << "if(" << L_VISITED << " == 0) {\n";
                    ++indent;
                }
                break;
            case ENOD:
                // if visited
                OUT <<  L_VISITED << " = " << cur_node << ";\n";
                --indent;
                OUT << "}\n";
            case EPRP:
                //OUT << "// " << op << "\n";
                while(loop_level > 0) {
                    --indent; 
                    --loop_level;
                    OUT << "}\n";
                    cur_loop_tmp.pop_back();
                }
                if(async_enabled) {
                    OUT << "int " << L_END_X  << " = " << L_STAGE_CALLS << ";\n";

                    OUT << "if(Async_call) {\n";
                    ++indent;
                    // The calls for each node are from BX_xxxx to EX_xxxx but we only keep xxxx_maxcc active at a time
                    // We use the (one based) index in the status/context vetor as a tag for the call
                    OUT << "for(int Ax = " << L_BEGIN << ", Ex = std::min(" << L_END_X << ", " << L_BEGIN << " + " << cur_node_name << "_maxcc); Ax < Ex; ++Ax) {\n";
                    ++indent;
                    OUT << "auto &RPCx = " << L_CARR << "[Ax-" << L_BEGIN << "];\n";
                    OUT << "RPCx->StartCall();\n";
                    OUT << "RPCx->Finish(" << L_OUTPTR << "[Ax-" << L_BEGIN << "], " << L_STATUS << "[Ax].get(), (void *) (long) (Ax+1));\n";
                    OUT << "++" << L_SENT << ";\n";
                    --indent;
                    OUT << "}\n";
                    --indent;
                    OUT << "}\n";
                }
                rs_dims[cur_output_name] = node_dim; 
                cur_node = node_dim = 0; cur_input_name.clear(); cur_output_name.clear();
                first_node = false;
                break;
            case BPRP:
                OUT << "// prepare the "<< op.d1->full_name() << " result for " << entry_dot_name << "\n";
                break;
            case LOOP:
                ++loop_level;
                {
                    std::string current_loop_size =  get_loop_size(this, icode, op.arg, rs_dims, loop_level); 
                    OUT << "for(int I" << loop_level << " = 0, IE" << loop_level << " = " << current_loop_size << "; I" << loop_level << " != IE" << loop_level << "; ++I" << loop_level << ") {\n";
                }
                ++indent;
                if(fd_accessor(op.arg1, op.d1)->message_type() != nullptr) {
                    OUT << "auto &Tmp" << loop_level << " = *" <<  cur_loop_tmp.back() << ::field_accessor(op.arg1, op.d1, rs_dims, LEFT_VALUE, loop_level-1) << ");\n"; 
                    cur_loop_tmp.push_back(sfmt() << "Tmp" << loop_level);
                } else {
                    cur_loop_tmp.push_back(cur_loop_tmp.back()); 
                }
                break;
            case ELP:
                --indent;
                cur_loop_tmp.pop_back();
                --loop_level;
                OUT << "}\n";
                break;
            case FUNC:
                OUT << "// function call\n";
                break;
            case SETI: {
                std::string value;
                if(convert_integer(value, fd_accessor(op.arg1, op.d1), get_value(op.arg[0]), nullptr, get_integer(op.arg[0]))) {
                    OUT << cur_loop_tmp.back() << ::field_accessor(op.arg1, op.d1, rs_dims, LEFT_VALUE, loop_level) << value << ");\n";
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(op.arg[0]), sfmt() << "cannot set this filed to integer value \"" << get_value(op.arg[0]) << "\"");
                }
            } break;
            case SETE: {
                std::string value;
                if(convert_enum(value, fd_accessor(op.arg1, op.d1), get_full_name(enum_descriptor(op.arg[0])), enum_descriptor(op.arg[0])->type(), enum_descriptor(op.arg[0]))) {
                    OUT << cur_loop_tmp.back() << ::field_accessor(op.arg1, op.d1, rs_dims, LEFT_VALUE, loop_level) << value << ");\n";
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(op.arg[0]), sfmt() << "cannot set this field to enum value \"" << get_dotted_id(op.arg[0]) << "\"");
                }
            } break;
            case SETS: {
                std::string value;
                if(convert_string(value, fd_accessor(op.arg1, op.d1), c_escape(get_value(op.arg[0])), nullptr, get_value(op.arg[0]))) {
                    OUT << cur_loop_tmp.back() << ::field_accessor(op.arg1, op.d1, rs_dims, LEFT_VALUE, loop_level) << value << ");\n";
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(op.arg[0]), sfmt() << "cannot set this field to string value \"" << get_dotted_id(op.arg[0]) << "\"");
                }
            } break;
            case SETF: {
                std::string value;
                if(convert_float(value, fd_accessor(op.arg1, op.d1), get_value(op.arg[0]), nullptr)) {
                    OUT << cur_loop_tmp.back() << ::field_accessor(op.arg1, op.d1, rs_dims, LEFT_VALUE, loop_level) << value << ");\n";
                } else {
                    ++error_count;
                    pcerr.AddError(main_file, at(op.arg[0]), sfmt() << "cannot set this field to floating point value \"" << get_value(op.arg[0]) << "\"");
                }
            } break;
            case SET: 
                convert_value(ledp, convert_code, fd_accessor(op.arg1, op.d1), grpc_type_to_ftk(fd_accessor(op.arg2, op.d2)->type()), false);
                OUT << cur_loop_tmp.back() << ::field_accessor(op.arg1, op.d1, rs_dims, LEFT_VALUE, loop_level) << convert_code.first << ::field_accessor(op.arg2, op.d2, rs_dims, RIGHT_VALUE) << convert_code.second << ");\n";
                break;
            case COPY:
                OUT << cur_loop_tmp.back() << ::field_accessor(op.arg1, op.d1, rs_dims, LEFT_STEM, loop_level) << "CopyFrom(" << ::field_accessor(op.arg2, op.d2, rs_dims, RIGHT_VALUE) << ");\n";
                break;
            case SETT:
                OUT << "// set this field from temp var\n";
                break;
            case CALL:
                OUT << "++" << L_STAGE_CALLS << ";\n";
                if(async_enabled) {
                    OUT << "if(Async_call) {\n";
                    ++indent;
                    OUT << L_CONTEXT << ".emplace_back(std::unique_ptr<grpc::ClientContext>(new ::grpc::ClientContext));\n";
                    OUT << L_STATUS << ".emplace_back(std::unique_ptr<grpc::Status>(new ::grpc::Status));\n";
                    OUT << L_OUTPTR << ".emplace_back(&" << cur_output_name << ");\n";
                    OUT << L_CARR << ".emplace_back("
                        << cur_node_name << "_prep(CID, " << L_STAGE_CALLS << " - " << L_BEGIN <<", " << L_QUEUE << ", *"<< L_CONTEXT << ".back(), &" << cur_input_name <<", Debug_Flag, Trace_call));\n";
                    --indent;
                    OUT << "} else {\n";
                    ++indent;
                }
                OUT << "if(CTX->IsCancelled()) {\n";
                ++indent;
                OUT << "ERROR(\"" << entry_dot_name << "/stage " << cur_stage << " (" << cur_stage_name << "): call cancelled\");\n";
                OUT << "return ::grpc::Status(::grpc::StatusCode::CANCELLED, \"Call exceeded deadline or was cancelled by the client\");\n";
                --indent;
                OUT << "}\n";
                OUT << "L_status = " << cur_node_name << "_call(CID, &" << cur_output_name << ", &" << cur_input_name << ", Debug_Flag, Trace_call);\n";
                OUT << "if(!L_status.ok()) return L_status;\n";
                if(async_enabled) {
                    --indent;
                    OUT << "}\n";
                }
                break;
            case COMM:
                if(!op.arg1.empty())
                    OUT << "// " << op.arg1 << "\n";
                if(!op.arg2.empty())
                    OUT << "// " << op.arg2 << "\n";
                break;
            case INDX:
                //OUT << "// " << op << "\n";
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
    for(auto const &ne: names) if(ne.second.first == "entry")
        entry_node_set.insert(ne.second.second);
    
    ServiceDescriptor const *sdp = nullptr;
    int entry_count = 0;
    for(int entry_node: entry_node_set) {
        ++entry_count;
        MethodDescriptor const *mdp = method_descriptor(entry_node);
        if(sdp != nullptr && sdp != mdp->service()) {
            error_count += 1;
            pcerr.AddError(main_file, -1, 0, "all entries must be methods of the same service");
        }
        sdp = mdp->service();
        std::string output_schema = json_schema(mdp->output_type(), decamelize(mdp->output_type()->name()), description(entry_node));
        std::string input_schema = json_schema(mdp->input_type(), to_upper(to_option(main_name)), main_description);
        append(vars, "ENTRY_FULL_NAME", mdp->full_name());
        append(vars, "ENTRY_NAME", mdp->name());
        append(vars, "ENTRY_URL", sfmt() << "/" << mdp->name());
        append(vars, "ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
        append(vars, "ENTRY_TIMEOUT", std::to_string(get_blck_timeout(entry_node, default_entry_timeout)));
        append(vars, "ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
        append(vars, "ENTRY_OUTPUT_SCHEMA_JSON_C", c_escape(output_schema));
        append(vars, "ENTRY_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "ENTRY_INPUT_SCHEMA_JSON_C", c_escape(input_schema));
        append(vars, "ENTRY_DESCRIPTION", description(entry_node));
        append(vars, "ENTRY_DESCRIPTION_HTML", html_escape(description(entry_node)));
        if(entry_count == 1) {
            append(vars, "MAIN_ENTRY_FULL_NAME", mdp->full_name());
            append(vars, "MAIN_ENTRY_NAME", mdp->name());
            append(vars, "MAIN_ENTRY_URL", sfmt() << "/" << mdp->name());
            append(vars, "MAIN_ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
            append(vars, "MAIN_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
            append(vars, "MAIN_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
            append(vars, "MAIN_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(entry_node, default_entry_timeout)));
            append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
            append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON_C", c_escape(output_schema));
            append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON", input_schema);
            append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON_C", c_escape(input_schema));
            append(vars, "MAIN_ENTRY_DESCRIPTION", description(entry_node));
            append(vars, "MAIN_ENTRY_DESCRIPTION_HTML", html_escape(description(entry_node)));
        } else {
            append(vars, "ALT_ENTRY_FULL_NAME", mdp->full_name());
            append(vars, "ALT_ENTRY_NAME", mdp->name());
            append(vars, "ALT_ENTRY_URL", sfmt() << "/" << mdp->name());
            append(vars, "ALT_ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
            append(vars, "ALT_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
            append(vars, "ALT_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
            append(vars, "ALT_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(entry_node, default_entry_timeout)));
            append(vars, "ALT_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
            append(vars, "ALT_ENTRY_OUTPUT_SCHEMA_JSON_C", c_escape(output_schema));
            append(vars, "ALT_ENTRY_INPUT_SCHEMA_JSON", input_schema);
            append(vars, "ALT_ENTRY_INPUT_SCHEMA_JSON_C", c_escape(input_schema));
            append(vars, "ALT_ENTRY_DESCRIPTION", description(entry_node));
            append(vars, "ALT_ENTRY_DESCRIPTION_HTML", html_escape(description(entry_node)));
        }
    }
    if(entry_count > 1)
        set(vars, "HAVE_ALT_ENTRY", "");
    return error_count;
}
int flow_compiler::set_cli_active_node_vars(decltype(global_vars) &vars, int cli_node) {
    int error_count = 0;
    auto rn = referenced_nodes.find(cli_node);
    assert(rn != referenced_nodes.end());
    std::string const &node_name = rn->second.name;

    clear(global_vars, "ALT_ENTRY_NAME");

    append(vars, "HAVE_ACTIVE_NODE", "");
    append(vars, "ACTIVE_NODE_UPPER", to_upper(node_name));
    append(vars, "MAIN_ENTRY_DESCRIPTION", description(cli_node));
    append(vars, "MAIN_ENTRY_DESCRIPTION_HTML", html_escape(description(cli_node)));
    append(vars, "MAIN_ENTRY_NAME", node_name);
    append(vars, "MAIN_ENTRY_URL", sfmt() << "/-node/" << node_name);
    append(vars, "MAIN_ENTRY_ID", to_lower(to_identifier(node_name)));
    append(vars, "MAIN_ENTRY_UPPERID", to_upper(to_identifier(node_name)));
    auto mdp = method_descriptor(cli_node);
    append(vars, "MAIN_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
    append(vars, "MAIN_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
    std::string output_schema = json_schema(mdp->output_type(), decamelize(mdp->output_type()->name()), description(cli_node));
    std::string input_schema = json_schema(mdp->input_type(), node_name, description(cli_node));
    append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
    append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON_C", c_escape(output_schema));
    append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON", input_schema);
    append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON_C", c_escape(input_schema));
    append(vars, "MAIN_ENTRY_METHOD_NAME", mdp->name());
    append(vars, "MAIN_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(cli_node, default_node_timeout)));
    return error_count;
}
int flow_compiler::set_cli_node_vars(decltype(global_vars) &vars) {
    int error_count = 0, node_count = 0;
    for(auto &rn: referenced_nodes) {
        ++node_count;
        auto cli_node = rn.first;
        if(type(cli_node) == "container" || method_descriptor(cli_node) == nullptr) 
            continue;
        std::string const &node_name = rn.second.name;
       
        append(vars, "CLI_NODE_LINE", sfmt() << at(cli_node).token.line);
        append(vars, "CLI_NODE_DESCRIPTION", description(cli_node));
        append(vars, "CLI_NODE_DESCRIPTION_HTML", html_escape(description(cli_node)));
        append(vars, "CLI_NODE_NAME", node_name);
        append(vars, "CLI_NODE_URL", sfmt() << "/-node/" << node_name);
        append(vars, "CLI_NODE_ID", to_lower(to_identifier(node_name)));
        append(vars, "CLI_NODE_UPPERID", to_upper(to_identifier(node_name)));
        auto mdp = method_descriptor(cli_node);
        append(vars, "CLI_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "CLI_GRPC_SERVICE_NAME", mdp->service()->name());
        append(vars, "CLI_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "CLI_INPUT_TYPE", get_full_name(mdp->input_type()));
        std::string output_schema = json_schema(mdp->output_type(), decamelize(mdp->output_type()->name()), description(cli_node));
        std::string input_schema = json_schema(mdp->input_type(), node_name, description(cli_node));
        append(vars, "CLI_OUTPUT_SCHEMA_JSON", output_schema);
        append(vars, "CLI_OUTPUT_SCHEMA_JSON_C", c_escape(output_schema));
        append(vars, "CLI_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "CLI_INPUT_SCHEMA_JSON_C", c_escape(input_schema));
        append(vars, "CLI_METHOD_NAME", mdp->name());
        append(vars, "CLI_NODE_TIMEOUT", std::to_string(get_blck_timeout(cli_node, default_node_timeout)));
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
int flow_compiler::gc_server(std::ostream &out) {
    int error_count = 0;
    decltype(global_vars) local_vars;

    std::set<int> entry_node_set;
    // Sort the entries in source order
    for(auto const &ne: names) if(ne.second.first == "entry") 
        entry_node_set.insert(ne.second.second);

    for(int entry_node: entry_node_set) {
        std::stringstream sbuf;
        error_count += gc_server_method(sbuf,  method_descriptor(entry_node)->full_name(), entry_node);
        append(local_vars, "ENTRY_CODE", sbuf.str());
    }
    ServiceDescriptor const *sdp =  method_descriptor(*entry_node_set.begin())->service();
    set(local_vars, "CPP_SERVER_BASE", get_full_name(sdp));
#if 0
    std::cerr << "** server * global **********************************\n";
    std::cerr << join(global_vars, "\n") << "\n";
    std::cerr << "** server * local ***********************************\n";
    std::cerr << join(local_vars, "\n") << "\n";
    std::cerr << "*****************************************************\n";
#endif
    extern char const *template_server_C;
    render_varsub(out, template_server_C, global_vars, local_vars);

    return error_count;
}
