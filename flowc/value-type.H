#ifndef H_VALUE_TYPE_H
#define H_VALUE_TYPE_H

#include <string>
#include <vector>

namespace fc {

enum fvt_type {
    fvt_none = 0,
    fvt_int, fvt_flt, fvt_str,  
    fvt_enum, 
    fvt_array, 
    fvt_struct,
};

struct value_type {
    fvt_type type;
    std::vector<value_type> inf;
    // group name - the typename of the structure or the enum this is part of
    std::string gname;
    // filed name - the filed name when this is a structure member
    std::string fname;
    // reference information (the node family name)
    std::string reference;

    // copy constructor
    value_type(value_type const &a_val);
    // basic type 
    value_type(fvt_type t=fvt_none, std::string group_name="", std::string field_name="");
    // array of arbitrary type
    value_type(int array_dim, value_type element_type);

    void add_type(value_type const &t) {
        inf.push_back(t);
    }
    void add_type(value_type &&t) {
        inf.push_back(t);
    }
    std::string to_string() const;
    std::string struct_name() const { return gname; }
    std::string enum_name() const { return gname; }
    std::string field_name() const { return fname; }
    std::string ref() const { return reference; }
    bool is_basic() const { return type == fvt_int || type == fvt_flt || type == fvt_str; }
    bool is_null() const { return type == fvt_none; }
    bool is_str() const { return type == fvt_str; }
    bool is_num() const { return type == fvt_int || type == fvt_flt; }
    bool is_int() const { return type == fvt_int; }
    bool is_flt() const { return type == fvt_int; }
    bool is_array() const { return type == fvt_array; }
    bool is_struct() const { return type == fvt_struct; }
    value_type a_type() const { return type == fvt_array? inf[0]: value_type(); }
    value_type field_type(std::string field_name) const;
    bool operator ==(value_type const &other) const;
    bool operator !=(value_type const &other) const {
        return !(*this == other);
    }
    int dimension() const {
        return !is_array()? 0:
            1+a_type().dimension();
    }
    bool can_assign_from(value_type const &right) const;
    bool can_assign_to(value_type const &left) const;
};
}
#endif
