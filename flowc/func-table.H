#include "value-type.H"
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace fc {
value_type op1_type(int op, value_type l);
value_type op2_type(int op, value_type l, value_type r);
typedef std::tuple<bool, fc::value_type, int, fc::value_type> fun_info_t; 
extern std::multimap<std::string, fun_info_t> supp_fun_table;
value_type fun_type(std::string fname, std::vector<value_type> const &avt, bool allow_promotions);
fun_info_t const &fun_first_match(std::string fun_name);
}
