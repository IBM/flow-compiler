#ifndef H_FILU_H
#define H_FILU_H

#include <map>
#include <string>
#include <utility>

namespace filu {

std::string gwd();
bool is_dir(char const *fn);
inline static
bool is_dir(std::string const &fn) {
    return is_dir(fn.c_str());
}
bool is_reg(char const *fn);
inline static
bool is_reg(std::string const &fn) {
    return is_reg(fn.c_str());
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

std::string search_path(std::string const &bin, std::string const &envvn);
inline static 
std::string search_path(std::string bin) {
    return search_path(bin, "PATH");
}

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
std::string gunzip(unsigned char const *buffer, unsigned zlen, bool gz=true);

std::string pipe(std::string command, std::string input, int *rc, std::string *err=nullptr, bool err2out=false);
/** Look for a config file .name, name.cfg in the current directory and in the home directory.
 *  Read the configuration in a map as below.
 */
bool config(std::map<std::string, std::string> &cfg, std::string name, bool search=true);
/** Read a configuration file in the form:
 *      name: value 
 *      name = value 
 *  Allow <value> to be quoted. Ignore lines that start with #.
 */
bool config(std::map<std::string, std::string> &cfg, std::istream &ins, std::string comment_triggers="#", bool case_sensitive=true, std::string separators=":=");
/** Find an ini file section. 
 *  If section is 'default' or empty, and there is no section defined, return the entire file.
 */
bool ini_section(std::string &section, std::istream &fs, std::string section_name);

/** Read the entire stream into a string
 */
std::string reads(std::istream &s);
}
#endif

