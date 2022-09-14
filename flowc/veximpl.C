#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <functional>
#include "stru1.H"

namespace vex {

char const *default_escape_strings[4] = { "{", "{", "}", "}" };

struct macro_descr {
    std::string label;
    std::string name;
    int count = -1;
    int min_max = 0;
    int start = -1;
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
        start = -1;
        min_max = 0;
        begin = nullptr;
        end = nullptr;
        before = b;
        after = a;
    }
};
inline static
std::ostream &operator << (std::ostream &o, macro_descr const &m) {
    o << "{" << m.label << "("<< m.name << ") c: "<< m.count << ", mm: " << m.min_max << ", len: " << (m.end-m.begin) << ", has-default: " << m.has_default << "}";
    return o;
}
/** 
 * Scan for a macro reference in one of the forms below
 *      
 *      1. left1 left2 name [ / transforms ]* [ + name [/ transforms]* ]* - default right1 right2
 *      2. left1 label left2 template right1 label right2
 *      3. left1 label : [-|+] left2 template right1 label right2
 *      4. left1 label : [-|+] name [+name]* [-integer ] [+integer] left2 template right1 label right2
 *      5. left1 label : [-|+] integer [+integer] left2 template right1 label right2
 *      6. left1 label ? name [|name]* [&name]* left2 template right1 label right2
 *
 *      In case 1 the name is looked up and and applied any transforms that follow. 
 *      Subsequent names will be looked up if the first name is not found. 
 *      If no name is found the default value will be used if given.
 *      When a default value is not given the output will be the macro itself.
 *
 *      Cases 2,3,4,5 render 'template' repeatedly in a loop.
 *      In case 2, and case 3 with -, the template is repeated until one of the variables referenced within are not found.
 *      In case 3 with +, the template is repeated until all variables in referenced in the template are not found.
 *
 *      In case 4 the number of repetitions is computed from the sizes of the named variables:
 *           when - is prepended, the minimum of the sizes is chosen, missing variables will have size 0
 *           when + is prepended, the maximum of the sizes will be used
 *           when no - or + is present the first non-zero size will be used
 *      In case 5 the integer value will be used as a loop counter
 *      In cases 4 and 5 if a [+integer] is given, indexing will start there (1 based)
 *      In case 6 variables are evaluated as a boolean expression: empty string, no value, 0, or false for false, anything else for true.
 *
 * Populates a descriptor structure and returns the number of errors encountered.
 * Errors are printed to the error stream.
 */
struct macro_parser {
    char const *left1;
    char const *right1;
    char const *left2;
    char const *right2;

    std::ostream *errp = nullptr;
    long lc = 0;
    std::string label;
    std::function<std::pair<bool, std::string>(std::string const &, int)> gv_value;
    std::function<int(std::string const &)> gv_count;

