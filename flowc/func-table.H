#include "value-type.H"
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace fc {
typedef std::tuple<std::string, bool, fc::value_type, int, fc::value_type> fun_info_t; 

int lookup_fun(value_type *return_type, std::string fname, std::vector<value_type> const &avt, bool allow_promotions);

fun_info_t const &fun_first_match(std::string fun_name);
fun_info_t const &fun_info(int);

value_type rpc_type(value_type ret_t, value_type arg_t, std::vector<value_type> const &avt, bool allow_promotions);
}
