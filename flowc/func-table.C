#include "value-type.H"
#include "container-strinsop.H" 
#include "flow-parser.h"
#include "func-table.H"
#include "json-ostream.H"

#include <cassert>
#include <map>
#include <tuple>
#include <string>
#include <iostream>

namespace fc {
/**
 * Function definition table
 * name -> preserve const return type, min num args, arg types
 */
static std::vector<fun_info_t> const supp_fun_table = {
    {"after",      true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_str)}) },
    {"before",     true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_str)}) },
    {"ceil",       true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)}) },
    {"ceil",       true, value_type(fvt_int), 1, value_type({value_type(fvt_int)}) },
    {"exp",        true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)}) },
    {"float",      true, value_type(fvt_flt), 1, value_type({value_type(fvt_basic)}) },
    {"floor",      true, value_type(fvt_int), 1, value_type({value_type(fvt_int)}) },
    {"floor",      true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)}) },
    {"flt",        true, value_type(fvt_flt), 1, value_type({value_type(fvt_basic)}) },
    {"int",        true, value_type(fvt_int), 1, value_type({value_type(fvt_basic)}) },
    {"integer",    true, value_type(fvt_int), 1, value_type({value_type(fvt_basic)}),  },
    {"join",       true, value_type(fvt_str), 2, value_type({value_type(1, value_type(fvt_basic)), value_type(fvt_str)}), },
    {"join",       true, value_type(fvt_str), 2, value_type({value_type(1, value_type(fvt_basic)), value_type(1, value_type(fvt_str))}), },
    {"length",     true, value_type(fvt_int), 1, value_type({value_type(fvt_str)}) },
    {"ln",         true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)}), },
    {"now",        false, value_type(fvt_int), 0, value_type(), },
    {"rand",       false, value_type(fvt_flt), 0, value_type({value_type(fvt_flt)}), },
    {"remainder",  true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"remainder",  true, value_type(fvt_flt), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"round",      true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)}),  },
    {"round",      true, value_type(fvt_int), 1, value_type({value_type(fvt_int)}),  },
    {"split",      true, value_type(1, value_type(fvt_str)), 1, value_type({value_type(fvt_str), value_type(fvt_str)}) },
    {"str",        true, value_type(fvt_str), 1, value_type({value_type(fvt_basic)}) },
    {"substr",     true, value_type(fvt_str), 3, value_type({value_type(fvt_str), value_type(fvt_int), value_type(fvt_int)}),  },
    {"tocname",    true, value_type(fvt_str), 1, value_type({value_type(fvt_str)}) },
    {"toid",       true, value_type(fvt_str), 1, value_type({value_type(fvt_str)}) },
    {"tolower",    true, value_type(fvt_str), 1, value_type({value_type(fvt_str)}) },
    {"toupper",    true, value_type(fvt_str), 1, value_type({value_type(fvt_str)}) },
    {"trunc",      true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)}) },
    {"trunc",      true, value_type(fvt_int), 1, value_type({value_type(fvt_int)}) },

    {"repeated",   true, value_type(1, fvt_any), 2, value_type({value_type(fvt_int), value_type(fvt_any)}), },
    {"batch",      true, value_type(2, value_type(fvt_any)), 2, value_type({value_type(1, value_type(fvt_any)), value_type(fvt_int)}) },
    {"slice",      true, value_type(1, fvt_any), 2, value_type({value_type(1, fvt_any), value_type(fvt_int), value_type(fvt_int)}), },

    {"+",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"+",    true, value_type(fvt_flt), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"+",    true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_basic)}), },
    {"-",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"-",    true, value_type(fvt_flt), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"-",    true, value_type(fvt_flt), 1, value_type({value_type(fvt_flt)}) },
    {"-",    true, value_type(fvt_int), 1, value_type({value_type(fvt_int)}) },
    {"*",    true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_int)}), },
    {"*",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"*",    true, value_type(fvt_flt), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"/",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"/",    true, value_type(fvt_flt), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"%",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"%",    true, value_type(fvt_flt), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"**",   true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"**",   true, value_type(fvt_flt), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },

    {"==",   true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"==",   true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"==",   true, value_type(fvt_int), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {"!=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"!=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"!=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {">=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {">=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {">=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {"<=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"<=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"<=",   true, value_type(fvt_int), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {">",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {">",    true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {">",    true, value_type(fvt_int), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {"<",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"<",    true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"<",    true, value_type(fvt_int), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {"<=>",   true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"<=>",   true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"<=>",   true, value_type(fvt_int), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {"&&",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"&&",    true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"&&",    true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {"||",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"||",    true, value_type(fvt_int), 2, value_type({value_type(fvt_flt), value_type(fvt_flt)}), },
    {"||",    true, value_type(fvt_str), 2, value_type({value_type(fvt_str), value_type(fvt_str)}), },

    {"!",     true, value_type(fvt_int), 1, value_type({value_type(fvt_int)}) },
    {"!",     true, value_type(fvt_int), 1, value_type({value_type(fvt_flt)}) },

    {"~",     true, value_type(fvt_int), 1, value_type({value_type(fvt_int)}) },
    {"|",     true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"&",     true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"^",     true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },

    {">>",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {">>",    true, value_type(1, fvt_any), 2, value_type({value_type(1, fvt_any), value_type(fvt_int)}), },
    {"<<",    true, value_type(fvt_int), 2, value_type({value_type(fvt_int), value_type(fvt_int)}), },
    {"<<",    true, value_type(1, fvt_any), 2, value_type({value_type(1, fvt_any), value_type(fvt_int)}), },

    {"ror",   true, value_type(1, fvt_any), 1, value_type({value_type(1, fvt_any), value_type(fvt_int)}), },
    {"rol",   true, value_type(1, fvt_any), 1, value_type({value_type(1, fvt_any), value_type(fvt_int)}), },

    {"#",     true, value_type(fvt_int), 1, value_type({value_type(1, fvt_any)}) },
    {"?",     true, value_type(fvt_any), 3, value_type({value_type(fvt_int), value_type(fvt_any), value_type(fvt_any)}),  },
};
static fun_info_t const no_func = 
    {"", false, value_type(), 0, value_type(),  };

fun_info_t const &fun_first_match(std::string fun_name) {
    for(fun_info_t const &pp: supp_fun_table) 
        if(std::get<0>(pp) == fun_name)
            return pp;
    assert(false);
    return no_func;
}
fun_info_t const &fun_info(int idx) {
    if(idx < 1 || idx > supp_fun_table.size())
        return no_func;
    return supp_fun_table[idx-1];
}
int lookup_fun(value_type *rvtp, std::string fname, std::vector<value_type> const &avt, bool allow_promotions) {
    std::cerr << "LOOKING for \"" << fname << "\" with " << avt << ", promotions: " << (allow_promotions? "yes": "no") << "\n";
    static int show = 0;
    if(++show == 1) {
       std::cerr << supp_fun_table;
       std::cerr << "\n";
    }
    
    value_type vt; int fun_x = 0, rdim = -1;
    for(auto const &pp: supp_fun_table) {
        ++fun_x;
        if(std::get<0>(pp) == fname) 
            std::cerr << "     CONSIDERING: " << pp << "\n";
        if(std::get<0>(pp) == fname && 
           (rdim = std::get<4>(pp).can_be_called_with(value_type(avt.begin(), avt.end()), allow_promotions, std::get<3>(pp))) >= 0) {
            vt = value_type(rdim, std::get<2>(pp));
            if(vt.zd_type() == value_type(fvt_any)) {
                // the return type depends on the argument type
                int any_arg = -1;
                for(auto atype: std::get<4>(pp).inf) {
                    ++any_arg;
                    if(atype.zd_type() == value_type(fvt_any)) {
                        vt = vt.retype(avt[any_arg].zd_type());
                        break;
                    }    
                }
            }
            break;
        }
    }
    if(vt.is_null()) 
        fun_x = 0;
    if(rvtp != nullptr) 
        *rvtp = vt;
    if(fun_x > 0)
        std::cerr << "     GOT: " << supp_fun_table[fun_x-1] << "\n";
    else 
        std::cerr << "     NOT GOT\n";
    return fun_x;
}
value_type rpc_type(value_type ret_t, value_type arg_t, std::vector<value_type> const &avt, bool allow_promotions) {
    //std::cerr << "RPC -> " << ret_t << "matching " << arg_t << " with " << avt << ", promotions: " << (allow_promotions? "yes": "no") << "\n";
    value_type vt;
    int rdim = arg_t.can_be_set_with(value_type(avt.begin(), avt.end()), allow_promotions);
    if(rdim >= 0) 
        vt = value_type(rdim, ret_t);
    return vt;
}
}
