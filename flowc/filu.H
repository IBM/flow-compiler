#ifndef H_FILU_H
#define H_FILU_H

#include <string>

namespace filu {
std::string gwd();
bool is_dir(char const *fn);
void chmodx(std::string const &fn);
std::string realpath(std::string const &f);
int cp_p(std::string const &source, std::string const &dest);
int write_file(std::string const &fn, std::string const &source_fn, char const *default_content);
}
#endif

