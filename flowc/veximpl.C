#include <map>
#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <functional>
#include "stru.H"
#include "strsli.H"

#include <iostream>

namespace vex {

char const *default_escape_strings[4] = { "{", "{", "}", "}" };
struct macrodef {
    char const *ref = nullptr;
    char const *begin = nullptr; 
    char const *end = nullptr;
    char const *outer_begin = nullptr;
    char const *outer_end = nullptr;
    std::string default_value;
    std::string label;
    std::string expr;
    int start_index = 1;
    int default_count = -1;
    bool has_default = false;
    char counter_type = ' ';
    char type = '\0';
    int line = 0;
    int pos = 0;
    void setpos() {
        if(line == 0 && ref != nullptr && outer_begin != nullptr) {
            for(auto cp = ref; cp != outer_begin; ++cp) 
                if(*cp == '\n') {
                    ++line; 
                    pos = outer_begin - cp;
                }
            ++line;
        }
    }
};
inline static
std::ostream &operator << (std::ostream &o, macrodef const &m) {
    o << m.outer_begin-m.ref << " " << m.type << ":" << m.label  << "[" << m.expr << "]("<< m.default_value << ") " 
        << "start: "<< m.start_index << ", ct: " << m.counter_type 
        << ", dc: " << m.default_count
        << ", len: " << (m.end-m.begin) << ", has-default: " << m.has_default;
    return o;
}
static 
bool stringtobool(std::string const &s, bool bias=false) {
    if(s.empty()) return false;
    std::string so(s.length(), ' ');
    std::transform(s.begin(), s.end(), so.begin(), ::tolower);
    if(!bias) {
        return so == "yes" || so == "y" || so == "t" || so == "true" || so == "on" || so == "1";
    } else {
        return !(so == "no" || so == "n" || so == "f" || so == "false" || so == "off" || so == "0");
    }
}
/**
 * Variable reference container type
 */
class vref_ct {
    std::vector<std::pair<std::string, int>> store;
public:
    auto begin() const {
        return store.cbegin();
    }
    auto end() const {
        return store.cend();
    }
    void add(std::string str, int val) {
        for(auto p: store) 
            if(p.first == str)
                return;
        store.push_back(std::make_pair(str, val));
    }
    void add(vref_ct const &rhs) {
        for(auto p: rhs)
            add(p.first, p.second);
    }
};
static std::ostream &operator << (std::ostream &s, vref_ct &m) {
    s << "{";
    int c = 0;
    for(auto const &p: m) {
        if(++c > 1) s << ", ";
        s << p.first << ": "  << p.second;
    }
    return s << "}";
}
/** 
 * Scan for a macro reference in one of the forms below
 *      
 *      1. left1 left2 name [ / transforms ]* [ + name [/ transforms]* ]* - default right1 right2
 *      2. left1 label [: [-|+] [name [+name]*] [-integer] [+integer] ] left2 template right1 label right2
 *      3. left1 label ? #?name [|#?name]* [&#?name]* left2 template right1 label right2
 *
 *      Case 1 is a variable reference: a '+' separated list of identifiers optionally followed by a list of
 *      text transform functions, and followed by an optional default value. Evaluation stops at the first
 *      defined variable. If no default value is given then the macro is left as is in the output.
 *
 *      Case 2 renders 'template' repeatedly in a loop. The loop variable can be referenced in the template and yields 
 *      the current index, one based. The leading '+' or '-' determines how the loop count. If missing, the size of 
 *      the first macro referenced is used as counter. The smallest or the larger of the size of all referenced macros 
 *      is used when - or + are prepended, respectively. A '+' separated list of variables, followed by a default value
 *      can be supplied to be used instead of the referenced macros. A start index (one based) can be given as a '+' 
 *      prefixed integer.
 *
 *      In case 3 variables are evaluated as a boolean expression: empty string, no value, 0, or false for false, 
 *      anything else for true. If name is preceeded by #, the valute is true if name has more than one value or if
 *      has one value that is not the empty string.
 *
 *
 * Populates a descriptor structure and returns the number of errors encountered.
 * Errors are printed to the error stream.
 */
#define DBGC(c) if((c)) dbg()
#define DBG DBGC(debug) 
struct macro_parser {
    char const *left1;
    char const *left2;
    char const *right1;
    char const *right2;

    std::ostream *errp;
    std::string label;
    std::function<std::pair<int, std::string>(std::string const &, int)> da_getv;
    bool debug = false;

