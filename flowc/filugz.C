#include <cassert>
#include <fstream>
#include <sstream>
#include <zlib.h>

#include "filu.H"

namespace filu {
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
std::string gunzip(unsigned char const *buffer, unsigned zlen, bool gz) {
    std::ostringstream buf;
    std::istringstream zbuf(std::string((char const *)buffer, zlen));
    filu::gunzip(buf, zbuf, gz);
    return buf.str();
}
}