    macro_parser(decltype(gv_value) gvf, decltype(gv_count) gcf, 
            char const *b1 = "{", char const *e1 = "}", char const *b2 = "{", char const *e2 = "}"
            ):gv_value(gvf), gv_count(gcf), left1(b1), right1(e1), left2(b2), right2(e2) {
    }
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
                if(*p == '-') 
                    ++p;
                if(p == templ_end) 
                    continue;
                if(std::isdigit(*p)) {
                    // match maximum count
                    e = match_integer(p, templ_end);
                    std::string value(p, e);
                    md.count = std::atoi(value.c_str());
                    p = e;
                }
                if(*p == '+') 
                    ++p;
                if(p == templ_end) 
                    continue;
                if(std::isdigit(*p)) {
                    // match start counter
                    e = match_integer(p, templ_end);
                    std::string value(p, e);
                    md.start = std::atoi(value.c_str());
                    p = e;
                }
            } else if(*p == '?') {
                if(++p == templ_end)
                    continue;
                // expect a list of ids separated by &, |, + or *
                e = match_id_list(p, templ_end, "&|");
                if(e == nullptr || e == templ_end)
                    continue;
                md.name = std::string(p, e);
                p = e;
                md.count = -2;
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
    int gets(std::string const &name, int min_max) const {
        int s = 0;
        std::string::size_type nb = 0;
        while(nb < name.length()) {
            auto ne = name.find_first_of('+', nb);
            if(ne == std::string::npos) ne = name.length();
            std::string n = name.substr(nb, ne - nb);
            int ns = gv_count(n);
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
    std::pair<int, std::string> get_value_c(std::string const &names, int index) {
        std::string::size_type tb = 0;
        while(tb < names.length()) {
            auto te = names.find_first_of('+', tb);
            if(te == std::string::npos) te = names.length();
            std::string namet = names.substr(tb, te - tb);
            tb = te+1;
            auto ne = namet.find_first_of('/');
            if(ne == std::string::npos) ne = namet.length();
            std::string name = namet.substr(0, ne);
            auto fv = gv_value(name, index);
            if(!fv.first) 
                continue;
            std::string value(fv.second);
            while(ne < namet.length()) {
                auto te = namet.find_first_of('/', ++ne);
                if(te == std::string::npos) te = namet.length();
                std::string transf = namet.substr(ne, te - ne);
                ne = te;
                // apply transform here
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
                    value = stru1::lstrip(nullptr, value, "\t\r\a\b\v\f\n ");
                } else if(transf == "rtrim") {
                    value = stru1::rstrip(nullptr, value, "\t\r\a\b\v\f\n ");
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
                } else if(transf == "cevs") {
                    value = stru1::cevs_transform(value);
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
    std::pair<int, std::string> get_value(std::string const &names, int index) {
        auto r = get_value_c(names, index);
        return r;
    } 

    /***
     *  Return the number of substitutions and the number of misses.
     */
    std::pair<int, int> render_varsub_r(std::ostream &out, char const *templ, char const *templ_end, int index, int start) {
        if(templ_end == nullptr) templ_end = templ + strlen(templ);
        macro_descr md;
        int mprc = find_macro(md, templ, templ_end);
        char const *pos = templ;
        int missed = 0, found = 0;
        while(mprc == 0 && !(md.name.empty() && md.label.empty())) {
            if(start < 0 || index >= start)
                out << std::string(pos, md.before);
            pos = md.after;
            if(md.label.empty()) {
                // regular variable reference
                auto fv = get_value(md.name, (index < 0? 0: index));
                std::string value;
                if(fv.first == 0) { 
                    value = fv.second;
                    ++found;
                } else if(fv.first == 1 && md.has_default) {
                    ++missed;
                    value = md.default_value;
                } else {
                    ++missed;
                    value = std::string(md.before, md.after);
                }
                if(start < 0 || index >= start)
                    out << value;
            } else if(md.count == -2) {
                // conditional reference
                bool cond = false;
                std::string bexp = md.name;
                for(auto op = bexp.find_first_of('&'); op != std::string::npos; op = bexp.find_first_of('&')) {
                    auto lb = bexp.substr(0, op).find_last_of('|');
                    if(lb == std::string::npos) lb = 0;
                    else ++lb;
                    std::string left = bexp.substr(lb, op-lb);
                    auto fe = bexp.find_first_of("&|", op+1);
                    if(fe == std::string::npos) fe = bexp.length();
                    std::string right = bexp.substr(op+1, fe-op-1);
                    auto fv = get_value(left, (index < 0? 0: index));
                    if(fv.first == 1 || fv.second.empty()) {
                        bexp = bexp.substr(0, lb) + left + bexp.substr(fe);
                        continue;
                    }
                    bexp = bexp.substr(0, lb) + right + bexp.substr(fe);
                }
                for(auto op = bexp.find_first_of('|'); op != std::string::npos; op = bexp.find_first_of('|')) {
                    std::string left = bexp.substr(0, op);
                    auto fe = bexp.find_first_of("&|", op+1);
                    if(fe == std::string::npos) fe = bexp.length();
                    std::string right = bexp.substr(op+1, fe-op-1);
                    auto fv = get_value(left, index < 0? 0: index);
                    if(fv.first == 1 || fv.second.empty()) {
                        bexp = bexp.substr(op+1);
                        continue;
                    }
                    cond = true;
                    break;
                }
                if(!cond) {
                    auto fv = get_value(bexp, (index < 0? 0: index));
                    cond = fv.first == 0 && !fv.second.empty();
                }
                if(cond) {
                    std::ostringstream sout;
                    render_varsub_r(sout, md.begin, md.end, index,  md.start > 0? md.start-1: 0);
                    if(start < 0 || index >= start)
                        out << sout.str(); 
                }
            } else {
                // loop reference
                int maxx = md.count >= 0? md.count: (md.name.empty()? -1: gets(md.name, md.min_max));
                int mima = md.name.empty()? (md.min_max > 0? 1: -1) : 0;
                std::ostringstream sout;
                for(int i = 0; maxx < 0 || i < maxx; ++i) {
                    auto rok = render_varsub_r(sout, md.begin, md.end, i, md.start > 0? md.start-1: 0);
                    if(mima < 0 && rok.second > 0)
                        break;
                    if(mima > 0 && rok.first == 0)
                        break;
                    if(start < 0 || index >= start)
                        out << sout.str(); 
                    sout.str("");
                    // prevent infinite loops for templates without variable references
                    if(rok.first == 0 && rok.second == 0 && maxx <= 0)
                        break;
                }
            }
            mprc = find_macro(md, pos, templ_end);
        }
        if(start < 0 || index >= start)
            out << std::string(md.before, md.after);
        return std::make_pair(found, missed);
    }
};
std::pair<int, int> render_varsub(std::ostream &out, char const *templ, char const *templ_end, 
        std::function<std::pair<bool, std::string>(std::string const &, int)> fgv, 
        std::function<int(std::string const &)> fgc,
        char const *b1, char const *b2, char const *e1, char const *e2
        ) {
    macro_parser mp(fgv, fgc, b1, e1, b2, e2);
    return mp.render_varsub_r(out, templ, templ_end, -1, -1); 
}
}

