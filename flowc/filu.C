#include <array>
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <ftw.h>
#include <map>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "filu.H"
#include "stru.H"

namespace filu {
std::string gwd() {
    auto d = std::getenv("PWD");
    return d? std::string(d): std::string();
}
bool is_dir(char const *fn) {
	struct stat sb;
    if(stat(fn, &sb) == -1) 
		return false;	
    return (sb.st_mode & S_IFMT) == S_IFDIR;
}
bool is_reg(char const *fn) {
	struct stat sb;
    if(stat(fn, &sb) == -1) 
		return false;	
    return (sb.st_mode & S_IFMT) == S_IFREG;
}
void chmodx(std::string const &fn) {
	struct stat sb;
    char const *fpath = fn.c_str();
    if(stat(fpath, &sb) == -1) 
		return;	

	mode_t chm = sb.st_mode;
    if(chm & S_IRUSR) chm = chm | S_IXUSR;
    if(chm & S_IRGRP) chm = chm | S_IXGRP;
    if(chm & S_IROTH) chm = chm | S_IXOTH;
    if(chm != sb.st_mode)
        chmod(fpath, chm);
}
std::string realpath(std::string const &f) {
    if(f.empty()) return "";
    char actualpath[PATH_MAX+1];
    auto r = ::realpath(f.c_str(), actualpath);
    return (r == nullptr) ? "" : r;
}
int cp_p(std::string const &source, std::string const &dest) {
    struct stat a_file_stat, b_file_stat;
    if(stat(source.c_str(), &a_file_stat) != 0 || S_ISDIR(a_file_stat.st_mode)) 
        return 1;
    if(stat(dest.c_str(), &b_file_stat) == 0 && a_file_stat.st_dev == b_file_stat.st_dev && a_file_stat.st_ino == b_file_stat.st_ino)
        return 0;
    bool ok = true;
    char buf[16384];
    size_t size;
    int sd = open(source.c_str(), O_RDONLY, 0);
    int dd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    while((size = read(sd, buf, sizeof(buf))) > 0) 
        ok = write(dd, buf, size) == size && ok;
    close(sd); close(dd);
    struct timeval tv[2];
    tv[0].tv_sec = a_file_stat.st_atime; tv[0].tv_usec = 0;
    tv[1].tv_sec = a_file_stat.st_mtime; tv[1].tv_usec = 0;
    utimes(dest.c_str(), &tv[0]);
    return ok? 0: 1;
}
int write_file(std::string const &fn, std::string const &source_fn, char const *default_content, size_t length) {
    if(!source_fn.empty()) 
        return cp_p(source_fn, fn);
    std::ofstream outf(fn.c_str());
    if(!outf.is_open()) 
        return 1;
    return outf.write(default_content, length != 0? length: strlen(default_content))? 0: 1;
}
std::string search_path(std::string const &bin, std::string const &venvn) {
    struct stat  st;
    char const *pathv = std::getenv(venvn.c_str());
    if(pathv == nullptr) 
        return bin;
    std::vector<std::string> buf;
    for(auto const &path: stru::split(buf, pathv, ":")) {
        std::string file(path+"/"+bin);
        if(stat(file.c_str(), &st) >= 0 && (st.st_mode & S_IEXEC) != 0)
            return file;
    }
    return bin;
}
std::string basename(std::string const &filename, std::string const &suffix, std::string *dirname) {
    auto lsl = filename.find_last_of("/");
    if(dirname != nullptr) 
        *dirname = lsl == std::string::npos? std::string(): filename.substr(0, lsl);
    std::string f(lsl == std::string::npos? filename: filename.substr(lsl+1));
    if(suffix.empty() || suffix.length() > f.length() || 
            f.substr(f.length()-suffix.length()) != suffix)
        return f;
    else
        return f.substr(0, f.length()-suffix.length());
}
int url_split(std::string const &url, std::string *scheme, std::string *host, std::string *path, std::string *query, std::string *section) {
    int c = 0;
    std::string buf(url), tmp;
    auto p = buf.find("://");
    if(p != std::string::npos) {
        tmp = buf.substr(0, p);
        c = tmp.empty()? 0: 1;
        if(scheme != nullptr) *scheme = tmp;
        buf = buf.substr(p+3);
    }
    if(buf.length() == 0)
        return c;
    p = buf.find_first_of("/?#");
    if(p == std::string::npos) {
        if(host != nullptr) *host = buf;
        return c | 2;
    }
    if(p > 0) c |= 2;
    if(host != nullptr) *host = buf.substr(0, p);
    buf = buf.substr(p);
    p = buf[0] == '/'? 0: std::string::npos;
    auto q = buf.find_first_of("?#");
    auto s = q;
    if(q != std::string::npos && buf[q] == '#') 
        q = std::string::npos;
    else
        s = buf.find_first_of('#', q);
    if(p != std::string::npos) {
        auto e = std::min(q, s);
        tmp = e == std::string::npos? buf.substr(p): buf.substr(p, e-p);
        c |= tmp.empty()? 0: 4;
        if(path != nullptr) *path = tmp;
    }
    if(q != std::string::npos) {
        tmp = s == std::string::npos? buf.substr(q+1): buf.substr(q+1, s-q-1);
        c |= tmp.empty()? 0: 8;
        if(query != nullptr) *query = tmp;
    }
    if(s != std::string::npos) {
        tmp = buf.substr(s+1);
        c |= tmp.empty()? 0: 16;
        if(section != nullptr) *section = tmp;
    }
    return c;
}
std::string path_split(std::string const &path, std::string *second) {
    if(path.empty()) return "";
    auto s = path.find_first_not_of("/");
    if(s == std::string::npos) return "/";
    auto e = path.find_first_of("/", s+1);
    if(e == std::string::npos) {
        return path.substr(s);
    }
    if(second != nullptr) {
        auto f = path.find_first_not_of("/", e);
        if(f != std::string::npos) {
            auto g = path.find_last_not_of("/");
            *second = path.substr(f, g+1-f);
        }
    }
    return path.substr(s, e-s);
}
std::string reads(std::istream &s) {
    std::stringstream buffer;
    buffer << s.rdbuf();
    return buffer.str();
}
std::string pipe(std::string command, std::string input, int *rc, std::string *err, bool err2out) {
    // pipe descriptors for each in, out and err streams
    int fdin[2], fdou[2], fder[2];
    std::string sout, serr;
    std::array<char, 16384> buffer;
    // don't redirect stderr to stdout if stderr is captured
    if(err != nullptr)
        err2out = false;
    // process return code
    int prc = 0;
    // read counter
    int rds = 0;

    // open the input and output streams
    if(::pipe(fdin) == -1 || ::pipe(fdou) == -1 || ::pipe(fder) == -1) 
        exit(1);
    
    pid_t npid = ::fork();
    switch(npid) {
        case -1:   // failed to fork
            prc = 1;
            break;
        case 0:    // child process, exec 
            // entrance of stdout pipe gets connected to stdout of child
            while((::dup2(fdou[1], STDOUT_FILENO) == -1) && (errno == EINTR));

            // entrance of stderr or stdout pipe gets connected to stderr of child 
            if(err2out) 
                while((::dup2(fdou[1], STDERR_FILENO) == -1) && (errno == EINTR));
            else 
                while((::dup2(fder[1], STDERR_FILENO) == -1) && (errno == EINTR));
            ::close(fdou[1]);
            ::close(fder[1]);
            // output of the stderr and stdout pipes are not needed in the child
            ::close(fdou[0]);
            ::close(fder[0]);
    
            // output of stdin needs to be connected to stdin
            while((::dup2(fdin[0], STDIN_FILENO) == -1) && (errno == EINTR));
            ::close(fdin[0]);
            // input of the stdin pipe is not needed here
            ::close(fdin[1]);

            ::execl("/bin/bash", "bash", "-c", command.c_str(), nullptr);
            // if exec fails
            _exit(errno);
            break;
        default:   // main process, wait 
            // close the exit of the stdin pipe
            ::close(fdin[0]); 
            // close the entry of the stdout and stderr pipes
            ::close(fdou[1]); 
            ::close(fder[1]);
            // send the input 
            if(!input.empty())
                write(fdin[1], input.data(), input.length());
            ::close(fdin[1]);
            // read stdout
            while((rds = read(fdou[0], buffer.data(), buffer.size())) != 0) 
                sout += std::string(buffer.data(), buffer.data()+rds);
            ::close(fdou[0]);
            // read stderr
            if(err != nullptr)
                while((rds = read(fder[0], buffer.data(), buffer.size())) != 0) 
                    serr += std::string(buffer.data(), buffer.data()+rds);
            ::close(fder[0]);
            wait(&prc);
    }
    if(err != nullptr) *err = serr;
    if(rc != nullptr) *rc = prc;

    return sout;
}
}
