#include <cctype>
#include <cstring>
#include <list>
#include <map>
#include <vector>
#include <sstream>
#include <string>
#include "stru1.H"

namespace vex {
/**
* Cascaded map template. The maps are looked up left to right.
*/
template <class... Ts> struct cmap{
    /**
     * If the variable is found true and the value corresponding to the index should be returned.
     * If the index is out of range the first value should be returned.
     * If the varaible is not found, false and an empty string must be returned.
     */
    std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
        return std::make_pair(false, std::string());
    }
    /** 
     * Returns the number of values for name or -1 if lookup fails.
     */
    virtual int gets(std::string const &name) const {
        return -1;
    }
};
template <class T, class... Ts>
struct cmap<T, Ts...>: cmap<Ts...> {
    cmap(T t, Ts... ts): cmap<Ts...>(ts...), tail(t) {}
    T tail;

    std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
        auto f = tail.getv(name, index);
        return f.first? f: cmap<Ts...>::getv(name, index);
    }
    int gets(std::string const &name) const {
        int s  = tail.gets(name);
        return s >= 0? s: cmap<Ts...>::gets(name);
    }
};    
template <class... Ts> inline static 
cmap<Ts...> make_cmap(Ts&&... args) {
    return cmap<Ts...>(std::forward<Ts>(args)...);
}
/**
 * Abstract map interface
 */
struct amap {
    /**
     * Return the value corresponding to the index and true if the value is found
     * Return the first value if the index is out of range.
     * If the variable is not found, return false and an empty string.
     */
    virtual std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
        return std::make_pair(false, std::string());
    }
    /** 
     * Returns the number of values for name or -1 if lookup fails.
     */
    virtual int gets(std::string const &name) const {
        return -1;
    }
};
/**
 * Templates to virtualize a vex::map.
 * vsmap keeps a reference to the original map, vdmap make a deep copy.
 */
template <class MAP> 
struct vmap: public amap {
    MAP const m;
    vmap(MAP mm): m(mm) {};
    virtual std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
        return m.getv(name, index);
    }
    virtual int gets(std::string const &name) const {
        return m.gets(name);
    }
};
template <class M> static inline
vmap<M> make_vmap(M m) {
    return vmap<M>(m);
}
/**
 * Variable map based on a std::map likeinterface. Uses find() and end() methods only.
 * find() must take a string type as argument.
 * The result to find must be a pointer type to a structure that stores the values in a filed called 'second'.
 * The value field must be an array type withe a size() method.
 */
template <class M=std::map<std::string, std::vector<std::string>>>
struct smap {
    M const m;
    smap(M mm): m(mm) {
    }
    std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
        auto f = m.find(name);
        if(f == m.end())
            return std::make_pair(false, std::string());
        if(f->second.size() == 0)
            return std::make_pair(true, std::string());
        return std::make_pair(true, f->second[f->second.size() > index && index >= 0? index: 0]);
    }
    int gets(std::string const &name) const {
        auto f = m.find(name);
        if(f == m.end()) 
            return -1;
        return int(f->second.size());
    }
};
template <class M>
struct smap<M> make_smap(M m) {
    return smap<M>(m);
}
/**
 * A map consisting of a list of virtual maps
 * Use the std::list interface to populate the map.
 * Lookups are prioritized in list oder.
 */
class lvmap: public std::list<amap *> {
public:    
    std::pair<bool, std::string> getv(std::string const &name, int index=0) const {
        for(auto const &m: *this) {
            auto f = m->getv(name, index);
            if(f.first) return f;
        }
        return std::make_pair(false, std::string());
    }
    int gets(std::string const &name) const {
        for(auto const &m: *this) {
            int s = m->gets(name);
            if(s >= 0)
                return s;
        }
        return -1;
    }
};
struct macro_descr {
    std::string label;
    std::string name;
    int count = -1;
    int min_max = 0;
    char const *before = nullptr;
    char const *after = nullptr;
    char const *begin = nullptr;
    char const *end = nullptr;
    std::string default_value;
    bool has_default = false;

