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
#include <unistd.h>
#include <vector>
#include <zlib.h>

#include "filu.H"
#include "stru1.H"

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
#define GZPIPE_CHUNK 16384

int gzipx(std::ostream &dest, long &source_size, std::istream &source, bool gzip, int level) {
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[GZPIPE_CHUNK];
    unsigned char out[GZPIPE_CHUNK];
    source_size = 0;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = gzip? deflateInit2(&strm, level, Z_DEFLATED, 16+15, 8, Z_DEFAULT_STRATEGY):
        deflateInit(&strm, level);

    if(ret != Z_OK)
        return ret;

    do {
        source.read((char *) in, GZPIPE_CHUNK);
        strm.avail_in = source.gcount();
        source_size += strm.avail_in;
        if(source.bad()) {
            deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = source.eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = GZPIPE_CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = GZPIPE_CHUNK - strm.avail_out;
            dest.write((char *)out, have);
            if(dest.bad()) {
                deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     // all input should have been used
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        // stream will be complete 

    deflateEnd(&strm);
    return Z_OK;
}
long gzip(std::ostream &dest, std::istream &source, bool gz) {
    long uncompressed_size = 0;
    bool rc = gzipx(dest, uncompressed_size, source, gz, 9) == Z_OK;
    return rc? uncompressed_size: -1;
}
int gunzipx(std::ostream &dest, long &source_size, std::istream &source, bool gzip) {
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[GZPIPE_CHUNK];
    unsigned char out[GZPIPE_CHUNK];
    source_size = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = gzip? inflateInit2(&strm, 16): inflateInit(&strm);
    if(ret != Z_OK)
        return ret;

    do {
        source.read((char *) in, GZPIPE_CHUNK);
        strm.avail_in = source.gcount();
        source_size += strm.avail_in;
        if(source.bad()) {
            inflateEnd(&strm);
            return Z_ERRNO;
        }
        if(strm.avail_in == 0)
            break;
        strm.next_in = in;
        do {
            strm.avail_out = GZPIPE_CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;         /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&strm);
                return ret;
            }
            have = GZPIPE_CHUNK - strm.avail_out;
            dest.write((char *)out, have);
            if(dest.bad()) {
                inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while(strm.avail_out == 0);

    } while(ret != Z_STREAM_END);

    inflateEnd(&strm);
    return ret == Z_STREAM_END? Z_OK: Z_DATA_ERROR;
}
long gunzip(std::ostream &dest, std::istream &source, bool gz) {
    long source_size = 0;
    bool rc = gunzipx(dest, source_size, source, gz) == Z_OK;
    return rc? source_size: -1;
}
std::string search_path(std::string const &bin) {
    struct stat  st;
    char const *pathv = std::getenv("PATH");
    if(pathv == nullptr) 
        return bin;
    std::vector<std::string> buf;
    for(auto const &path: stru1::split(buf, pathv, ":")) {
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
std::string gunzip(unsigned char const *buffer, unsigned zlen, bool gz) {
    std::ostringstream buf;
    std::istringstream zbuf(std::string((char const *)buffer, zlen));
    filu::gunzip(buf, zbuf, gz);
    return buf.str();
}
std::string pipe(std::string command, std::string input, int *rc, std::string *err, bool err2out) {
    // pipe descriptors for each in, out and err streams
    std::array<int, 6> pipefd;
    pid_t npid = -1;
    int prc = 0, rds = 0;
    std::array<char, 16384> buffer;
    std::string sout;

    // open the input and output streams
    ::pipe(pipefd.data());
    ::pipe(pipefd.data()+2);
    // open the err stream
    if(err != nullptr && !err2out)
        ::pipe(pipefd.data()+4);
    else
        pipefd[4] = pipefd[5] = -1;

    npid = ::fork();
    switch(npid) {
        case -1:   // failed to fork
            prc = 1;
            break;
        case 0:    // the child process, exec here
            ::close(pipefd[1]);
            ::dup2(pipefd[0], STDIN_FILENO);
            ::close(pipefd[0]);
            ::close(pipefd[2]);
            ::dup2(pipefd[3], STDOUT_FILENO);
            if(pipefd[4] >= 0) ::close(pipefd[4]);
            if(err2out)
                ::dup2(pipefd[3], STDOUT_FILENO);
            else if(pipefd[5] >= 0)
                ::dup2(pipefd[5], STDOUT_FILENO);
            ::close(pipefd[3]);
            if(pipefd[4] >= 0) ::close(pipefd[5]);

            ::execl("/bin/bash", "bash", "-c", command.c_str(), nullptr);
            // if exec failes
            _exit(errno);
            break;
        default:   // main process, wait here
            ::close(pipefd[0]); 
            ::close(pipefd[3]);
            if(pipefd[5] >= 0) ::close(pipefd[5]);
            pipefd[0] = pipefd[3] = pipefd[5] = -1;

            if(!input.empty())
                write(pipefd[1], input.c_str(), input.length());
            ::close(pipefd[1]);
            pipefd[1] = -1;
            while((rds = read(pipefd[2], buffer.data(), buffer.size())) != 0) {
                sout += std::string(buffer.data(), buffer.data()+rds);
            }
            ::close(pipefd[2]); 
            pipefd[2] = -1;
            if(err != nullptr)
                while((rds = read(pipefd[4], buffer.data(), buffer.size())) != 0) {
                    *err += std::string(buffer.data(), buffer.data()+rds);
                }
            ::close(pipefd[4]); 
            pipefd[4] = -1;
            wait(&prc);
    }

    for(int i = 0; i < pipefd.size(); ++i) 
        if(pipefd[i] >= 0 && ::close(pipefd[i])) {
            pipefd[i] = -1;
        }

    if(rc != nullptr) *rc = prc;
    return sout;
}
static 
bool cfg_line(std::string &name, std::string &value, std::string line, std::string separators, std::string unquote="") {
    if(stru1::split(&name, &value, line, separators) != 2)
        return false;
    name = stru1::strip(name);
    value = stru1::strip(value);
    if(!unquote.empty() && !value.empty()) {
        auto lqp = unquote.find_first_of(*value.begin());
        if(lqp != std::string::npos && lqp+1 < unquote.length() && *value.rbegin() == unquote[lqp+1]) {
            if(unquote[lqp] == '"')
                value = stru1::json_unescape(value);
            else 
                value = value.substr(1, value.length()-2);
        }
    }
    return true;
}

bool config(std::map<std::string, std::string> &cfg, std::istream &ins, std::string comment_triggers, bool case_sensitive, std::string separators) {
    std::string line;
    int errors = 0;
    while(std::getline(ins, line)) {
        auto p = line.find_first_not_of("\t\r\a\b\v\f\n ");
        if(p == std::string::npos || comment_triggers.find_first_of(line[p]) != std::string::npos)
            continue;
        std::string name, value;
        if(!cfg_line(name, value, line, separators, "\"\""))
            ++errors;
        if(case_sensitive) 
            cfg[name] = value;
        else
            cfg[stru1::to_lower(name)] = value;
    }
    return errors == 0;
}

bool config(std::map<std::string, std::string> &cfg, std::string name, bool search) {
    std::string filename = name;
    while(search) {
        filename = std::string(".")+name;
        if(is_reg(filename)) 
            break;
        filename = name+".cfg";
        if(is_reg(filename)) 
            break;
        auto home = std::getenv("HOME");
        if(home == nullptr)
            break;
        filename = path_join(home, std::string(".")+name);
        if(is_reg(filename)) 
            break;
        filename = path_join(home, name+".cfg");
        search = false;
    }
    std::ifstream inputs(filename.c_str());
    return inputs.is_open() && config(cfg, inputs);
}
std::string reads(std::istream &s) {
    std::stringstream buffer;
    buffer << s.rdbuf();
    return buffer.str();
}

bool ini_section(std::string &result, std::istream &fs, std::string section) {
    section = stru1::to_lower(section);
    std::string line;
    int errors = 0, section_count = 0;
    bool found_section = false;
    std::ostringstream buf;
    while(std::getline(fs, line)) {
        auto p = line.find_first_not_of("\t\r\a\b\v\f\n ");
        if(p == std::string::npos || line[p] == ';') {
            buf << line << "\n";
            continue;
        }
        std::string secd = stru1::strip(line);
        if(secd.length() > 2 && *secd.begin()== '[' && *secd.rbegin() == ']') {
            ++section_count;
            if(stru1::to_lower(secd.substr(1, secd.length()-2)) == section) {
                // reset the accumulator buffer
                buf.str("");
                found_section = true;
            } else if(found_section) {
                result = buf.str();
                return true;
            }
            // avoid accumulationg section defs
            continue;
        }
        buf << line << "\n";
    }

    if(found_section || section_count == 0 && (section.empty() || section == "default")) {
        result = buf.str();
        return true;
    }
    return false;
}
}
