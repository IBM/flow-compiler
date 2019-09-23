#ifndef H_VARSUB_H
#define H_VARSUB_H

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <tuple>

namespace varsub {
/********************************************************************
 * Find a value in a variable number of maps
 * All maps have to have a find() that returns a pointer to pair of key values
 * and an end() that rerurns a poiner indicating not found.
 * All maps must also have K as a key type and V as a value type.
 *
 * Returns the first found value in the argument list or nullptr if not found.
 */
template <typename V, typename K, typename M> 
inline static V *find_value(K key, M *map) {
    if(map == nullptr) return nullptr;
    auto f = map->find(key);
    if(f == map->end()) return nullptr;
    return &(f->second());
}
template <typename V, typename K, typename M, typename... MM> 
inline static V *find_value(K key, M *first, MM... args) {
    auto v = find_value(key, first);
    if(v != nullptr) return v;
    return find_value(key, args...);
}
/********************************************************************
 * Find a macro reference of the form
 *      left+name+[:default]+right 
 * or
 *      left[0]+letterX+left[1..]+repeated-template+right[0]+letterX+right[1..]
 *
 * Return (letterX, name, before, after, repeated-begin, repeated-end) for the second form, 
 *        (' ',     name, before, after) for the first, and
 *        ('\0',      "", templ) for not found
 *
 * LetterX must be a single character [a-zA-Z]
 * 
 */
inline static
std::tuple<char, std::string, char const *, char const *, char const *, char const *> find_macro2(char const *templ, char const *left, char const *right) {
    if(templ != nullptr) for(char const *p = strchr(templ, left[0]); p != nullptr; p = strchr(p+1, left[0])) {
        if(strncmp(p, left, strlen(left)) == 0) {
            // possible form 1
            char const *x = p+strlen(left);
            if(*x == '\0' || !isalpha(*x)) 
                continue;
            while(*x != '\0' && (isalnum(*x) || *x == '_' || *x == '.')) ++x;
            if(*x == ':') {
                char const *r = strstr(p+strlen(left), right);
                if(r != nullptr) 
                    return std::make_tuple(' ', std::string(p+strlen(left), r), p, r+strlen(right), nullptr, nullptr); 
            } else if(strncmp(x, right, strlen(right)) == 0) {
                return std::make_tuple(' ', std::string(p+strlen(left), x), p, x+strlen(right), nullptr, nullptr);
            }
            continue;
        }
        // Look for letterX
        char const *x = p+1;
        if(*x == '\0' || !isalpha(*x)) 
            continue;
        char const *q = strstr(x+1, left+1);
        if(q == nullptr) 
            continue;
        // Look for the end of loop marker
        std::string end_marker = std::string(right, right+1) + *x + (right+1);
        char const *r = strstr(q+strlen(left+1), end_marker.c_str());
        if(r != nullptr) 
            return std::make_tuple(*x, std::string(*(x+1)==':'? x+2: x+1, q), p, r+end_marker.length(), q+strlen(left+1), r);
    }
    return std::make_tuple('\0', std::string(), templ, nullptr, nullptr, nullptr);
}
inline static
std::tuple<char, std::string, char const *, char const *, char const *, char const *> find_macro(char const *templ, char const *left, char const *right) {
    auto r = find_macro2(templ, left, right);
    return r;
}
template <class MAP1, class MAP2> static inline
std::pair<int, std::string> findvar2(std::string const &name, MAP1 const &vars, MAP2 const &lvars, int index) {
    auto default_value_pos = name.find_first_of(':');
    bool have_default = default_value_pos != std::string::npos;
    std::string var_name(have_default? name.substr(0, default_value_pos): name);
    auto lvp = lvars.find(var_name);
    if(lvp != lvars.end()) {
        if(index < lvp->second.size())
            return std::make_pair(1, lvp->second[index]);
        return std::make_pair(have_default? 0: -1, have_default? name.substr(default_value_pos+1): std::string());
    }
    auto vp = vars.find(var_name);
    if(vp != vars.end()) {
        if(index < vp->second.size())
            return std::make_pair(1, vp->second[index]);
        if(vp->second.size() == 1)
            return std::make_pair(1, vp->second[0]);

    }
    return std::make_pair(have_default? 0: -1, have_default? name.substr(default_value_pos+1): std::string());
}
template <class MAP1, class MAP2> static inline
std::pair<int, std::string> findvar(std::string const &name, MAP1 const &vars, MAP2 const &lvars, int index) {
    auto r =  findvar2(name, vars, lvars, index);
    return r;
}
template <class MAP1, class MAP2> static inline
bool render_varsub(std::ostream &out, char const *templ, MAP1 const &map, MAP2 const &lvars, char const *left="{{", char const *right="}}", int index=-1) {
    auto f = varsub::find_macro(templ, left, right); 
    int found = 0;
    while(std::get<0>(f) != '\0') {
        out << std::string(templ, std::get<2>(f));
        if(std::get<0>(f) == ' ') {
            auto fv = findvar(std::get<1>(f), map, lvars, index < 0? 0: index);
            if(fv.first > 0) ++found;
            if(fv.first < 0) out << left << std::get<1>(f) << right;
            else out << fv.second;
        } else {
            int maxx = -1;
            if(!std::get<1>(f).empty()) {
                auto xv = map.find(std::get<1>(f));
                if(xv != map.end()) {
                    maxx =  xv->second.size();
                } else {
                    auto xl = lvars.find(std::get<1>(f));
                    maxx = xl == lvars.end()? 0: xl->second.size();
                }
            }
            std::string repeat(std::get<4>(f), std::get<5>(f));
            std::ostringstream sout;
            int i = 0;

            while((maxx < 0 || i < maxx) && render_varsub(sout, repeat.c_str(), map, lvars, left, right, i)) {
                out << sout.str(); sout.str(""); ++i;
            }
        }
        f = varsub::find_macro(templ = std::get<3>(f), left, right);
    }
    out << std::get<2>(f);
    return found > 0;
}
template <class MAP1> static inline
bool render_varsub(std::ostream &out, char const *templ, MAP1 const &map, char const *left="{{", char const *right="}}") {
    std::map<std::string, std::vector<std::string>> empty;
    return render_varsub(out, templ, map, empty, left, right);
}
template <class MAP1, class MAP2> static inline
std::string render_varsub(char const *templ, MAP1 const &map, MAP2 const &lvars, char const *left="{{", char const *right="}}") {
    std::ostringstream out;
    render_varsub(out, templ, map, lvars, left, right);
    return out.str();
}
template <class MAP1> static inline
std::string render_varsub(char const *templ, MAP1 const &map, char const *left="{{", char const *right="}}") {
    std::map<std::string, std::vector<std::string>> empty;
    std::ostringstream out;
    render_varsub(out, templ, map, empty, left, right);
    return out.str();
}
template <class MAP1> static inline
std::string render_varsub(std::string const &templ, MAP1 const &map, char const *left="{{", char const *right="}}") {
    std::map<std::string, std::vector<std::string>> empty;
    std::ostringstream out;
    render_varsub(out, templ.c_str(), map, empty, left, right);
    return out.str();
}
}
#endif