    // loop variable stack
    std::vector<std::pair<std::string, std::string>> loop_var_stack;
    // get a loop var value
    std::pair<int, std::string> lvs_get_value(std::string const &name, int index) const {
        for(auto p = loop_var_stack.rbegin(), e = loop_var_stack.rend(); p != e; ++p) 
            if(p->first == name)
                return std::make_pair(1, p->second);
        return std::make_pair(-1, "");
    }

    macro_parser(decltype(da_getv) gvf, 
            char const *b1 = default_escape_strings[0], 
            char const *b2 = default_escape_strings[1],
            char const *e1 = default_escape_strings[2],
            char const *e2 = default_escape_strings[3],
            std::string a_label = "vex", 
            std::ostream *a_errp = &std::cerr
            ):da_getv(gvf), left1(b1), right1(e1), left2(b2), right2(e2), errp(a_errp), label(a_label) {
    }
    void err(std::string const &message) const {
        if(errp != nullptr) *errp << label << ": " << message << "\n";
    }
    void err(macrodef &m, std::string const &message) const {
        m.setpos();
        if(errp != nullptr) *errp << label << "(" << m.line << ":" << m.pos << ") error: " << message << "\n";
    }
    std::ostream &dbg() const {
        std::ostream &ds = errp == nullptr? std::cerr: *errp;
        ds << label << ": debug: ";
        return ds;
    }
    /**
     * Match the 'obj' string at the start of the 'heap'. 
     * Returns a pointer past the match or nullptr for no match. 
     * Empty obj strings always match.
     */
    static char const *matchs(char const *obj, char const *heap, char const *end_heap) {
        if(obj == nullptr || heap == nullptr) return nullptr;
        char const *bo = obj, *bh = heap;
        while(*bo != '\0' && bh != end_heap && *bo == *bh) ++bo, ++bh;
        return *bo == '\0'? bh: nullptr;
    }
    /**
     * Look for the fist occurence of 'obj' in the 'heap'.
     * Returns the position of the match.
     * Empty 'obj' strings are never found.
     */
    static char const *find(char const *obj, char const *heap, char const *end_heap) {
        if(obj == nullptr || *obj == '\0')
            return nullptr;
        while(heap != end_heap) {
            // scan for the first character
            while(heap != end_heap && *heap != *obj) 
                ++heap;
            if(heap == end_heap) 
                break;
            if(obj[1] == '\0' || matchs(obj+1, heap+1, end_heap)) 
                return heap;
            ++heap;
        }
        return nullptr;
    }
    /**
     * Match an id at the current position: [a-z][_a-z.]*
     * Return a pointer past the match, or nullptr on no match.
     */
    static char const *match_id(char const *begin, char const *end, bool sizeop=false) {
        if(begin == nullptr || begin == end) return nullptr;
        char const *start = begin;
        if(sizeop && *start == '#') ++start;
        if(start == end || !std::isalpha(*start)) return nullptr;
        else ++start;
        while(start != end && (std::isalnum(*start) || *start == '_' || *start == '.')) ++start;
        return start;
    }
    /**
     * Match an integer at the current position [0-9]+
     * Return a pointer past the match or nullptr on no match.
     */
    static char const *match_integer(char const *begin, char const *end, int *vptr=nullptr) {
        if(begin == nullptr) return nullptr;
        char const *e = begin;
        while(e != end && std::isdigit(*e)) ++e;
        if(e == begin) return nullptr;
        if(vptr != nullptr) {
            std::string value(begin, e);
            *vptr = std::atoi(value.c_str());
        }
        return e;
    }
    /**
     * Match a list of ids, at the current position, separated by any of the characters in 'sep'.
     * Return a pointer past the match or nullptr for no match.
     */
    static char const *match_id_list(char const *begin, char const *end, char const *sep, bool sizeop=false) {
        auto p = match_id(begin, end, sizeop); 
        if(p != nullptr) while(p != end) {
            if(strchr(sep, *p) == nullptr) 
                break;
            auto np = match_id(p+1, end, sizeop);
            if(np == nullptr) 
                break;
            p = np;
        }
        return p;
    }
    /**
     * Match a macro begining at the outer begin separator.
     * Return the macro type found, or 0 for no match.
     */
    static char const *match_macro_begin(char const *begin, char const *end, char const *outer_begin, char const *inner_begin, macrodef *mdptr=nullptr) {
        char const *cp = matchs(outer_begin, begin, end);
        if(cp == nullptr)
            return nullptr;

        char const *tp = nullptr;
        macrodef m;
        if(mdptr != nullptr) m = *mdptr;
        m.type = '\0';
        m.outer_begin = begin;

        tp = match_id(cp, end);
        if(tp != nullptr) {
            m.label = std::string(cp, tp);
            cp = tp;
            // assume type loop
            if(cp != end && cp+1 != end) switch(*cp) {
                case ':': 
                    ++cp;
                    // counter type '+' max, '-' min default first
                    m.type = 'L';
                    if(matchs("+", cp, end) || matchs("-", cp, end)) 
                        m.counter_type = *cp++;

                    // match an id list separated by +
                    tp = match_id_list(cp, end, "+");
                    if(tp != nullptr) {
                        m.expr = std::string(cp, tp);
                        cp = tp;
                    }
                    // match a counter possibly preceded by '-'
                    if(!matchs("@", cp, end)) {
                        int ofs = matchs("-", cp, end)? 1: 0;
                        if((tp = match_integer(cp+ofs, end, &m.default_count)) == nullptr) 
                            break;
                        cp = tp;
                    }
                    if(matchs("@", cp, end)) {
                        if((tp = match_integer(cp+1, end, &m.start_index)) == nullptr) 
                            break;
                        cp = tp;
                    }
                    break;
                case '?': 
                    // expect a list of ids separated by & or |
                    tp = match_id_list(cp+1, end, "&|", true);
                    if(tp == nullptr)
                        break;
                    m.type = 'C';
                    m.expr = std::string(cp+1, tp);
                    cp = tp;
                    break;
                default:
                    m.type = 'L';
                    break;
            }
        } else {
            // assume type reference
            m.type = 'R';
        }
        tp = matchs(inner_begin, cp, end);
        if(tp != nullptr) {
            m.begin = tp;
            // save the md
            if(mdptr != nullptr) *mdptr = m;
        }
        return tp;
    }
    static char const *match_macro(char const *begin, char const *end, 
            char const *outer_begin, char const *inner_begin, 
            char const *inner_end, char const *outer_end,
            macrodef *mdptr=nullptr) {
        macrodef m; 
        char const *cp = match_macro_begin(begin, end, outer_begin, inner_begin, &m);
        if(cp == nullptr)
            return nullptr;
        // find the end of the macro
        std::string endm = std::string(inner_end) + m.label + outer_end;
        cp = find(endm.c_str(), m.begin, end);
        if(cp == nullptr)
            return nullptr;
        m.outer_end = cp + endm.length();
        m.end = cp;
        if(m.type == 'R') {
            // make sure the reference data is valid
            cp = match_id_list(m.begin, m.end, "+/");

            // expect a possible default value
            if(cp == nullptr)
                return nullptr;

            if(cp != m.end) {
                if(matchs("-", cp, m.end) == nullptr)
                    return nullptr;
                m.default_value = std::string(cp+1, m.end);
                m.has_default = true;
            }
            m.expr = std::string(m.begin, cp);
        }
        if(mdptr != nullptr)
            *mdptr = m;
        return m.outer_end;
    }
    char const *get_macro(char const *begin, char const *end, macrodef *mptr, char const *ref=nullptr) {
        macrodef m;
        char const *cp = nullptr; 
        if(begin != nullptr && begin != end) {
            cp = begin; 
            while(cp != end) {
                char const *tp = find(*left1 != '\0'? left1: left2, cp, end);
                cp = tp;
                if(tp == nullptr)
                    break;
                ++cp;
                tp = match_macro(tp, end, left1, left2, right1, right2, &m);
                m.ref = ref == nullptr? begin: ref;
                if(tp != nullptr) {
                    cp = tp;
                    break;
                }
            }
        }
        *mptr = m;
        return cp;
    }
    std::string transform(macrodef &m, std::string const &transf, std::string value, int &errc) {
        // apply transform here
        if(transf == "lower") {
            value = stru::to_lower(value);
        } else if(transf == "upper") {
            value = stru::to_upper(value);
        } else if(transf == "option" || transf == "cname") {
            value = stru::to_option(value, false);
        } else if(transf == "id" || transf == "identifier") {
            value = stru::to_identifier(value);
        } else if(transf == "html") {
            value = stru::html_escape(value);
        } else if(transf == "C" || transf == "c" || transf == "string") {
            value = stru::c_escape(value);
        } else if(transf == "shell" || transf == "sh") {
            value = stru::sh_escape(value);
        } else if(transf == "shell_comment" || transf == "shcom") {
            value = stru::to_line_comment(value, "# ");
        } else if(transf == "C_comment" || transf == "ccom" || transf == "c_comment") {
            value = stru::to_line_comment(value, "// ");
        } else if(transf == "trim") {
            value = stru::strip(value);
        } else if(transf == "ltrim") {
            value = stru::lstrip(nullptr, value, "\t\r\a\b\v\f\n ");
        } else if(transf == "rtrim") {
            value = stru::rstrip(nullptr, value, "\t\r\a\b\v\f\n ");
        } else if(transf == "lchop") {
            value = value.length() <= 1? std::string(): value.substr(1);
        } else if(transf == "rchop") {
            value = value.length() <= 1? std::string(): value.substr(0, value.length()-1);
        } else if(transf == "chop") {
            value = value.length() <= 2? std::string(): value.substr(1, value.length()-2);
        } else if(transf == "camelize") {
            value = stru::camelize(value);
        } else if(transf == "bool") {
            value = stringtobool(value)? "true": "false";
        } else if(transf == "decamelize") {
            value = stru::decamelize(value);
        } else if(transf == "underscore") {
            value = stru::to_underscore(value);
        } else if(transf == "base64") {
            value = stru::base64(value);
        } else if(transf == "base64d") {
            value = stru::base64d(value);
        } else if(transf == "json") {
            value = stru::json_escape(value);
        } else {
            // unrecognized transform function
            err(m, stru::sfmt() << "unrecognized transform '" << transf << "'");
            ++errc;
        }
        return value;
    }
    /** Return the value at 'index'. If the variable has one value only, return that regardless of index.
     */
    auto get_value(std::string const &name, int index) const {
        auto vi = lvs_get_value(name, 0);
        if(vi.first == 1)
            return vi;
        vi = da_getv(name, index);
        DBG << name << "[" << index << "]: " << vi.first << " (" << vi.second << ")\n";
        return vi.first != 1 || index == 0? vi: da_getv(name, 0);
    }
    int vex_value(std::string &value, macrodef &m, std::string names, int index, vref_ct *vref_cp) {
        int errc = 0; 
        bool have_value = false;
        for(auto varr: stru::splitter(names, "+")) {
            auto nt = stru::splitter(varr, "/");
            std::string name(nt.first());
            auto fv = get_value(name, index);
            if(fv.first <= 0) 
                continue;
            if(vref_cp != nullptr)
                vref_cp->add(name, fv.first);
            value = fv.second;
            for(auto transf: stru::splitter(nt.second(), "/"))
                value = transform(m, transf, value, errc);
            have_value = true;
            break;
        }
        if(!have_value && m.has_default) {
            value = m.default_value;
            have_value = true;
        }
        if(!have_value)
            err(m, stru::sfmt() << "variable " << stru::joint(stru::splitter(m.expr, "+"), [](std::string s) -> std::string { return std::string("'") + stru::splitter(s, "/").first() + "'";}, ", ")  << " not found");
        return have_value? errc: errc+1;
    }
    // Evaluate a simple boolean expression, missing variables are allowed.
    // TODO allow for parenthesized expressions
    bool eval_condition(std::string cexpr, int index, vref_ct *vref_cp=nullptr) {
        bool res = false;
        for(auto orex: stru::splitter(cexpr, "|")) {
            bool orterm = true;
            for(auto andterm: stru::splitter(orex, "&")) {
                bool bv;
                if(andterm[0] == '#') {
                    auto vv = get_value(andterm.substr(1), 0);
                    bv = vv.first > 1 || vv.first == 1 && stringtobool(vv.second, true);
                } else {
                    auto vv = get_value(andterm, index);
                    bv = vv.first >= 0 && index <= vv.first && stringtobool(vv.second, true);
                }
                if(!(orterm = orterm && bv))
                    break;
            }
            if(!!(res = res || orterm))
                break;
        }
        return res;
    }
    int count_from_refs(vref_ct const &vref_c, char counter_type, std::string *chosenp=nullptr) {
        int count = -1;
        std::string chosen;
        for(auto vs: vref_c) {
            switch(counter_type) {
                case '+':
                    if(vs.second > count) {
                        count = vs.second;
                        chosen = vs.first;
                    }
                    break;
                case '-':
                    if(count < 0 || vs.second < count) {
                        count = vs.second;
                        chosen = vs.first;
                    }
                    break;
                default:
                    if(count < 0) {
                        count = vs.second;
                        chosen = vs.first;
                    }
            }
        }
        if(chosenp != nullptr)
            *chosenp = chosen;
        return count;
    }
    int vex_count(int &count, macrodef &m, std::string vars, vref_ct *vref_cp=nullptr) {
        std::string chosen;
        vref_ct vref_c;
        int errcnt = 0;
        for(auto name: stru::splitter(vars, "+")) {
            auto vv = get_value(name, 0);
            if(vv.first > 0)
                vref_c.add(name, vv.first);
            /*
            else
                ++errcnt, err(m, stru::sfmt() << "counter variable '" << name << "' not found");
                */
        }
        if(m.default_count >= 0)
            vref_c.add("", m.default_count);
        count = count_from_refs(vref_c, m.counter_type);
        if(!chosen.empty() && vref_cp != nullptr)
            vref_cp->add(chosen, count);
        if(count < 0 && !vars.empty())
            count = 0;
        return errcnt;
    }
    int vex_r(std::ostream &out, char const *templ, char const *templ_end, int index, vref_ct *vref_cp, char const *ref) {
        if(templ_end == nullptr) templ_end = templ + strlen(templ);
        int errcnt = 0;
        macrodef m;
        std::string vval;
        std::ostringstream buff;
        char const *prevb = templ;
        while(get_macro(prevb, templ_end, &m, ref) != nullptr) {
            out << std::string(prevb, m.outer_begin);
            switch(m.type) {
                case 'R':
                    // expand variable reference
                    {
                        int rc = vex_value(vval, m, m.expr, index, vref_cp);
                        errcnt += rc;
                        out << (rc == 0? vval: std::string(m.outer_begin, m.outer_end));
                        //if(rc != 0) DBG << m.expr << ": " << rc << " error(s)\n";
                    }
                    break;
                case 'C':
                    // check condition and render template
                    {
                        bool eval = eval_condition(m.expr, index, vref_cp);
                        DBG << "cond: " << m.expr << "[" << index << "]:" << eval << "\n";
                        if(eval) {
                            buff.str("");
                            errcnt += vex_r(buff, m.begin, m.end, index, vref_cp, ref);
                            out << buff.str();
                        }
                    }
                    break;
                case 'L':
                    // loop and render template
                    {
                        vref_ct vref_c;
                        int count = -1;
                        errcnt += vex_count(count, m, m.expr, &vref_c);
                        int cx = std::max(m.start_index, 1)-1;
                        if(count > 0 && cx < count || count < 0) {
                            buff.str("");
                            loop_var_stack.push_back(std::make_pair(m.label, std::to_string(cx+1)));
                            errcnt += vex_r(buff, m.begin, m.end, cx, &vref_c, ref);
                            loop_var_stack.pop_back();
                        }
                        // if the count is not set, compute it from the references
                        if(count < 0) 
                            count = count_from_refs(vref_c, m.counter_type);
                        if(cx < count) {
                            out << buff.str();
                            if(vref_cp != nullptr)
                                vref_cp->add(vref_c);
                        }
                        for(++cx; cx < count; ++cx) {
                            buff.str("");
                            loop_var_stack.push_back(std::make_pair(m.label, std::to_string(cx+1)));
                            errcnt += vex_r(buff, m.begin, m.end, cx, &vref_c, ref);
                            loop_var_stack.pop_back();
                            out << buff.str();
                            if(vref_cp != nullptr)
                                vref_cp->add(vref_c);
                        }
                    }
                    break;
                default:
                    break;
            }
            prevb = m.outer_end;
        }
        out << std::string(prevb, templ_end);
        return errcnt;
    }
    int vex_r(std::ostream &out, char const *templ, char const *templ_end) {
        vref_ct vref_c;
        loop_var_stack.clear();
        int rc = vex_r(out, templ, templ_end, 0, &vref_c, templ);
        DBG << "refs: " << vref_c << "\n";
        if(rc != 0)
            err(stru::sfmt() << rc << " error(s)");
        return rc;
    }
};
int pand_impl(
        std::ostream &out, 
        char const *templ, char const *templ_end, 
        std::function<std::pair<int, std::string>(std::string const &, int)> fgv, 
        std::string label, bool debug,
        char const *bo, char const *bi, char const *ei, char const *eo, 
        std::ostream *eout
        ) {
    macro_parser mp(fgv, bo, bi, ei, eo, label, eout);
    mp.debug = debug;
    int rc = mp.vex_r(out, templ, templ_end);
    return rc;
}
}

