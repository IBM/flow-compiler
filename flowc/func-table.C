#include "value-type.H"
#include "container-strinsop.H" 
#include "flow-parser.h"
#include "func-table.H"

#include <cassert>
#include <map>
#include <tuple>
#include <string>
#include <iostream>

namespace fc {
value_type op1_type(int op, value_type l) {
    value_type vt;
    if(!l.is_null()) switch(op) {
        case FTK_MINUS:
            if(l.type == fvt_int) 
                vt = value_type(fvt_int);
            else if(l.is_basic()) 
                vt = value_type(fvt_flt);
            break;
        default:
            std::cerr << "internal " << __FUNCTION__ << " not implemented for: " << op << "\n";
            assert(false);
            break;
    }
    return vt;
}
value_type op2_type(int op, value_type l, value_type r) {
    value_type vt;
    if(!l.is_null() && !r.is_null()) switch(op) {
        case FTK_PLUS:
            if(l.type == r.type && l.is_basic()) 
                vt = l;
            else if(l.is_basic() && r.is_basic())
                vt = value_type(l.is_str()? fvt_str: fvt_flt);
            else if(l.elem_type().type == r.type)
                vt = l;
            else if(l.type == r.elem_type().type)
                vt = r;
            else if(l.is_array() && l.elem_type().type == r.elem_type().type)
                vt = l;
            break;
        case FTK_STAR:   
            if(l.is_str() && r.is_num())
                vt = value_type(fvt_str);
            else if(l.is_int() && r.is_int())
                vt = value_type(fvt_int);
            else if(l.is_num() && r.is_num())
                vt = value_type(fvt_flt);
            break;
        case FTK_PERCENT: 
        case FTK_SLASH:
        case FTK_POW:
            if(l.is_int() && r.is_int())
                vt = value_type(fvt_int);
            else if(l.is_num() && r.is_num())
                vt = value_type(fvt_flt);
            break;
        default:
            std::cerr << "internal " << __FUNCTION__ << " not implemented for: " << op << "\n";
            assert(false);
            break;
    }
    return vt;
}
/**
 * Function definition table
 * name -> preserve const return type, min num args, arg types, preserve const
 */
std::multimap<std::string, fun_info_t> supp_fun_table = {
    {"after",     { true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_str)})} },
    {"batch",     { true, value_type(2, value_type(fvt_any)), 2, value_type({value_type(1, value_type(fvt_any)), value_type(fvt_int)})} },
    {"before",    { true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_str)})} },
    {"ceil",      { true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)})} },
    {"exp",       { true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)})} },
    {"float",     { true, value_type(fvt_flt), 1, value_type({value_type(fvt_basic)})} },
    {"floor",     { true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)})} },
    {"flt",       { true, value_type(fvt_flt), 1, value_type({value_type(fvt_basic)})} },
    {"int",       { true, value_type(fvt_int), 1, value_type({value_type(fvt_basic)})} },
    {"integer",   { true, value_type(fvt_int), 1, value_type({value_type(fvt_basic)})},  },
    {"join",      { true, value_type(fvt_str), 2, value_type({value_type(1, value_type(fvt_basic)), value_type(fvt_str)})}, },
    {"join",      { true, value_type(fvt_str), 2, value_type({value_type(1, value_type(fvt_basic)), value_type(1, value_type(fvt_str))})}, },
    {"length",    { true, value_type(fvt_int), 1, value_type({value_type(fvt_str)})} },
    {"ln",        { true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)})}, },
    {"now",       { false, value_type(fvt_int), 0, value_type()}, },
    {"rand",      { false, value_type(fvt_flt), 0, value_type({value_type(fvt_int)})}, },
    {"remainder", { true, value_type(fvt_flt), 2, value_type({value_type(fvt_int), value_type(fvt_int)})}, },
    {"round",     { true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)})},  },
    {"split",     { true, value_type(1, value_type(fvt_str)), 1, value_type({value_type(fvt_str), value_type(fvt_str)})} },
    {"str",       { true, value_type(fvt_str), 1, value_type({value_type(fvt_basic)})} },
    {"substr",    { true, value_type(fvt_str), 3, value_type({value_type(fvt_str), value_type(fvt_int), value_type(fvt_int)})},  },
    {"tocname",   { true, value_type(fvt_str), 1, value_type({value_type(fvt_str)})} },
    {"toid",      { true, value_type(fvt_str), 1, value_type({value_type(fvt_str)})} },
    {"tolower",   { true, value_type(fvt_str), 1, value_type({value_type(fvt_str)})} },
    {"toupper",   { true, value_type(fvt_str), 1, value_type({value_type(fvt_str)})} },
    {"trunc",     { true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)})} },
};
static fun_info_t no_func;
fun_info_t const &fun_first_match(std::string fun_name) {
    for(auto pp = supp_fun_table.equal_range(fun_name); pp.first != pp.second; ++pp.first) 
        return pp.first->second;

    assert(false);
    return no_func;
}

value_type fun_type(std::string fname, std::vector<value_type> const &avt, bool allow_promotions) {
    static int show = 0;
    if(++show == 1) 
        std::cerr << supp_fun_table  << "\n";
    
    value_type vt;
    for(auto pp = supp_fun_table.equal_range(fname); pp.first != pp.second; ++pp.first) {
        int rdim = std::get<3>(pp.first->second).can_be_called_with(value_type(avt.begin(), avt.end()), allow_promotions);
        if(rdim < 0) 
            continue;
        vt = value_type(rdim, std::get<1>(pp.first->second));
        break;
    }
    return vt;
}
}
