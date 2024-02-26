#include <string>

#include "value-type.H"

namespace fc {

value_type::value_type(value_type const &a_val):
    type(a_val.type), inf(a_val.inf), gname(a_val.gname), fname(a_val.fname) {
}
value_type::value_type(fvt_type t, std::string gn):type(t), gname(gn) {
}
value_type::value_type(fvt_type t, std::initializer_list<value_type> tl, std::string gn):
    type(t), inf(tl), gname(gn) { 
}
value_type::value_type(int array_dim, value_type element_type) {
    if(array_dim < 0) {
        type = element_type.type;
        inf = element_type.inf;
        gname = element_type.gname;
    } else {
        type = fvt_array;
        inf.push_back(value_type(array_dim-1, element_type));
    }
}

bool value_type::operator ==(value_type const &other) const {
    switch(type) {
        case fvt_none: case fvt_int: case fvt_flt: case fvt_str: case fvt_enum:
            return type == other.type;
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

}
