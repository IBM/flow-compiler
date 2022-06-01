#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <ftw.h>
#include <string>
#include <cassert>
#include <sys/stat.h>
#include <sys/time.h>
#include <vector>
#include <unistd.h>
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
/*
static std::string zerr(int ret) {
    std::string s;
    switch(ret) {
        case Z_ERRNO:
            s = "i/o error";
            break;
        case Z_STREAM_ERROR:
            s = "invalid compression level";
            break;
        case Z_DATA_ERROR:
            s = "invalid or incomplete deflate data";
            break;
        case Z_MEM_ERROR:
            s = "out of memory";
            break;
        case Z_VERSION_ERROR:
            s = "zlib version mismatch";
            break;
        default:
            break;
    }
    return s;
}
*/
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
}