    void clear(char const *b=nullptr, char const *a=nullptr) {
        label.clear();
        name.clear();
        count = -1;
        min_max = 0;
        begin = nullptr;
        end = nullptr;
        before = b;
        after = a;
    }
};
/** 
 * Find a mcaro reference in one of the thre forms below
 *      
 *      1. left1 left2 name [ / transforms ]* [ + name [/ transforms]* ]* - default right1 right2
 *      2. left1 label left2 template right1 label right2
 *      3. left1 label : [-|+] left2 template right1 label right2
 *      4. left1 label : [-|+] name [+ name]* [ - integer ] left2 template right1 label right2
 *      5. left1 label : [-|+] integer left2 template right1 label right2
 *
 *      In case 1 the name is looked up and and applied any transforms that follow. 
 *      Subsequent names will be looked up if the first name is not found. 
 *      If no name is found the default value will be used if given.
 *      When a default value is not given the output will be an empty string.
 *
 *      Cases 2,3,4,5 render 'template' repeatedly in a loop.
 *      In the cases 2 and 3 with - the template is repeated until one of the variables within are not found.
 *      In case 3 with + the template is repeated until all variables in template are not found.
 *
 *      In case 4 the number of repetitions is computed from the sizes of the named variables:
 *           when - is prepended, the minimum of the sizes chosen, missing variables will have size 0
 *           when + is prepended, the maximum of the sizes will be used
 *           when no - or + is present the first non-zero size will be used
 *
 *      In case 5 the integer value will be used as a loop counter
 *
 * Populates a descriptor structure and returns the number of errors encountered.
 * Errors are printed to the error stream.
 */
struct macro_parser {
    char const *left1 = "{";
    char const *right1 = "}";
    char const *left2 = "{";
    char const *right2 = "}";

    std::ostream *errp = nullptr;
    long lc = 0;
    std::string label;

