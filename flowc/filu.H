#ifndef H_FILU_H
#define H_FILU_H

#include <string>
#include <utility>

namespace filu {

std::string gwd();
bool is_dir(char const *fn);
inline static
bool is_dir(std::string const &fn) {
    return is_dir(fn.c_str());
}
void chmodx(std::string const &fn);
std::string realpath(std::string const &f);
int cp_p(std::string const &source, std::string const &dest);
int write_file(std::string const &fn, std::string const &source_fn, char const *default_content, size_t length = 0);
inline static 
int write_file(std::string const &fn, std::string const &source_fn, std::string const &default_content) {
    return write_file(fn, source_fn, default_content.data(), default_content.length());
}
inline static 
int write_file(std::string const &fn, std::string const &content) {
    return write_file(fn, "", content.data(), content.length());
}
inline static 
int write_file(std::string const &fn, char const *content, size_t length) {
    return write_file(fn, "", content, length);
}
long gzip(std::ostream &dest, std::istream &source, bool gz=false);
long gunzip(std::ostream &dest, std::istream &source, bool gz=false);

std::string search_path(std::string const &bin);

std::string basename(std::string const &filename, std::string const &suffix = "", std::string *dirname=nullptr);
inline static 
std::string dirname(std::string const &filename) {
    std::string dn;
    basename(filename, "", &dn);
    return dn;
}
inline static 
std::string path_join(std::string const &base_path, std::string const &path) {
    if(path.empty()) return base_path;
    if(base_path.empty() || path[0] == '/') return path;
    return *base_path.rbegin() == '/'? base_path + path: base_path + '/' + path;
}
}
#endif

