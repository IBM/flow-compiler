#include "value-type.H"
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace fc {
value_type op1_type(int op, value_type l);
value_type op2_type(int op, value_type l, value_type r);
extern std::multimap<std::string, std::tuple<bool, fc::value_type, int, fc::value_type>> supp_fun_table;
value_type fun_type(std::string fname, std::vector<value_type> const &avt);
}