    static bool match(char const *obj, char const *heap, char const *end_heap) {
        char const *bo = obj, *bh = heap;
        while(*bo != '\0' && bh != end_heap && *bo == *bh) ++bo, ++bh;
        return *bo == '\0';
    }
    static char const *find(char const *obj, char const *heap, char const *end_heap) {
        while(heap != end_heap) {
            while(heap != end_heap && *heap != *obj) ++heap;
            if(heap != end_heap) {
                if(matchs(obj+1, heap+1, end_heap)) return heap;
                ++heap;
            }
        }
        return nullptr;
    }
    static char const *matchs(char const *obj, char const *heap, char const *end_heap) {
        if(obj == nullptr || heap == nullptr) return nullptr;
        char const *bo = obj, *bh = heap;
        while(*bo != '\0' && bh != end_heap && *bo == *bh) ++bo, ++bh;
        return *bo == '\0'? bh: nullptr;
    }
    static char const *match_id(char const *begin, char const *end) {
        if(begin == nullptr || begin == end || !std::isalpha(*begin)) return nullptr;
        char const *start = begin+1;
        while(start != end && (std::isalnum(*start) || *start == '_' || *start == '.')) ++start;
        return start;
    }
    static char const *match_integer(char const *begin, char const *end) {
        if(begin == nullptr) return nullptr;
        char const *e = begin;
        while(e != end && std::isdigit(*e)) ++e;
        return e == begin? nullptr: e;
    }
    static char const *match_id_list(char const *begin, char const *end, char const *sep) {
        auto p = match_id(begin, end); 
        if(p != nullptr) while(p != end) {
            if(strchr(sep, *p) == nullptr) 
                break;
            auto np = match_id(p+1, end);
            if(np == nullptr) 
                break;
            p = np;
        }
        return p;
    }
    int find_macro(macro_descr &md, char const *templ, char const *templ_end) {
        char const *mp = templ-1, *p = nullptr, *e = nullptr;

        while(++mp != templ_end) {
            md.clear(templ, templ_end);
            // scan and match for first1
            mp = find(left1, mp, templ_end);
            p = matchs(left1, mp, templ_end);
            if(mp == nullptr || p == templ_end) { 
                // either not found or no room for the rest of the macro
                break;
            }
            if(*p == *left2) { // match type 1
                p = matchs(left2, p, templ_end);
                if(p == nullptr || p == templ_end)
                    continue;
                e = match_id_list(p, templ_end, "+/");
                if(e == nullptr || e == templ_end)
                    continue;
                md.name = std::string(p, e);
                p = e;
                if(*p == '-') {
                    std::string right(right1); right += right2;
                    e = find(right.c_str(), p+1, templ_end);
                    if(e == nullptr) 
                        continue;
                    md.default_value = std::string(p+1, e);
                    md.has_default = true;
                    p = e;
                }
                e = matchs(right2, matchs(right1, p, templ_end), templ_end);
                if(e == nullptr) 
                    continue;
                md.before = mp;
                md.after = e;
                md.begin = md.end = nullptr;
                return 0;
            }
            if(!std::isalpha(*p)) 
                continue;
            // match loop types
            e = match_id(p, templ_end);
            if(e == nullptr || e == templ_end)
                continue;
            md.label = std::string(p, e);
            p = e;
            if(*p == ':') { 
                // match types 2 and 3
                if(++p == templ_end)
                    continue;
                // match stop condition
                if(*p == '+' || *p == '-') {
                    md.min_max = *p == '+'? 1: -1;
                    if(++p == templ_end) 
                        continue;
                } else {
                    md.min_max = 0;
                }

                e = match_id_list(p, templ_end, "+");
                if(e == templ_end)
                    continue;
                if(e != nullptr) {
                    md.name = std::string(p, e);
                    p = e;
                }
                if(e == nullptr && *p == '-') 
                    ++p;
                if(p == templ_end) 
                    continue;
                if(std::isdigit(*p)) {
                    // match default number
                    e = match_integer(p, templ_end);
                    std::string value(p, e);
                    md.count = std::atoi(value.c_str());
                    p = e;
                }
            }
            p = matchs(left2, p, templ_end);
            if(p == nullptr || p == templ_end)
                continue;
            // scan for right1 label right2
            std::string right(right1); right += md.label; right += right2;
            e = find(right.c_str(), p, templ_end);
            if(e != nullptr) {
                md.before = mp;
                md.after = e + right.length();
                md.begin = p;
                md.end = e;
                return 0;
            }
        }
        md.clear(templ, templ_end);
        return 0;
    }
    template <class MAP1> int gets(MAP1 const &map, std::string const &name, int min_max) const {
        int s = 0;
        std::string::size_type nb = 0;
        while(nb < name.length()) {
            auto ne = name.find_first_of('+', nb);
            if(ne == std::string::npos) ne = name.length();
            std::string n = name.substr(nb, ne - nb);
            int ns = map.gets(n);
            if(ns >= 0) {
                if(min_max == 0 || (min_max < 0 && s < 0)) {
                    s = ns;
                } else if(min_max < 0) {
                    s = std::min(ns, s);
                } else {
                    s = std::max(ns, s);
                }
                if(min_max == 0)
                    break;
            }
            nb = ne + 1; 
        }
        return s;
    }
    template <class MAP1> std::pair<int, std::string> get_value(MAP1 const &map, std::string const &names, int index) {
        std::string::size_type tb = 0;
        while(tb < names.length()) {
            auto te = names.find_first_of('+', tb);
            if(te == std::string::npos) te = names.length();
            std::string namet = names.substr(tb, te - tb);
            tb = te+1;
            auto ne = namet.find_first_of('/');
            if(ne == std::string::npos) ne = namet.length();
            std::string name = namet.substr(0, ne);
            auto fv = map.getv(name, index);
            if(!fv.first) 
                continue;
            std::string value(fv.second);
            while(ne < namet.length()) {
                auto te = namet.find_first_of('/', ++ne);
                if(te == std::string::npos) te = namet.length();
                std::string transf = namet.substr(ne, te - ne);
                ne = te;
                // apply transform here
                //std::cerr << "APPLY TRANSFORM: " << transf << "\n";
                if(transf == "lower") {
                    value = stru1::to_lower(value);
                } else if(transf == "upper") {
                    value = stru1::to_upper(value);
                } else if(transf == "option" || transf == "cname") {
                    value = stru1::to_option(value, false);
                } else if(transf == "id" || transf == "identifier") {
                    value = stru1::to_identifier(value);
                } else if(transf == "html") {
                    value = stru1::html_escape(value);
                } else if(transf == "C" || transf == "c" || transf == "string") {
                    value = stru1::c_escape(value);
                } else if(transf == "shell" || transf == "sh") {
                    value = stru1::sh_escape(value);
                } else if(transf == "shell_comment" || transf == "shcom") {
                    value = stru1::to_line_comment(value, "# ");
                } else if(transf == "C_comment" || transf == "ccom" || transf == "c_comment") {
                    value = stru1::to_line_comment(value, "// ");
                } else if(transf == "trim") {
                    value = stru1::strip(value);
                } else if(transf == "ltrim") {
                    int c;
                    value = stru1::lstrip(c, value, "\t\r\a\b\v\f\n ");
                } else if(transf == "rtrim") {
                    int c;
                    value = stru1::rstrip(c, value, "\t\r\a\b\v\f\n ");
                } else if(transf == "lchop") {
                    value = value.length() <= 1? std::string(): value.substr(1);
                } else if(transf == "rchop") {
                    value = value.length() <= 1? std::string(): value.substr(1, value.length()-1);
                } else if(transf == "chop") {
                    value = value.length() <= 2? std::string(): value.substr(1, value.length()-2);
                } else if(transf == "camelize") {
                    value = stru1::camelize(value);
                } else if(transf == "decamelize") {
                    value = stru1::decamelize(value);
                } else if(transf == "underscore") {
                    value = stru1::to_underscore(value);
                } else if(transf == "json") {
                    value = stru1::json_escape(value);
                } else {
                    return std::make_pair(2, std::string());
                }
            }
            return std::make_pair(0, value);
        }
        return std::make_pair(1, std::string());
    }
    /***
     *  Return the number of substitutions and the number of misses.
     */
    template <class MAP1> std::pair<int, int> render_varsub(std::ostream &out, char const *templ, char const *templ_end, MAP1 const &map, int index=-1) {
        if(templ_end == nullptr) templ_end = templ + strlen(templ);
        macro_descr md;
        int mprc = find_macro(md, templ, templ_end);
        char const *pos = templ;
        int missed = 0, found = 0;
        while(mprc == 0 && !(md.name.empty() && md.label.empty())) {
            //std::cerr << "--- " << (md.before - templ) << ":" << (md.after-md.before) << " GOT: " << md.name << "(" << md.label << ") def: " << md.has_default << " [" << md.default_value << "] " << (md.end - md.begin) << std::endl;
            out << std::string(pos, md.before);
            pos = md.after;
            if(md.label.empty()) {
                // regular variable reference
                auto fv = get_value(map, md.name, index < 0? 0: index);
                if(fv.first == 0) { 
                    out << fv.second;
                    ++found;
                } else if(fv.first == 1 && md.has_default) {
                    ++missed;
                    out << md.default_value;
                } else {
                    ++missed;
                    out << std::string(md.before, md.after);
                }
            } else {
                // loop variable reference
                int maxx = md.count >= 0? md.count: (md.name.empty()? -1: gets(map, md.name, md.min_max));
                int mima = md.name.empty()? (md.min_max > 0? 1: -1) : 0;
                //std::cerr << "REPEATING " << md.label << " " << maxx << " min-max: " << md.min_max << " mima: " << mima << " TIMES\n";
                std::ostringstream sout;
                for(int i = 0; maxx < 0 || i < maxx; ++i) {
                    auto rok = render_varsub(sout, md.begin, md.end, map, i);
                    //std::cerr << "    REP " << md.label << i << " found: " << rok.first << ", missed: " << rok.second << "\n";
                    if(mima < 0 && rok.second > 0)
                        break;
                    if(mima > 0 && rok.first == 0)
                        break;
                    out << sout.str(); sout.str("");
                    // prevent infinite loops for templates without variable references
                    if(rok.first == 0 && rok.second == 0 && maxx <= 0)
                        break;

                    if(i > 12) break;
                }
            }
            mprc = find_macro(md, pos, templ_end);
        }
        out << std::string(md.before, md.after);
        return std::make_pair(found, missed);
    }
};
template <class MAP> 
int expand(std::ostream &out, char const *in, char const *end_in, MAP const &map) {
    macro_parser mp;
    auto rc = mp.render_varsub(out, in, end_in, map); 
    return rc.second;
}
template <class MAP> 
int expand(std::ostream &out, char const *in, MAP const &map) {
    return expand(out, in, strchr(in, '\0'), map); 
}
template <class MAP> 
int expand(std::ostream &out, std::string const &in, MAP const &map) {
    return expand(out, in.c_str(), in.c_str()+in.length(), map); 
}
template <class MAP> 
int expand(std::ostream &out, std::istream &in, MAP const &map) {
    std::string templ;
    for(char c = in.get(); in.good(); c = in.get()) {
        templ +=  c;
    }
    return expand(out, templ, map);
}
}
