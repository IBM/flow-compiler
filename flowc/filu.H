#ifndef H_FILU_H
#define H_FILU_H

#include <string>
#include <utility>

namespace filu {

std::string gwd();
bool is_dir(char const *fn);
void chmodx(std::string const &fn);
std::string realpath(std::string const &f);
int cp_p(std::string const &source, std::string const &dest);
int write_file(std::string const &fn, std::string const &source_fn, char const *default_content, size_t length = 0);
inline static 
int write_file(std::string const &fn, std::string const &source_fn, std::string const &default_content) {
    return write_file(fn, source_fn, default_content.data(), default_content.length());
}
long gzip(std::ostream &dest, std::istream &source, bool gz=false);
long gunzip(std::ostream &dest, std::istream &source, bool gz=false);

}
#endif

