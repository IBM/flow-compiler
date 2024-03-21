#include <cassert>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include "ansi-escapes.H"
#include "value-type.H"
#include "container-strinsop.H"

namespace fc {
value_type::value_type(fvt_type t, std::string group_name, std::string field_name, std::string ref):
    type(t), gname(group_name), fname(field_name), reference(ref) {
}
value_type::value_type(int array_dim, value_type element_type) {
    if(array_dim <= 0) {
        type = element_type.type;
        inf = element_type.inf;
        gname = element_type.gname;
        reference = element_type.reference;
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
value_type value_type::field_type(std::vector<std::string> field_names) const {
    int dim = 0;
    value_type vt = *this;
    for(unsigned fi = 0, fc = field_names.size(); !vt.is_null() && fi < fc; ++fi) {
        vt = vt.field_type(field_names[fi]);
        if(vt.dimension() > 0) {
            dim += vt.dimension();
            vt = vt.zd_type();
        }
    }
    if(vt.is_null())
        return value_type();
    return value_type(dim, vt);
}
bool value_type::has_field_names() const {
    bool hfn = field_count() > 0;
    for(auto const &vt: inf)
        if((hfn = hfn && !vt.fname.empty()))
            break;
    return hfn;
}
int value_type::dimension() const {
    return !is_array()? 0:
        1+elem_type().dimension();
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
bool value_type::can_assign_to(value_type const &left, bool allow_promotions) const {
    return left.can_assign_from(*this, allow_promotions);
}
bool value_type::can_assign_from(value_type const &right, bool allow_promotions) const {
    //std::cerr << "CAN " << *this << "\n    " << right << ":\n";
    bool can_assign = true;
    switch(type) {
        case fvt_any:
            break;
        case fvt_basic:
            can_assign = right.type == fvt_str || right.type == fvt_flt || right.type == fvt_int;
            break;
        case fvt_num:
            can_assign = right.type == fvt_int || right.type == fvt_flt || right.type == fvt_enum;
            break;
        case fvt_array:
            //std::cerr << "CASE 1 arrays?\n";
            can_assign = right.type == fvt_array && inf[0].can_assign_from(right.inf[0], allow_promotions);
            break;
        case fvt_struct:
            if(has_field_names() && right.has_field_names()) {
                // match by field name
                //std::cerr << "CASE 2a struct by field name\n";
                for(unsigned fi = 0, fe = field_count(); can_assign && fi < fe; ++fi)
                    can_assign = can_assign && field_type(fi).can_assign_from(right.field_type(field_type(fi).field_name()), allow_promotions);
            } else {
                // match in order
                //std::cerr << "CASE 2a struct in order\n";
                for(unsigned fi = 0, fe = field_count(); can_assign && fi < fe; ++fi)
                    can_assign = can_assign && field_type(fi).can_assign_from(right.field_type(fi), allow_promotions);
            }
            break;
        case fvt_flt:
            can_assign = right.type == fvt_flt || (right.type == fvt_int || right.type == fvt_enum) && allow_promotions;
            break;
        case fvt_int:
            can_assign = right.type == fvt_int || right.type == fvt_enum && allow_promotions;
            break;
        default:
            //std::cerr << "CASE d any\n";
            can_assign = type == right.type;
            break;
    }
    //std::cerr << (can_assign? "yes": "no") << "\n";
    return can_assign;
}
/***
 * The left struct members need to be assignable from the right side members either directly or
 * repeatedly from an array.
 */
int value_type::can_be_called_with(value_type const &values, bool allow_promotions, int min_argc, std::vector<int> *arg_dims) const {
    std::vector<int> tmpad, &ads = arg_dims == nullptr? tmpad: *arg_dims;
    if(!is_struct() || !values.is_struct()) {
        std::cerr << "internal error: both types need to be struct\n"
                     "left is " << *this << "\nright is " << values << "\n"; 
        assert(false);
    }
    int pdim = 0;
    unsigned match_count = 0;
    if(min_argc < 0 && field_count() == values.field_count() ||
            min_argc >= 0 && values.field_count() >= min_argc && values.field_count() <= field_count())  
    for(unsigned f = 0, fe = values.field_count(); f < fe; ++f) {
        auto const &lf = field_type(f);
        int dim = 0; 
        bool can_assign = false;
        for(value_type rf = values.field_type(f); !(can_assign = lf.can_assign_from(rf, allow_promotions)) && rf.is_array(); rf = rf.elem_type())
            ++dim;
        if(!can_assign) 
            break;
        pdim = std::max(pdim, dim);
        ads.push_back(dim);
        ++match_count;
    }
    if(match_count != values.field_count())
        pdim = -1;
    return pdim;
}
int value_type::can_be_set_with(value_type const &values, bool allow_promotions, std::map<std::string, int> *arg_dims) const {
    //std::cerr << "CAN MASS " << *this << "\n";
    //std::cerr << "    FROM " << values << "\n";
    int pdim = -1;
    std::map<std::string, int> tmpad, &ads = arg_dims == nullptr? tmpad: *arg_dims;
    if(!is_struct() || !values.is_struct()) {
        std::cerr << "internal error: both types need to be of type struct\n"
                     "left is " << *this << "\nright is " << values << "\n"; 
        assert(false);
    }
    unsigned match_count = 0;
    if(field_count() >= values.field_count()) for(unsigned f = 0, fe = values.field_count(); f < fe; ++f) {
        std::string f_name = values.field_type(f).field_name();
        auto lf = field_type(f_name);
        int dim = 0; 

        bool can_assign = false;
        for(value_type rf = values.field_type(f); !(can_assign = lf.can_assign_from(rf, allow_promotions)) && rf.is_array(); rf = rf.elem_type())
            ++dim;

        if(!can_assign) {
            pdim = -1;
            break;
        }
        pdim = std::max(pdim, dim);
        ads[f_name] = dim;
        ++match_count;
    }
    if(match_count != values.field_count())
        pdim = -1;
    //std::cerr << (pdim >= 0? "yes": "no") << " " << pdim << " " << ads << "\n";
    return pdim;
}
std::string value_type::to_string() const {
    std::ostringstream out;
    out << ansi::off << to_string();
    return out.str();
}
value_type value_type::retype(value_type const &nt) {
    if(type == fvt_array) 
        return value_type(dimension(), zd_type().retype(nt));
    return type == fvt_any? nt: *this;
}
}
std::ostream &operator << (std::ostream &s, fc::value_type const &vt) {
    if(!vt.ref().empty())
        s << ANSI_BLUE+ANSI_BOLD << vt.ref() << ANSI_RESET<< "@";
    if(!vt.field_name().empty()) 
        s << vt.field_name() << ": ";
    switch(vt.type) {
        case fc::fvt_none:
            s << ANSI_RED+ANSI_BOLD << "ø" << ANSI_RESET;
            break;
        case fc::fvt_any:
            s << ANSI_MAGENTA+ANSI_BOLD << "\u2200" << ANSI_RESET;
            break;
        case fc::fvt_basic:
            s << ANSI_MAGENTA+ANSI_BOLD << "\u27E1" << ANSI_RESET;
            break;
        case fc::fvt_num:
            s << ANSI_MAGENTA+ANSI_BOLD << "ℝ" << ANSI_RESET;
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
            s << ANSI_CYAN+ANSI_BOLD << "\u2026" << vt.enum_name() << ANSI_RESET;
            break;
        case fc::fvt_array:
            s << ANSI_BOLD << '[' << ANSI_RESET << vt.inf[0] << ANSI_BOLD << ']' << ANSI_RESET;
            break;

        case fc::fvt_struct:
            s << ANSI_BOLD << '(' << ANSI_RESET;
            if(!vt.struct_name().empty()) 
                s << ANSI_MAGENTA+ANSI_BOLD << vt.struct_name() << " "<< ANSI_RESET;
            for(unsigned u = 0, e = vt.inf.size(); u < e; ++u) {
                if(u > 0) s << ", ";
                s << vt.inf[u];
            }
            s << ANSI_BOLD << ')' << ANSI_RESET;
            break;
    }
    return s;
}
