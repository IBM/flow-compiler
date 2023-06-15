#ifndef H_FILU_H
#define H_FILU_H

#include <map>
#include <string>
#include <utility>

namespace filu {
/** Get the working directory from the PWD environment variable
 */
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
/** Copy files, as the cp -p command
 */
int cp_p(std::string const &source, std::string const &dest);
/** Wrapper for stdlib realpath
 */
std::string realpath(std::string const &f);
/** Search the directories in the <evn=PATH> environment variable for <command>
 */
std::string search_path(std::string const &bin, std::string const &envvn);
inline static 
std::string search_path(std::string bin) {
    return search_path(bin, "PATH");
}
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
/** Return the filename part of <path>, optionally removing <suffix>, similar to the basename command.
 * If <dirname> is a valid pointer, the directory part of <path> will be stored there.
 */
std::string basename(std::string const &filename, std::string const &suffix = "", std::string *dirname=nullptr);
/** Return the directory part of <path>, similar to the dirname command.
 */
inline static 
std::string dirname(std::string const &filename) {
    std::string dn;
    basename(filename, "", &dn);
    return dn;
}
/** Return the first component of the path, or / if the path is root.
 */
std::string path_split(std::string const &path, std::string *second=nullptr);
/** Split the url into components [scheme://]host[/path][?query][#section]
 * The default component is host. Return a bitfiled with a bit set for each non-empty component, 
 * with bit 4 set for section and bit 0 for scheme.
 */
int url_split(std::string const &url, std::string *scheme=nullptr, std::string *host=nullptr, std::string *path=nullptr, std::string *query=nullptr, std::string *section=nullptr);
/** Join the path path components
 */
template <typename S1, typename S2>
inline static 
std::string path_join(S1 base_path, S2 path) {
    std::string p(path), bp(base_path);
    if(p.empty()) return bp;
    if(bp.empty() || p[0] == '/') return p;
    return *bp.rbegin() == '/'? bp + p: bp + '/' + p;
}
template <typename S1, typename S2, typename... MS>
inline static 
std::string path_join(S1 base_path, S2 path, MS... more_paths) {
    std::string bp = path_join(base_path, path);
    return path_join(bp, more_paths...);
}
/** Unzip the data in the <buffer> and return the result as a string.
 * Set <gz> to true if the buffer contains the output of the gzip command (as opposed to the ouptupt of a zlib call).
 * Set <zlen> to the number of bytes pointed to by <buffer>.
 */
std::string gunzip(unsigned char const *buffer, unsigned zlen, bool gz=true);
/** Unzip the data in the input stream and write it to the output stream.
 * Set <gz> to true if the input was produced by the gzip command as opposed to the zlib library call.
 * Return the number of bytes processed from the input stream or a negative number if an error occured.
 */
long gunzip(std::ostream &dest, std::istream &source, bool gz=false);
/** Zip the data from the input stream into the output stream. 
 * Set <gz> to true to produce output compaitible with the gzip command.
 * Return the number of bytes processed from the input stream or a negative number if an error occured.
 */
long gzip(std::ostream &dest, std::istream &source, bool gz=false);

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

