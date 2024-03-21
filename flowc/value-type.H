#ifndef H_VALUE_TYPE_H
#define H_VALUE_TYPE_H

#include <string>
#include <vector>
#include <map>
#include <initializer_list>
#include <iosfwd>

namespace fc {

enum fvt_type {
    fvt_none = 0,
    fvt_any, fvt_basic, fvt_num,
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

    // basic type 
    value_type(fvt_type t=fvt_none, std::string group_name="", std::string field_name="", std::string ref="");
    // array of arbitrary type
    value_type(int array_dim, value_type element_type);
    // struct of arbitrary types
    value_type(std::initializer_list<value_type> stypes): type(fvt_struct), inf(stypes) {
    };
    template<class IT>
    value_type(IT be, IT en): type(fvt_struct), inf(be, en) {
    };
    void add_type(value_type const &t) {
        inf.push_back(t);
    }
    void add_type(value_type &&t) {
        inf.push_back(t);
    }
    // serialize to string
    std::string to_string() const;
    
    bool is_array() const { return type == fvt_array; }
    bool is_basic() const { return type == fvt_int || type == fvt_flt || type == fvt_str; }
    bool is_flt() const { return type == fvt_int; }
    bool is_int() const { return type == fvt_int; }
    bool is_null() const { return type == fvt_none; }
    bool is_num() const { return type == fvt_int || type == fvt_flt; }
    bool is_str() const { return type == fvt_str; }
    bool is_struct() const { return type == fvt_struct; }
    /*** Accessors
     */
    std::string struct_name() const { return gname; }
    std::string enum_name() const { return gname; }
    std::string field_name() const { return fname; }
    std::string ref() const { return reference; }
    // whether this is a struct with named fields
    bool has_field_names() const;
    // the number of fields if this is a struct, 0 otherwise
    unsigned field_count() const { return is_struct()? inf.size(): 0; }
    // the array element type
    value_type elem_type() const { return type == fvt_array? inf[0]: value_type(); }
    // the element type of multi-dimensional arrays
    value_type zd_type() const { return type == fvt_array? elem_type().zd_type(): *this; }
    // whether this is an array of <e>
    bool is_array_of(value_type const &e) const { return is_array() && zd_type() == e; }
    // field type by name
    value_type field_type(std::vector<std::string> field_names) const;
    value_type field_type(std::string field_name) const;
    // field type by index
    value_type field_type(unsigned field_x) const {
        return field_x < field_count()? inf[field_x]: value_type();
    }
    // number of dimensions if this is an array type
    int dimension() const;
    bool operator ==(value_type const &other) const;
    bool operator !=(value_type const &other) const {
        return !(*this == other);
    }
    // assignment compatibilty
    bool can_assign_from(value_type const &right, bool allow_promotions=false) const;
    bool can_assign_to(value_type const &left, bool allow_promotions=false) const;
    // whether <values> can be mathched to this argument list, return the number of dimensions or -1 if incompatible
    int can_be_called_with(value_type const &values, bool allow_promotions, int min_argc=-1, std::vector<int> *arg_dims=nullptr) const;
    // wheter this struct can be set unambiguously from values 
    int can_be_set_with(value_type const &values, bool allow_promotions, std::map<std::string, int> *arg_dims=nullptr) const;
    // change any reference to fvt_any to nt
    value_type retype(value_type const &nt);
};
}

std::ostream &operator << (std::ostream &s, fc::value_type const &vt);

#endif
