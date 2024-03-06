#include <string>

#include <ostream>
#include <string>
#include "value-type.H"
#include "ansi-escapes.H"

namespace fc {
/*
value_type::value_type(value_type const &a_val):
    type(a_val.type), inf(a_val.inf), gname(a_val.gname), fname(a_val.fname) {
}
*/
value_type::value_type(fvt_type t, std::string group_name, std::string field_name, std::string ref):
    type(t), gname(group_name), fname(field_name), reference(ref) {
}
/*
value_type::value_type(fvt_type t, std::initializer_list<value_type> tl, std::string gn):
    type(t), inf(tl), gname(gn) { 
}
*/
value_type::value_type(int array_dim, value_type element_type) {
    if(array_dim <= 0) {
        type = element_type.type;
        inf = element_type.inf;
        gname = element_type.gname;
    } else {
        type = fvt_array;
        inf.push_back(value_type(array_dim-1, element_type));
    }
}
value_type value_type::field_type(std::string field_name) const {
    for(auto const &vt: inf) 
        if(vt.fname == field_name)
            return vt;
    return value_type();
}
bool value_type::operator ==(value_type const &other) const {
    switch(type) {
        case fvt_none: case fvt_int: case fvt_flt: case fvt_str: case fvt_enum: case fvt_any: case fvt_basic: case fvt_num:
            return type == other.type;
            // return type == other.type || other.type == fvt_any || type == fvt_any && (
            // other.type == fvt_none || other.type == fvt_int || other.type == fvt_flt || other.type == fvt_enum
            // )
        case fvt_array:
            return other.type == fvt_array && inf[0] == other.inf[0];
        case fvt_struct:
            if(other.type != fvt_struct || inf.size() != other.inf.size())
                return false;
            for(unsigned t = 0, e = inf.size(); t < e; ++t)
                if(inf[t] != other.inf[t])
                    return false;
            return true;
    }
    return false;
}
bool value_type::can_assign_to(value_type const &left) const {
    return left.can_assign_from(*this);
}
bool value_type::can_assign_from(value_type const &right) const {
    switch(type) {
        case fvt_array:
            return right.type == fvt_array && inf[0].can_assign_from(right.inf[0]);
        case fvt_struct:
            if(right.type != fvt_struct)
                break;
            break;
        default:
            return type == right.type;
    }
    bool can_assign = true;
    for(auto const &lfvt: inf) 
        for(auto const &rfvt: right.inf) 
            if(lfvt.fname == rfvt.fname)
                can_assign = can_assign && lfvt.can_assign_from(rfvt);
        
    return can_assign;
}
}
std::ostream &operator << (std::ostream &s, fc::value_type const &vt) {
    if(!vt.ref().empty())
        s << ANSI_BLUE+ANSI_BOLD << vt.ref() << ANSI_RESET<< "@";
    if(!vt.field_name().empty()) 
        s << vt.field_name() << ": ";
    switch(vt.type) {
        case fc::fvt_none:
            s << ANSI_RED+ANSI_BOLD << "??" << ANSI_RESET;
            break;
        case fc::fvt_any:
            s << ANSI_MAGENTA+ANSI_BOLD << "*" << ANSI_RESET;
            break;
        case fc::fvt_basic:
            s << ANSI_MAGENTA+ANSI_BOLD << "b" << ANSI_RESET;
            break;
        case fc::fvt_num:
            s << ANSI_MAGENTA+ANSI_BOLD << "n" << ANSI_RESET;
            break;
        case fc::fvt_int:
            s << ANSI_CYAN+ANSI_BOLD << "int" << ANSI_RESET;
            break;
        case fc::fvt_flt:
            s << ANSI_YELLOW+ANSI_BOLD << "flt" << ANSI_RESET;
            break;
        case fc::fvt_str:
            s << ANSI_GREEN+ANSI_BOLD << "str" << ANSI_RESET;
            break;
        case fc::fvt_enum:
            s << "enum: " << ANSI_CYAN+ANSI_BOLD << vt.enum_name() << ANSI_RESET;
            break;
        case fc::fvt_array:
            s << ANSI_BOLD << "[" << ANSI_RESET << vt.inf[0] << ANSI_BOLD << "]" << ANSI_RESET;
            break;

        case fc::fvt_struct:
            if(!vt.struct_name().empty()) 
                s << ANSI_MAGENTA+ANSI_BOLD << vt.struct_name() << ANSI_RESET;
            s << "(";
            for(unsigned u = 0, e = vt.inf.size(); u < e; ++u) {
                if(u > 0) s << ", ";
                s << vt.inf[u];
            }
            s << ")";
            break;
    }
    return s;
}
