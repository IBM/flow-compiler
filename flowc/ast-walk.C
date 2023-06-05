#include "ast.H"
#include <string>
#include <vector>
#include <cctype>
#include <set>

namespace {
bool is_num(std::string s) {
    for(auto c: s)
        if(!isdigit(c))
            return false;
    return true;
}
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
    std::vector<int> match;
    int n = node == 0? root(): node;
    if(path.empty()) {
        match.push_back(n);
        return match;
    }
    std::string tail;
    std::string pe = split(tail, path);
    bool anywhere = pe == "//";
    if(anywhere) 
        pe = split(tail, tail);
    std::vector<int> gv;

    if(is_num(pe)) {
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
    } else {
        int tk = pe == "*"? 0: string_to_tk(pe);
        if(anywhere) {
            for(auto x = begin(n), d = end(); x != d; ++x) 
                if(*x != n && (tk == 0 || at(*x).type == tk)) {
                    if(tail.empty()) 
                        match.push_back(*x);
                    else 
                        add_to_set(match, get(tail, *x));
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
    return match;
}

}
