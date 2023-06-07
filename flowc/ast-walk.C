#include "ast.H"
#include <string>
#include <vector>
#include <cctype>
#include <set>
#include <iostream>
#include "stru.H"
#include "strsli.H"

namespace {
bool is_num(std::string s) {
    for(auto c: s)
        if(!isdigit(c))
            return false;
    return true;
}

/** Crude expression parser for path expression
 * return //    
 *        node
 *        child-idx
 *        *
 *        (expr)
 */
std::string split(std::string &tail, std::string str) { 
    if(str.substr(0, 2) == "//") {
        tail = str.substr(2);
        while(tail.length() > 0 && tail[0] == '/')
            tail = tail.substr(1);
        return "//";
    }
    while(str.length() > 0 && str[0] == '/') 
        str = str.substr(1);
    if(str.empty()) {
        tail = str;
        return "";
    }

    if(str[0] == '(') {
        unsigned len = 0, pc = 0;
        while(len < str.length() && !(str[len] == ')' && pc == 1)) {
            if(str[len] == '(') ++pc;
            else if(str[len] == ')') --pc;
            ++len;
        }
        if(pc != 1 || len >= str.length()) {
            std::cerr << "path expression error [" << str << "]\n";
            assert(false);
        }
        std::string pex = str.substr(0, len+1); 
        tail = str.substr(len+1);
        return pex;
    }

    auto e = str.find_first_of("/[");
    auto s = str;
    if(e == std::string::npos) {
        tail = ""; 
    } else {
        s = str.substr(0, e);
        tail = str.substr(e);
    }
    return s;
}
void add_to_set(std::vector<int> &dest, std::vector<int> const &src) {
    std::set<int> have(dest.begin(), dest.end());
    for(int i: src) if(have.find(i) == have.end()) {
        have.insert(i); dest.push_back(i);
    }
}
}

namespace ast {

std::vector<int> tree::get(std::string path, int node) const {
    int n = node == 0? root(): node;
    std::vector<int> match;
    std::string tail;
    std::string pe = split(tail, path);

    bool anywhere = pe == "//";
    if(anywhere) 
        pe = split(tail, tail);

    if(pe[0] == '(') { // path expression
        for(auto en: stru::splitter(pe.substr(1, pe.length()-2), "|")) 
            add_to_set(match, get((anywhere? std::string("//"): std::string())+en+tail, n));
    } else if(is_num(pe)) { // child index
        int pen = atol(pe.c_str());
        if(anywhere) {
            for(auto x = begin(n), d = end(); x != d; ++x) 
                if(at(*x).children.size() >= pen) {
                    if(tail.empty()) 
                        match.push_back(at(*x).children[pen-1]);
                    else 
                        add_to_set(match, get(tail, at(*x).children[pen-1]));
                }
        } else {
            if(at(n).children.size() >= pen) {
                if(tail.empty()) 
                    match.push_back(at(n).children[pen-1]);
                else 
                    add_to_set(match, get(tail, at(n).children[pen-1]));
            }
        }
    } else { // field or *
        int tk = pe == "*"? 0: string_to_tk(pe);
        if(anywhere) {
            for(auto x = begin(n), d = end(); x != d; ++x) {
                if(*x != n && (tk == 0 || at(*x).type == tk)) {
                    if(tail.empty()) 
                        match.push_back(*x);
                    else 
                        add_to_set(match, get(tail, *x));
                }
            }
        } else {
            for(int x: at(n).children) 
                if(x != n && (tk == 0 || at(x).type == tk)) {
                    if(tail.empty()) 
                        match.push_back(x);
                    else 
                        add_to_set(match, get(tail, x));
                }
        }
    }
    if(match.size() > 1) {
        std::set<int> found(match.begin(), match.end());
        match.clear();
        for(auto x = begin(n), d = end(); x != d; ++x) 
            if(found.find(*x) != found.end()) 
                match.push_back(*x);
        if(match.size() != found.size()) {
            std::cerr << "internal error matches outside subtree " << n << " " << path << "\n";
            assert(false);
        }
    }
    return match;
}

}
