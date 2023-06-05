#include "ast.H"
#include "strsli.H"
#include "stru.H"
#include <string>
#include <vector>
#include <cctype>

#include <iostream>

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
}

namespace ast {

std::vector<int> tree::get(std::string path, int node) const {
    std::vector<int> match;
    int n = node == 0? root(): node;
    if(path.empty()) {
        match.push_back(n);
        return match;
    }
    bool anywhere = path.substr(0, 2) == "//";
    if(anywhere) path = path.substr(2);
    while(path.substr(0, 1) == "/") { 
        path = path.substr(1);
        n = root();
    }
    stru::splitter pss(path, "/");

    std::string pe = *pss.begin();
    std::string tail = pss.begin().tail();
    std::vector<int> gv;
    if(pe.empty()) {
                        
        gv = get(std::string("//") + tail, n);
        match.insert(match.end(), gv.begin(), gv.end());
        
    } else if(is_num(pe)) {
        int pen = atol(pe.c_str());
        if(anywhere) {
            for(auto x = begin(n), d = end(); x != d; ++x) 
                if(at(*x).children.size() >= pen) {
                    if(tail.empty()) {
                        match.push_back(at(*x).children[pen-1]);
                    } else {
                        gv = get(tail, at(*x).children[pen-1]);
                        match.insert(match.end(), gv.begin(), gv.end());
                    }
                }
        } else {
            if(at(n).children.size() >= pen) {
                if(tail.empty()) {
                    match.push_back(at(n).children[pen-1]);
                } else {
                    gv = get(tail, at(n).children[pen-1]);
                    match.insert(match.end(), gv.begin(), gv.end());
                }
            }
        }
    } else {
        int tk = pe == "*"? 0: string_to_tk(pe);
        if(anywhere) {
            for(auto x = begin(n), d = end(); x != d; ++x) 
                if(*x != n && (tk == 0 || at(*x).type == tk)) {
                    if(tail.empty()) {
                        match.push_back(*x);
                    } else {
                        gv = get(tail, *x);
                        match.insert(match.end(), gv.begin(), gv.end());
                    }
                }

        } else {
            for(int x: at(n).children) 
                if(x != n && (tk == 0 || at(x).type == tk)) {
                    if(tail.empty()) {
                        match.push_back(x);
                    } else {
                        gv = get(tail, x);
                        match.insert(match.end(), gv.begin(), gv.end());
                    }
                }
        }
    }
    return match;
}

}
