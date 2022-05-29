/************************************************************************************************************
 *
 * {{NAME}}-server.C 
 * generated from {{MAIN_FILE_SHORT}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ratio>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <grpc++/grpc++.h>
#include <grpc++/health_check_service_interface.h>
#include <grpc++/resource_quota.h>
#include <google/protobuf/util/json_util.h>

#include <ares.h>
extern char **environ;

#if !defined(NO_REST) || !(NO_REST)    
extern "C" {
#include <civetweb.h>
}
#endif
namespace flowc {
inline static bool stringtobool(std::string const &s, bool default_value=false) {
    if(s.empty()) return default_value;
    std::string so(s.length(), ' ');
    std::transform(s.begin(), s.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(so.c_str()) != 0;
}
inline static bool strtobool(char const *s, bool default_value=false) {
    if(s == nullptr || *s == '\0') return default_value;
    return stringtobool(std::string(s), default_value);
}
}
namespace flowrt {
enum flow_type {INTEGER, FLOAT, STRING};
inline static 
std::string getenv(std::string const &name, std::string const &default_v="") {
    auto v = ::getenv(name.c_str());
    return v == nullptr? default_v: std::string(v);
}
}
{H:HAVE_DEFN{namespace flowdef {
typedef long vt_INTEGER;
typedef std::string vt_STRING;
typedef double vt_FLOAT;
static inline vt_STRING to_vt_STRING(std::string const &s) { return s; }
static inline vt_FLOAT to_vt_FLOAT(std::string const &s) { return std::strtod(s.c_str(), nullptr); }
static inline vt_INTEGER to_vt_INTEGER(std::string const &s) { return std::atol(s.c_str()); }
{I:DEFN{{{DEFD/ccom}}
vt_{{DEFT}} v{{DEFN}}(to_vt_{{DEFT}}(flowrt::getenv("{{NAME/id/upper}}_FD_{{DEFN/upper}}", {{DEFV/string}})));
}I}
}
}H}
namespace flowrt {
inline static
int next_utf8_cp(std::string const &s, int bpos = 0) {
    int e = s.length();
    if(e <= bpos) return -1;
    char c = s[bpos++];
    if((c & 0x80) == 0) return bpos;
    if(bpos == e || (s[bpos++] & 0xC0) != 0x80) return -bpos;
    if((c & 0xE0) == 0xC0) return bpos;
    if(bpos == e || (s[bpos++] & 0xC0) != 0x80) return -bpos;
    if((c & 0xF0) == 0xE0) return bpos;
    if(bpos == e || (s[bpos++] & 0xC0) != 0x80) return -bpos;
    if((c & 0xF1) == 0xF0) return bpos;
    return -bpos;
}
inline static
int length(std::string const &s) {
    int bp = -1, nbp = 0, l = -1;
    do 
        nbp = next_utf8_cp(s, bp = nbp), ++l;
    while(nbp >= 0);
    return l;
}
inline static
int cpos(std::string const &s, int pos) {
    int bp = 0, nbp = 0;
    while(pos > 0) { 
        nbp = next_utf8_cp(s, bp);
        if(nbp < 0) break;
        bp = nbp;
        --pos;
    }
    return pos == 0? bp: -bp;
}
inline static
std::string substr(std::string const &s, int begin, int end) {
    if(s.empty()) return s;
    if(end < 0) end += length(s);
    if(begin < 0) begin += length(s);
    if(begin >= end) return "";
    begin = cpos(s, begin); 
    if(begin < 0) return "";
    end = cpos(s, end);
    if(end < 0) return "";
    return s.substr(begin, end-begin);
}
inline static
std::string substr(std::string const &s, int begin) {
    return substr(s, begin, length(s));
}
inline static
int size(std::string const &s) {
    return s.length();
}
inline static
std::string strslice(std::string const &s, int begin, int end) {
    if(s.empty()) return s;
    if(end < 0) end += s.length();
    if(begin < 0) begin += s.length();
    if(begin >= end) return "";
    return s.substr(begin, end-begin);
}
inline static
std::string strslice(std::string const &s, int begin) {
    return strslice(s, begin, s.length());
}
inline static
std::string toupper(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::toupper);
    return u;
}
inline static
std::string tolower(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    return u;
}
inline static
std::string camelize(std::string const &s, bool capitalize_first=false) {
    std::ostringstream out;
    bool cap_next = capitalize_first;
    for(char c: s) 
        if(isspace(c)) {
            cap_next = true;
        } else {
            out << (cap_next? ::toupper(c): c);
            cap_next = false;
        }
    return out.str();
}
inline static 
std::string decamelize(std::string const &s) {
    std::ostringstream out;

    char prev_c = ' ';
    for(unsigned sl = s.length(), i = 0; i < sl; ++i) {
        char c = s[i];
        if(::isspace(c) || c == '_') {
            if(prev_c != ' ') out << ' ';
            prev_c = ' ';
        } else if(::isalpha(c) && ::toupper(c) == c) {
            if((::isalpha(prev_c) && ::toupper(prev_c) != prev_c) ||
               (::isalpha(prev_c) && ::toupper(prev_c) == prev_c && i+1 < sl && ::isalpha(s[i+1]) && ::toupper(s[i+1]) != s[i+1]))
                out << ' ';
            out << (prev_c = c);
        } else {
            out << (prev_c = c);
        }
    }
    return out.str();
}
inline static
std::string tocname(std::string const &s, bool lower=true) {
    std::string u;
    char l = '\0';
    for(char c: s) {
        c = ::tolower(c);
        if(c == '_') c = '-';
        if(l != '-') u += c;
        l = c;
    }
    return u;
}
inline static
std::string toid(std::string const &s) {
    std::string u;
    char l = '\0';
    for(char c: s) {
        if(c == '-' || c == '.') c = '_';
        if(l != '_') u += c;
        l = c;
    }

    return u;
}
inline static
std::string ltrim(std::string const &s, std::string const &chars="\t\r\a\b\v\f\n ") {
    auto lp = s.find_first_not_of(chars);
    return lp == std::string::npos? std::string(): s.substr(lp);
}
inline static
std::string rtrim(std::string const &s, std::string const &chars="\t\r\a\b\v\f\n ") {
    auto rp = s.find_last_not_of(chars);
    return rp == std::string::npos? std::string(): s.substr(0, rp+1);
}
inline static
std::string trim(std::string const &s, std::string const &chars="\t\r\a\b\v\f\n ") {
    return ltrim(rtrim(s, chars), chars);
}
inline static
std::string tr(std::string const &s, std::string const &match, std::string const &replace="") {
    std::string r;
    for(auto c: s) {
        auto mp = match.find_first_of(c);
        if(mp == std::string::npos) r += c;
        else if(mp < replace.length()) r += replace[mp];
    }
    return r;
}
template <class T> inline static 
std::string join(int acc_max, T&& acc, std::string const &sep, std::string const &last_sep) {
    std::ostringstream buff;
    for(int i = 0; i < acc_max; ++i) {
        if(i > 0) {
            if(i + 1 == acc_max)
                buff << last_sep;
            else 
                buff << sep;
        }
        buff << acc(i);
    }
    return buff.str();
}
template <class T> inline static 
std::string join(int acc_max, T&& acc, std::string const &sep="") {
    return join(acc_max, acc, sep, sep);
}
template <typename N, typename A> inline static
N min(A l) { return (N)l; }
template<typename N, typename A, typename... As> inline static 
N min(A first, As... args) {
    return std::min((N) first, min(args...));
}
}
#if !defined(NO_REST) || !(NO_REST)    
/**********************************************************************************************************
 * REST headers 
 */
#define RFH_ALT_CALL_ID "x-call-id"
#define RFH_CALL_ID "x-flow-call-id"
#define RFH_CALL_TIMES "X-Flow-Call-Times"
#define RFH_CHECK "x-flow-check"
#define RFH_OVERLAPPED_CALLS "x-flow-overlapped-calls"
#define RFH_TIME_CALL "x-flow-time-call"
#define RFH_TIMEOUT "x-flow-timeout"
#define RFH_TRACE_CALL "x-flow-trace-call"
#endif
/**********************************************************************************************************
 * gRPC headers 
 */
#define GFH_CALL_ID "call-id"
#define GFH_CALL_TIMES "times-bin"
#define GFH_NODE_ID "node-id"
#define GFH_OVERLAPPED_CALLS "overlapped-calls"
#define GFH_START_TIME "start-time"
#define GFH_TIME_CALL "time-call"
#define GFH_TRACE_CALL "trace-call"
/**********************************************************************************************************
 * Hardcoded default values for certain configurable parameters
 */
#ifndef DEFAULT_GRPC_THREADS
#define DEFAULT_GRPC_THREADS 0
#endif
#ifndef DEFAULT_REST_THREADS
#define DEFAULT_REST_THREADS 48
#endif
#ifndef DEFAULT_CARES_REFRESH
#define DEFAULT_CARES_REFRESH 30
#endif
#ifndef DEFAULT_ENTRY_TIMEOUT
#define DEFAULT_ENTRY_TIMEOUT 3600000
#endif
#ifndef DEFAULT_NODE_TIMEOUT
#define DEFAULT_NODE_TIMEOUT 3600000
#endif
#ifndef REST_CONNECTION_CHECK_INTERVAL
#define REST_CONNECTION_CHECK_INTERVAL 5000
#endif
inline static std::ostream &operator << (std::ostream &out, std::chrono::steady_clock::duration time_diff) {
    auto td = double(time_diff.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
    char const *unit = "s";
    if(td < 1.0) { td *= 1000; unit = "ms"; }
    if(td < 1.0) { td *= 1000; unit = "\\u00B5s"; }
    if(td < 1.0) { td *= 1000; unit = "ns"; }
    if(td < 1.0) { td = 0; unit = ""; }
    return out << td << unit;
}
template <class FE>
inline static std::ostream &operator << (std::ostream &out, std::set<FE> const &c) {
    char const *sep = "";
    out << "(";
    for(auto const &e: c) {
        out << sep << e;
        sep = ", ";
    }
    out << ")";
    return out;
}
template <class FE>
inline static std::ostream &operator << (std::ostream &out, std::vector<FE> const &c) {
    char const *sep = "";
    out << "[";
    for(auto const &e: c) {
        out << sep << e;
        sep = ", ";
    }
    out << "]";
    return out;
}
template <class F, class E>
inline static std::ostream &operator << (std::ostream &out, std::pair<F, E> const &c) {
    char const *sep = "";
    out << "(" << c.first << ", " << c.second << ")";
    return out;
}
template <class F, class E>
inline static std::ostream &operator << (std::ostream &out, std::map<F,E> const &c) {
    char const *sep = "";
    out << "{";
    for(auto const &e: c) {
        out << sep << e.first << ": "<< e.second;
        sep = ", ";
    }
    out << "}";
    return out;
}
namespace flowc {
inline static
std::string to_lower(std::string const &s) {
    std::string u(s);
    std::transform(s.begin(), s.end(), u.begin(), ::tolower);
    return u;
}
static std::string get_system_time() {
    timeval tv;
    gettimeofday(&tv, 0);
    struct tm *nowtm = localtime(&tv.tv_sec);
    char tmbuf[64], buf[256];
    strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
    static char const *seconds_format = sizeof(tv.tv_sec) == sizeof(long)?  "%s.%03ld": "%s.%03d";  
    snprintf(buf, sizeof(buf), seconds_format, tmbuf, tv.tv_usec/1000);
    return buf;
}
inline static long strtolong(char const *s, long default_value=0) {
    if(s == nullptr || *s == '\0') return default_value;
    char *endptr = nullptr;
    auto value = strtol(s, &endptr, 10);
    if(endptr == s) return default_value;
    for(; *endptr != '\0' && isspace(*endptr); ++endptr);
    if(*endptr == '\0') return value;
    return default_value;
}
inline static long stringtolong(std::string const &s, long default_value=0) {
    return strtolong(s.c_str(), default_value);
}
inline static std::string strtostring(char const *s, std::string const &default_value) {
    if(s == nullptr || *s == '\0') return default_value;
    return s;
}
static std::string strip(std::string const &str, std::string const &strip_chars="\t\r\a\b\v\f\n ") {
    auto b = str.find_first_not_of(strip_chars);
    if(b == std::string::npos) b = 0;
    auto e = str.find_last_not_of(strip_chars);
    if(e == std::string::npos) return "";
    return str.substr(b, e-b+1);
}
static size_t split(std::vector<std::string> &buf, std::string const &str, std::string const &separators) {
    for(auto b = str.find_first_not_of(separators), e = (b == std::string::npos? b: str.find_first_of(separators, b));
        b != std::string::npos; 
        b = (e == std::string::npos? e: str.find_first_not_of(separators, e)),
        e = (b == std::string::npos? b: str.find_first_of(separators, b))) 
        buf.push_back(e == std::string::npos? str.substr(b): str.substr(b, e-b));
    return buf.size();
}
static std::string get_metadata_string(std::multimap<grpc::string_ref, grpc::string_ref> const & mm, std::string const &key, std::string const &default_value="") { 
    auto vp = mm.find(key);
    if(vp == mm.end()) return default_value;
    return std::string((vp->second).data(), (vp->second).length());
}
static bool get_metadata_bool(std::multimap<grpc::string_ref, grpc::string_ref> const &mm, std::string const &key, bool default_value=false) { 
    auto vp = mm.find(key);
    if(vp == mm.end()) return default_value;
    return stringtobool(std::string((vp->second).data(), (vp->second).length()), default_value);
}
#if defined(NO_UUID)
static std::string server_id() {
    srand(time(nullptr));
    return std::string("{{NAME}}-R-")+std::to_string(rand());
}
#endif
#if defined(OSSP_UUID)
#include <uuid.h>
static std::string server_id() {
    uuid_t *puid = nullptr;
    char *sid = nullptr;;
    if(uuid_create(&puid) == UUID_RC_OK) {
        uuid_make(puid, UUID_MAKE_V4);
        uuid_export(puid, UUID_FMT_STR, &sid, nullptr);
        std::string id = std::string("{{NAME}}-")+sid;
        uuid_destroy(puid);
        return id;
    }
    srand(time(nullptr));
    return std::string("{{NAME}}-R-")+std::to_string(rand());
}
#endif
#if !defined(OSSP_UUID) && !defined(NO_UUID)
#include <uuid/uuid.h>
static std::string server_id() {
    uuid_t id;
    uuid_generate(id);
    char sid[64];
    uuid_unparse(id, sid);
    return std::string("{{NAME}}-")+sid;
}
#endif

#ifndef MAX_REST_REQUEST_SZIE
/** Size limit for the  REST request **/
#define MAX_REST_REQUEST_SIZE 1024ul*1024ul*100ul
#endif

static char const *get_cfg(std::vector<std::string> const &cfg, std::string const &name) {
    for(auto p = cfg.rbegin(), e = cfg.rend(); p != e; ++p, ++p) {
        auto n = p + 1;
        if(*n == name) return p->c_str();
    }
    return nullptr;
}
enum cli_nodes {
    cn_{{NO_NODE_NAME/id/upper-NO_NODE}} = 0 {I:CLI_NODE_NAME{, cn_{{CLI_NODE_NAME/id/upper}}}I}
};
enum nodes {
    n_{{NO_NODE_NAME/id/upper-NO_NODE}} = 0 {I:A_NODE_NAME{, n_{{A_NODE_NAME/id/upper}}}I}
};
struct node_cfg {
    cli_nodes eid;
    std::string id;
    std::set<std::string> fendpoints;
    std::set<std::string> dendpoints;
    std::set<std::string> dnames;
    std::string endpoint;
    std::string cert_filename;
    int maxcc;
    long timeout;
    bool trace;
    std::shared_ptr<grpc::ChannelCredentials> creds;

    node_cfg(cli_nodes a_eid, std::string const &a_id, int a_maxcc, long a_timeout, std::string const &a_endpoint, std::string const &cert_fn):
        eid(a_eid), id(a_id), maxcc(a_maxcc), timeout(a_timeout), endpoint(a_endpoint), cert_filename(cert_fn), trace(false), creds(nullptr) {
    }
    bool read_from_cfg(std::vector<std::string> const &cfg);
    bool check_option(std::string const &) const;
};
{I:CLI_NODE_NAME{node_cfg ns_{{CLI_NODE_NAME/lower/id}}(cn_{{CLI_NODE_NAME/id/upper}}, "{{CLI_NODE_NAME/lower/id}}", /*maxcc*/{{CLI_NODE_MAX_CONCURRENT_CALLS}}, /*timeout*/{{CLI_NODE_TIMEOUT-DEFAULT_NODE_TIMEOUT}}, "{{CLI_NODE_ENDPOINT-}}", "{{CLI_NODE_CERTFN-}}");
}I}
{I:ENTRY_NAME{long entry_{{ENTRY_NAME}}_timeout = {{ENTRY_TIMEOUT-DEFAULT_ENTRY_TIMEOUT}};
}I}
std::string global_node_ID;
bool asynchronous_calls = true;
bool trace_calls = false;
bool send_global_ID = true;
bool trace_connections = false;
bool accumulate_addresses = false;

std::string global_start_time = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1000); 
static std::string json_string(const std::string &s) {
    std::ostringstream o;
    o << '\"';
    for(auto c: s) {
        switch (c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if('\x00' <= c && c <= '\x1f') {
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c);
            } else {
                o << c;
            }
        }
    }
    o << '\"';
    return o.str();
}
static std::string log_abridge(std::string const &message, unsigned max_length=256) {
    if(max_length == 0 || message.length() <= max_length) return message;
    return message.substr(0, (max_length - 5)/2) + " ... " + message.substr(message.length()-(max_length-5)/2);
}
static std::string log_abridge(google::protobuf::Message const &message, unsigned max_length=256) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = false;
    options.always_print_primitive_fields = false;
    options.preserve_proto_field_names = true;
    std::string json_reply;
    google::protobuf::util::MessageToJsonString(message, &json_reply, options);
    return log_abridge(json_reply, max_length);
}
struct call_info {
    long id;
    std::string id_str;
    std::string entry_name;
    std::unique_ptr<std::stringstream> tissp;
    bool time_call, async_calls, trace_call, return_protobuf, have_deadline;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point deadline;

#if !defined(NO_REST) || !(NO_REST)    
    call_info(std::string const &entry, long num, struct mg_connection *A_conn, long default_timeout): 
            entry_name(entry), id(num), start_time(std::chrono::system_clock::now()) {

        char const *header = mg_get_header(A_conn, RFH_CALL_ID);
        if(header == nullptr) header = mg_get_header(A_conn, RFH_ALT_CALL_ID);
        if(header != nullptr) id_str = header;
        async_calls = flowc::strtobool(mg_get_header(A_conn, RFH_OVERLAPPED_CALLS), flowc::asynchronous_calls);
        time_call = flowc::strtobool(mg_get_header(A_conn, RFH_TIME_CALL), time_call);
        trace_call = flowc::strtobool(mg_get_header(A_conn, RFH_TRACE_CALL), flowc::trace_calls);
        if(time_call) {
            tissp.reset(new std::stringstream);
            *tissp << "[";
        }
        header = mg_get_header(A_conn, "accept");
        return_protobuf = header != nullptr && (
            strcasecmp(header, "application/protobuf") == 0 || 
            strcasecmp(header, "application/x-protobuf") == 0 || 
            strcasecmp(header, "application/vnd.google.protobuf") == 0);

        header = mg_get_header(A_conn, RFH_TIMEOUT);
        if(header == nullptr) {
            have_deadline = true;
            deadline = start_time + std::chrono::milliseconds(default_timeout);
        } else {
            long timeout_ms = flowc::strtolong(header, 0);
            if((have_deadline = timeout_ms > 0)) 
                deadline = start_time + std::chrono::milliseconds(timeout_ms);
        }
    }
#endif

    call_info(std::string const &entry, long num, ::grpc::ServerContext *ctx, long default_timeout): 
            entry_name(entry), id(num), 
            return_protobuf(true), have_deadline(true), start_time(std::chrono::system_clock::now()), deadline(ctx->deadline()) {
        auto const &md = ctx->client_metadata();
        id_str = flowc::get_metadata_string(md, GFH_CALL_ID), 
        trace_call = flowc::get_metadata_bool(md, GFH_TRACE_CALL, flowc::trace_calls);
        time_call = flowc::get_metadata_bool(md, GFH_TIME_CALL);
        async_calls = flowc::get_metadata_bool(md, GFH_OVERLAPPED_CALLS, flowc::asynchronous_calls);
        if(time_call) {
            tissp.reset(new std::stringstream);
            *tissp << "[";
        }
    }
    std::ostream &printcc(std::ostream &out, int cc=0) const {
        out << "[" << id;
        if(cc != 0) out << ":" << cc;
        if(!id_str.empty()) out << "~" << id_str;
        out << "] ";
        return out;
    }
    std::string get_time_info() const {
        if(time_call)
            return tissp->str() + "]";
        return "";
    }
    void record_time_info(int stage, std::string const &stage_name, std::chrono::steady_clock::duration call_elapsed_time, std::chrono::steady_clock::duration stage_duration, int calls) const {
        if(stage != 1) *tissp << ",";
        *tissp << "{" 
            "\"method\":" << json_string(entry_name) << ","
            "\"stage-name\":" << json_string(stage_name) << ","
            "\"stage\":" << stage << ","
            "\"calls\":" << calls << ","
            "\"duration\":" << double(stage_duration.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den << ","
            "\"started\":" << double(call_elapsed_time.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den << ","
            "\"duration-u\":\"" << stage_duration << "\","
            "\"started-u\":\"" << call_elapsed_time << "\""
            "}";
    }
};
}
inline static std::ostream &operator << (std::ostream &out, flowc::call_info const &cid) {
    return cid.printcc(out, 0);
}
inline static std::ostream &operator << (std::ostream &out, std::tuple<flowc::call_info const *, int> const &ccid) {
    return std::get<0>(ccid)->printcc(out, std::get<1>(ccid));
}
namespace flowc {
struct sfmt {
    std::ostringstream os;
    inline operator std::string() {
        return os.str();
    }
    template <class T> 
    inline sfmt &operator <<(T const &v);
};
template <class T>
sfmt &sfmt::operator <<(T const &v) {
    os << v; return *this;
}
std::mutex global_display_mutex;
class flog {
public:
    flog &operator <<= (std::string const &v) {
        flowc::global_display_mutex.lock();
        std::cerr << v << std::flush; 
        flowc::global_display_mutex.unlock();
        return *this;
    }
};
/* cg helpers
 */
void closeq(::grpc::CompletionQueue &q) {
    void *tag; bool ok = false;
    q.Shutdown();
    while(q.Next(&tag, &ok));
}
}

#define FLOGC(c) if(c) flowc::flog() <<= flowc::sfmt() << flowc::get_system_time() << " " 
#define FLOG FLOGC(true)

namespace casd {
static bool is_hostname(std::string const &name) {
    if(strcasecmp(name.c_str(), "localhost") == 0) 
        return false;
    uint64_t ip[2] = {0, 0};

    ares_inet_pton(AF_INET, name.c_str(), &ip);
    if(ip[0] || ip[1]) 
        return false;
    ares_inet_pton(AF_INET6, name.c_str(), &ip);
    return !(ip[0] || ip[1]);
}
static std::string arg_to_endpoint(std::string const &arg) {
    bool arg_is_number = false;
    for(auto a: arg) 
        if(!(arg_is_number = std::isdigit(a))) 
            break;
    if(!arg_is_number) return arg;
        return std::string("localhost:") + arg;
}
static std::string name_from_endpoint(std::string const &endpoint) {
    auto pp = endpoint.find_last_of(':');
    if(pp == std::string::npos) return endpoint;

    if(pp > 1 && endpoint[0] == '[' && endpoint[pp-1] == ']')
        return endpoint.substr(1, pp-2);
    else if(pp == endpoint.find_first_of(':'))
        return endpoint.substr(0, pp);
    else 
        return endpoint;
}
static std::string port_from_endpoint(std::string const &endpoint) {
    auto pp = endpoint.find_last_of(':');
    if(pp == std::string::npos) return "";
    return endpoint.substr(pp+1);
}
static std::mutex address_store_mutex;
static std::map<std::string, std::tuple<std::set<std::string>, std::string, int>> address_store;
static int last_changed_iteration = 0;
static int current_iteration = 0;
static std::set<std::string> deleted_ips;
static std::vector<std::tuple<std::string, std::set<std::string>, std::string>> updates;
static void remove_address(std::string const &ip) {
    std::lock_guard<std::mutex> guard(address_store_mutex);
    deleted_ips.insert(ip);
}
static void state_cb(void *data, int s, int read, int write) {
    //FLOG << "change state fd: " << s << " read " << read << " write " << write << "\n";
}
static void update_addresses(std::string const &entry, std::set<std::string> const &ips, std::string const &host_name) {
    auto asp = address_store.find(entry);
    if(asp == address_store.end()) {
        address_store[entry] = std::make_tuple(std::set<std::string>(), host_name, current_iteration);
        asp = address_store.find(entry);
        last_changed_iteration = current_iteration;
    } else {
        std::get<1>(asp->second) = host_name;
    }

    std::set<std::string> &current_set = std::get<0>(asp->second);
    std::set<std::string> addresses;

    bool changed;
    if(flowc::accumulate_addresses) {
        size_t previous_size = current_set.size();
        for(auto ip: ips) 
            if(deleted_ips.find(ip) == deleted_ips.end())
                current_set.insert(ip);
        changed = previous_size != current_set.size();
    } else {
        changed = current_set.size() != ips.size();
        for(auto a: ips) {
            addresses.insert(a);
            if(!changed && current_set.find(a) == current_set.end())
                changed = true;
        }
        if(changed) std::swap(addresses, current_set);
    }

    if(changed) {
        std::get<2>(asp->second) = last_changed_iteration = current_iteration;
        FLOGC(flowc::trace_connections) << "ADDRESSES for " << entry << " version=" << std::get<2>(asp->second) << ", " << std::get<0>(asp->second) << "\n";
    }
}
static void callback(void *arg, int status, int timeouts, struct hostent *host) {
    std::string lookup((char const *) arg);
    if(!host || status != ARES_SUCCESS){
        FLOG << "failed to lookup " << lookup << ": " << ares_strerror(status) << "\n";
        return;
    }
    std::set<std::string> ips;
    char ip[INET6_ADDRSTRLEN];
    for(int i = 0; host->h_addr_list[i]; ++i) {
        ares_inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
        ips.insert(ip);
    }
    updates.emplace_back(std::make_tuple(lookup, ips, std::string(host->h_name)));
}
static void wait_ares(ares_channel channel) {
    for(;;) {
        struct timeval *tvp, tv;
        fd_set read_fds, write_fds;
        int nfds;

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        nfds = ares_fds(channel, &read_fds, &write_fds);
        if(nfds == 0) 
            break;
        tvp = ares_timeout(channel, NULL, &tv);
        select(nfds, &read_fds, &write_fds, NULL, tvp);
        ares_process(channel, &read_fds, &write_fds);
    }
}
static bool stop_looking = false;
static int keep_looking(std::set<std::string> const &dnames,  int interval_s, int loops) {
    if(dnames.size() == 0) {
        FLOG << "ip watcher: no names to look up, leaving.\n";
        return 0;
    }
    FLOGC(flowc::trace_connections) "ip watcher: every " << interval_s << "s lookig for " << dnames << "\n";

    ares_channel channel1;
    struct ares_options options;
    int optmask = 0;

    //options.sock_state_cb_data;
    options.sock_state_cb = state_cb;
    optmask |= ARES_OPT_SOCK_STATE_CB;
    int status = ares_init_options(&channel1, &options, optmask);
    if(status != ARES_SUCCESS) {
        FLOG << "ares_init_options: " << ares_strerror(status) << "\n";
        return 1;
    }

    for(current_iteration = 1; (loops == 0 || current_iteration <= loops) && !stop_looking; ++current_iteration) {
        updates.resize(0);
        int dname_count = 0;
        for(auto const &endpoint: dnames) if(endpoint[0] != '(') {
            char const *lookup_host = endpoint.c_str();
            ares_gethostbyname(channel1, lookup_host, AF_INET, callback, (void *) lookup_host);
            dname_count += 1;
        }
        if(dname_count > 0)
            wait_ares(channel1);

        for(auto const &plugin: dnames) if(plugin[0] == '(') {
            std::array<char, 4096> buffer;
            std::string cmd = plugin.substr(1, plugin.length()-2);
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
            std::string ip_list;
            if(!pipe) {
                FLOG << "plugin \"" << plugin << "\" returned a non-zero code\n";
                continue;    
            }
            while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
                ip_list += buffer.data();
            std::vector<std::string> ips;
            flowc::split(ips, ip_list, "\t\r\a\b\v\f\n ");
            updates.emplace_back(std::make_tuple(plugin, std::set<std::string>(ips.begin(), ips.end()), std::string()));
        }
        if(updates.size() > 0) {
            std::lock_guard<std::mutex> guard(address_store_mutex);
            for(auto &ut: updates) 
                update_addresses(std::get<0>(ut), std::get<1>(ut), std::get<2>(ut));
            deleted_ips.clear();
        }
        sleep(interval_s);
    }

    ares_destroy(channel1);
    FLOG << "ip watcher: finished.\n";
    return 0;
}
/**
 * dnames: set of lookup addresses or plug-in commands
 * dendpoints: endpoints that generated dnames
 * fendpoints: fixed enpoints
 * ins: input string
 */
static
bool parse_endpoint_list(std::set<std::string> &dnames, std::set<std::string> &dendpoints, std::set<std::string> &fendpoints, std::string const &ins) {
    char const *separators = ",;\t\r\a\b\v\f\n ";
    auto begin = ins.find_first_not_of(separators);
    auto end = ins.find_last_not_of(separators);
    if(end == std::string::npos || begin == std::string::npos || end == begin) 
        return false;

    while(begin != std::string::npos && begin != end) {
        auto ss = begin;
        auto se = ss;
        bool is_plugin = false;
        if(ins[begin] == '(') {
            se = ins.find_first_of(")", ss+1);
            if(se == std::string::npos) return false;
            is_plugin = true;
        }
        se = ins.find_first_of(separators, se);
        if(se == std::string::npos) 
            se = ins.length();
        if(ss == se) break;
        std::string ep = ins.substr(ss, se-ss);
        if(is_plugin) {
            dendpoints.insert(ep);
            dnames.insert(name_from_endpoint(ep));
        } else if(ep[0] ==  '@') {
            dendpoints.insert(ep.substr(1));
            dnames.insert(name_from_endpoint(ep.substr(1)));
        } else {
            fendpoints.insert(ep);
        }
        begin = ins.find_first_not_of(separators, se);
    }
    return true;
}
int print_address_store(int version = -1) {
    std::lock_guard<std::mutex> guard(address_store_mutex);
    if(version == last_changed_iteration) {
        FLOGC(flowc::trace_connections) << "address_store not changed since: " << version << "\n";
        return version;
    }
    for(auto const &ase: address_store) {
        FLOG << "addrs for " << ase.first << " [" << std::get<2>(ase.second) << "]: " << std::get<0>(ase.second) << "\n";
    }
    return last_changed_iteration;
}
int get_latest_addresses(std::map<std::string, std::vector<std::string>> &addrs, int version, std::set<std::string> const &dnames) {
    int last_version = version;
    std::lock_guard<std::mutex> guard(address_store_mutex);
    if(version == last_changed_iteration) 
        return version;
    for(auto const &dname: dnames) {
        auto asep = address_store.find(dname);
        if(asep == address_store.end()) continue;
        if(std::get<2>(asep->second) > version) {
            addrs[dname].assign(std::get<0>(asep->second).begin(), std::get<0>(asep->second).end());
            last_version = std::max(last_version, std::get<2>(asep->second));
        }
    }
    return last_version;
}
}

{I:GRPC_GENERATED_H{#include "{{GRPC_GENERATED_H}}"
}I}
class {{NAME/id}}_service;
namespace flowc {
class {{NAME/id}}_service *{{NAME/id}}_service_ptr = nullptr;

#define GRPC_ERROR(cid, ccid, text, status, context) FLOG << std::make_tuple(&cid, ccid) << (text) << " grpc error: " << (status).error_code() << " context: " << (context).debug_error_string() << "\n";

#define PRINT_TIME(CIF, stage, stage_name, call_elapsed_time, stage_duration, calls) {\
    if(CIF.time_call) CIF.record_time_info(stage, stage_name, (call_elapsed_time), (stage_duration), calls);\
    FLOGC(CIF.trace_call) << CIF << "time-call: " << CIF.entry_name << " stage " << stage << " (" << stage_name \
    << ") started after " << call_elapsed_time << " and took " << stage_duration << " for " << calls << " call(s)\n"; \
    }

template<class CSERVICE> class connector {
    typedef typename CSERVICE::Stub Stub_t;
    typedef decltype(std::chrono::system_clock::now()) ts_t;
    std::vector<std::tuple<int, ts_t, std::unique_ptr<Stub_t>, std::string, std::string>> stubs;
    std::vector<int> activity_index;
    unsigned long cc;
    unsigned total_active;
    std::mutex allocator;
    std::unique_ptr<Stub_t> dead_end;
public:
    std::atomic<int> const &active_calls;
    std::string label; 
    std::map<std::string, std::vector<std::string>> addresses;
    connector(std::atomic<int> const &a_active_calls, node_cfg const &ns, std::map<std::string, std::vector<std::string>> const &a_addresses): 
        cc(0), total_active(0),
        active_calls(a_active_calls),
        label(ns.id), addresses(a_addresses) {
        int maxcc = a_addresses.size() > 0? 1: ns.maxcc;
        int i = 0;
        for(auto const &aep: ns.fendpoints) for(int j = 0; j < maxcc; ++j) {
            FLOGC(flowc::trace_connections) << "creating @" << label << " stub " << i << " -> " << aep << "\n";
            std::shared_ptr<::grpc::Channel> channel(::grpc::CreateChannel(aep, ns.creds));
            stubs.emplace_back(std::make_tuple(0, std::chrono::system_clock::now(), CSERVICE::NewStub(channel), aep, std::string()));
            activity_index.emplace_back(i);
            ++i;
        }
        for(auto const &aep: ns.dendpoints) {
            auto pp = aep.find_last_of(':');
            if(pp == std::string::npos) {
                FLOG << "skipping invalid endpoint for @" << label << "(" <<aep << ") missing port value\n"; 
                continue;
            }
            auto addrp = addresses.find(aep.substr(0, pp));
            if(addrp == addresses.end()) {
                FLOG << "skipping endpoint for @" << label << "(" <<aep << ") no addresses available\n"; 
            } else for(auto const &ipaddr: addrp->second) {
                std::string ipep(ipaddr.find_first_of(':') == std::string::npos?
                    sfmt() << ipaddr << aep.substr(pp):
                    sfmt() << "[" << ipaddr << "]" << aep.substr(pp));
                std::shared_ptr<::grpc::Channel> channel(::grpc::CreateChannel(ipep, ns.creds));
                FLOGC(flowc::trace_connections) << "creating @" << label << " stub " << i << " -> " << aep << " (" << ipaddr << ")\n";
                stubs.emplace_back(std::make_tuple(0, std::chrono::system_clock::now(), CSERVICE::NewStub(channel), aep, ipaddr));
                activity_index.emplace_back(i);
                ++i;
            }
        }
        if(i == 0) 
            dead_end = CSERVICE::NewStub(::grpc::CreateChannel("localhost:0", ::grpc::InsecureChannelCredentials()));
    }
    size_t count() const {
        return stubs.size();
    }
    std::string log_allocation() const {
        std::stringstream slog;
        slog << "allocation @" << label << " " << stubs.size() << " [";
        auto sep = "";
        for(auto const &s: stubs) { slog << sep << std::get<0>(s); sep = " "; }
        slog << "] " << total_active << " / " << active_calls.load();
        return slog.str();
    }
    Stub_t *stub(int &connection_number, flowc::call_info const &scid, int ccid) {
        std::lock_guard<std::mutex> guard(allocator);
        auto index = ++cc;
        auto time_now = std::chrono::system_clock::now();
        if(stubs.size() == 0)  {
            connection_number = -1;
            return dead_end.get();
        }
        std::sort(activity_index.begin(), activity_index.end(), [this](int const &x1, int const &x2) -> bool {
            if(std::get<0>(stubs[x1]) != std::get<0>(stubs[x2]))
                return std::get<0>(stubs[x1]) < std::get<0>(stubs[x2]);
            return std::get<0>(stubs[x1]) < std::get<0>(stubs[x2]);
        });
        connection_number = activity_index[0]; 
        auto &stubt = stubs[connection_number];
        FLOGC(flowc::trace_connections) << std::make_tuple(&scid, ccid) << "using @" << label << " stub[" << connection_number << "] for call #" << index 
            << ", to: " << std::get<3>(stubt) << (std::get<4>(stubt).empty() ? "": "(") << std::get<4>(stubt) << (std::get<4>(stubt).empty() ? "": ")")
            << ", active: " << std::get<0>(stubt) 
            << "\n";
        std::get<1>(stubt) = std::chrono::system_clock::now();
        std::get<0>(stubt) += 1;
        total_active += 1;
        FLOGC(flowc::trace_connections) << std::make_tuple(&scid, ccid) << "+ " << log_allocation() << "\n";
        return std::get<2>(stubt).get();
    }
    void finished(int &connection_number, flowc::call_info const &scid, int ccid, bool in_error) {
        std::lock_guard<std::mutex> guard(allocator);
        FLOGC(flowc::trace_connections) << std::make_tuple(&scid, ccid) << "releasing @" << label << " stub[" << connection_number << "]\n";
        if(connection_number < 0 || connection_number+1 > count())
            return;
        auto &stubt = stubs[connection_number];
        connection_number = -1;
        std::get<0>(stubt) -= 1;
        total_active -= 1;
        FLOGC(flowc::trace_connections) << std::make_tuple(&scid, ccid) << "- " << log_allocation() << "\n";
        if(in_error && flowc::accumulate_addresses && !std::get<4>(stubt).empty()) {
            FLOGC(flowc::trace_connections) << std::make_tuple(&scid, ccid) << "dropping @" << label << " stub[" << connection_number << "] to: " 
                << std::get<3>(stubt) << "(" << std::get<4>(stubt) <<  ")\n";
            // Mark with a very high count so it won't be allocated anymore
            std::get<0>(stubt) -= 100000;
            casd::remove_address(std::get<4>(stubt));
        }
    }
    template <class INTP>
    void release(flowc::call_info const &scid, INTP begin, INTP end) {
        while(begin != end) {
            int p = *begin;
            if(p >= 0) finished(p, scid, -1, false);
            ++begin;
        }
    }
};
}

{I:SERVER_XTRA_H{#include "{{SERVER_XTRA_H}}"
}I}
#ifndef GRPC_RECEIVED
#define GRPC_RECEIVED(NODE_NAME, SERVER_CALL_ID, CLIENT_CALL_ID, CLI_NODE_ID, STATUS, CONTEXT, RESPONSE_PTR)
#endif
#ifndef GRPC_SENDING
#define GRPC_SENDING(NODE_NAME, SERVER_CALL_ID, CLIENT_CALL_ID, CLI_NODE_ID, CONTEXT, REQUEST_PTR)
#endif
{I:ENTRY_NAME{
#ifndef GRPC_LEAVE_{{ENTRY_NAME}}
#define GRPC_LEAVE_{{ENTRY_NAME}}(ENTRY_NAME, SERVER_CALL_ID, STATUS, CONTEXT, RESPONSE_PTR)
#endif
#ifndef GRPC_ENTER_{{ENTRY_NAME}}
#define GRPC_ENTER_{{ENTRY_NAME}}(ENTRY_NAME, SERVER_CALL_ID, CONTEXT, REQUEST_PTR)
#endif
// http_code, return value. It must be set to the HTTP status code
// http_message, return value. It must be set to the HTTP status messge
// json_body, return value. Should be set to the body of the reply (json). If left empty, {"code": http_code, "message": http_msessage} will be returned.
// check_header, value of the header 'X-Flow-Check'. Used to convey per request information to the check before and after hooks.
// context is a reference to the gRPC context to be used in the call
// inp is a pointer to the gRPC {{ENTRY_INPUT_TYPE}} message that will be sent
// outp is a pointer to the received {{ENTRY_OUTPUT_TYPE}} message
// xtra_headers, return value. A string where HTTP headers can be appended
// status is a reference to the gRCP status returned by the call
// return true if check succeeded and results can be ignored (except xtra_headers).
 
// rest_check_{{ENTRY_NAME}}_before(int &http_code, std::string &http_message, std::string &json_body, char const *check_header, ::grpc::ClientContext &context, {{ENTRY_INPUT_TYPE}} *inp, std::string &xtra_headers);  
// rest_check_{{ENTRY_NAME}}_after(int &http_code, std::string &http_message, std::string &json_body, char const *check_header, ::grpc::ClientContext &context, {{ENTRY_INPUT_TYPE}} *inp, ::grpc::Status const &status,  {{ENTRY_OUTPUT_TYPE}} *outp, std::string &xtra_headers);  
//
// to use, define REST_CHECK_{{ENTRY_NAME/id/upper}}_BEFORE and/or REST_CHECK_{{ENTRY_NAME/id/upper}}_AFTER with the name of function
}I}

class {{NAME/id}}_service final: public {{CPP_SERVER_BASE}}::Service {
public:
    bool Async_Flag = flowc::asynchronous_calls;
    // Global call counter used to generate an unique ID for each call regardless of entry
    std::atomic<long> Call_Counter;
    // Current number of active calls regardless of entry
    std::atomic<int> Active_Calls;

{I:CLI_NODE_NAME{
    // {{MAIN_FILE_SHORT}}:{{CLI_NODE_LINE}}: {{CLI_NODE_NAME}} ({{CLI_NODE}})
#define SET_METADATA_{{CLI_NODE_NAME/id}}(context) {{CLI_NODE_METADATA}}
    std::mutex {{CLI_NODE_NAME/id}}_conm; // mutex to guard {{CLI_NODE_NAME/id}}_nversion and {{CLI_NODE_NAME/id}}_conp
    int {{CLI_NODE_NAME/id}}_nversion = 0;
    std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> {{CLI_NODE_NAME/id}}_conp;

    std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> {{CLI_NODE_NAME/id}}_get_connector() {
        std::lock_guard<std::mutex> guard({{CLI_NODE_NAME/id}}_conm);
        if(flowc::ns_{{CLI_NODE_NAME/id}}.dendpoints.size() == 0) {
            return {{CLI_NODE_NAME/id}}_conp;
        }
        std::map<std::string, std::vector<std::string>> addresses = {{CLI_NODE_NAME/id}}_conp->addresses;
        int ver = casd::get_latest_addresses(addresses, {{CLI_NODE_NAME/id}}_nversion,  flowc::ns_{{CLI_NODE_NAME/id}}.dnames);
        if(ver == {{CLI_NODE_NAME/id}}_nversion) 
            return {{CLI_NODE_NAME/id}}_conp;
        {{CLI_NODE_NAME/id}}_nversion = ver;

        FLOGC(flowc::trace_connections) << "new @{{CLI_NODE_NAME}} connector to " << flowc::ns_{{CLI_NODE_NAME/id}}.fendpoints << " " << flowc::ns_{{CLI_NODE_NAME/id}}.dendpoints << "\n"; 
        auto {{CLI_NODE_NAME/id}}_cp = new ::flowc::connector<{{CLI_SERVICE_NAME}}>({{CLI_NODE_NAME/id}}_conp->active_calls, flowc::ns_{{CLI_NODE_NAME/id}}, addresses);
        {{CLI_NODE_NAME/id}}_conp.reset({{CLI_NODE_NAME/id}}_cp);
        return {{CLI_NODE_NAME/id}}_conp;
    }
    std::unique_ptr<::grpc::ClientAsyncResponseReader<{{CLI_OUTPUT_TYPE}}>> {{CLI_NODE_NAME/id}}_prep(int &ConN, flowc::call_info const &CIF, int CCid,
            std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> ConP, ::grpc::CompletionQueue &CQ, ::grpc::ClientContext &CTX, {{CLI_INPUT_TYPE}} *A_inp) {
        FLOGC(CIF.trace_call || flowc::ns_{{CLI_NODE_NAME/id}}.trace) << std::make_tuple(&CIF, CCid) << "{{CLI_NODE_NAME}} prepare " << flowc::log_abridge(*A_inp) << "\n";
        if(flowc::send_global_ID) {
            CTX.AddMetadata("node-id", flowc::global_node_ID);
            CTX.AddMetadata("start-time", flowc::global_start_time);
        }
        SET_METADATA_{{CLI_NODE_NAME/id}}(CTX)
        GRPC_SENDING("{{CLI_NODE_NAME/id}}", CIF, CCid, flowc::cn_{{CLI_NODE_NAME/id/upper}}, CTX, A_inp)
        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds(flowc::ns_{{CLI_NODE_NAME/id}}.timeout);
        CTX.set_deadline(std::min(deadline, CIF.deadline));
        if(ConP->count() == 0) 
            return nullptr;
        return ConP->stub(ConN, CIF, CCid)->PrepareAsync{{CLI_METHOD_NAME}}(&CTX, *A_inp, &CQ);
    }
    ::grpc::Status {{CLI_NODE_NAME/id}}_call(flowc::call_info const &CIF, int CCid, std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> ConP, {{CLI_OUTPUT_TYPE}} *A_outp, {{CLI_INPUT_TYPE}} *A_inp) {
        ::grpc::ClientContext L_context;
        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds(flowc::ns_{{CLI_NODE_NAME/id}}.timeout);
        L_context.set_deadline(std::min(deadline, CIF.deadline));
        if(flowc::send_global_ID) {
            L_context.AddMetadata("node-id", flowc::global_node_ID);
            L_context.AddMetadata("start-time", flowc::global_start_time);
        }
        SET_METADATA_{{CLI_NODE_NAME/id}}(L_context)
        GRPC_SENDING("{{CLI_NODE_NAME/id}}", CIF, CCid, flowc::cn_{{CLI_NODE_NAME/id/upper}}, CTX, A_inp)
        ::grpc::Status L_status;
        if(ConP->count() == 0) {
            L_status = ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, ::flowc::sfmt() << "No addresses found for {{CLI_NODE_NAME/id}}: " << flowc::ns_{{CLI_NODE_NAME/id}}.endpoint << "\n");
        } else {
            int ConN = -1;
            L_status = ConP->stub(ConN, CIF, CCid)->{{CLI_METHOD_NAME}}(&L_context, *A_inp, A_outp);
            ConP->finished(ConN, CIF, CCid, L_status.error_code() == grpc::StatusCode::UNAVAILABLE);
            GRPC_RECEIVED("{{CLI_NODE_NAME/id}}", CIF, CCid, flowc::cn_{{CLI_NODE_NAME/id/upper}}, L_status, L_context, A_outp)
        }
        FLOGC(CIF.trace_call || flowc::ns_{{CLI_NODE_NAME/id}}.trace) << std::make_tuple(&CIF, CCid) << "{{CLI_NODE_NAME}} request: " << flowc::log_abridge(*A_inp) << "\n";
        if(!L_status.ok()) {
            GRPC_ERROR(CIF, CCid, "{{CLI_NODE_NAME}} ", L_status, L_context);
        } else {
            FLOGC(CIF.trace_call || flowc::ns_{{CLI_NODE_NAME/id}}.trace) << std::make_tuple(&CIF, CCid) << "{{CLI_NODE_NAME}} reply: " << flowc::log_abridge(*A_outp) << "\n";
        }
        return L_status;
    }}I}
    // Constructor
    {{NAME/id}}_service() {
        Call_Counter = 1;
        Active_Calls = 0;
        {I:CLI_NODE_NAME{
        {{CLI_NODE_NAME/id}}_nversion = 0;
        auto {{CLI_NODE_NAME/id}}_cp = new ::flowc::connector<{{CLI_SERVICE_NAME}}>(Active_Calls, flowc::ns_{{CLI_NODE_NAME/id}}, std::map<std::string, std::vector<std::string>>());
        {{CLI_NODE_NAME/id}}_conp.reset({{CLI_NODE_NAME/id}}_cp);
        }I}
    }
{I:ENTRY_NAME{
    // {{ENTRY_SERVICE_NAME}}::{{ENTRY_NAME}}(::grpc::ServerContext *, {{ENTRY_INPUT_TYPE}} const *, {{ENTRY_OUTPUT_TYPE}} *);
{{ENTRY_CODE}}
    ::grpc::Status {{ENTRY_NAME}}(::grpc::ServerContext *context, {{ENTRY_INPUT_TYPE}} const *pinput, {{ENTRY_OUTPUT_TYPE}} *poutput) override {
        Active_Calls.fetch_add(1, std::memory_order_seq_cst);
        flowc::call_info ge_cif("{{ENTRY_NAME}}", Call_Counter.fetch_add(1, std::memory_order_seq_cst), context, flowc::entry_{{ENTRY_NAME}}_timeout);
        auto const time_now = std::chrono::system_clock::now();

        auto s = {{ENTRY_NAME}}(ge_cif, context, pinput, poutput);

        if(ge_cif.time_call) 
            context->AddTrailingMetadata(GFH_CALL_TIMES, ge_cif.get_time_info());
        if(flowc::send_global_ID || ge_cif.trace_call) { 
            context->AddTrailingMetadata(GFH_NODE_ID, flowc::global_node_ID); 
            context->AddTrailingMetadata(GFH_START_TIME, flowc::global_start_time); 
            context->AddTrailingMetadata(GFH_CALL_ID, std::to_string(ge_cif.id)); 
        }
        Active_Calls.fetch_add(-1, std::memory_order_seq_cst);
        return s;
    }
}I}
    
};
namespace flowinfo {
std::map<std::string, char const *> schema_map = {
{I:CLI_NODE_NAME{    
#if !defined(NO_REST) || !(NO_REST)    
    { "/-node-output/{{CLI_NODE_NAME/id/option}}", {{CLI_OUTPUT_SCHEMA_JSON/c}} },
    { "/-node-input/{{CLI_NODE_NAME/id/option}}", {{CLI_INPUT_SCHEMA_JSON/c}} },
#endif
    { "/-node-proto/{{CLI_NODE_NAME/id/option}}", {{CLI_PROTO/c}} }, 
}I}
{I:ENTRY_NAME{    
#if !defined(NO_REST) || !(NO_REST)    
    { "/-output/{{ENTRY_NAME}}", {{ENTRY_OUTPUT_SCHEMA_JSON/c}} }, 
    { "/-input/{{ENTRY_NAME}}", {{ENTRY_INPUT_SCHEMA_JSON/c}} }, 
#endif
    { "/-proto/{{ENTRY_NAME}}", {{ENTRY_PROTO/c}} }, 
}I}
    { "/-proto/", {{ENTRIES_PROTO/c}} },
    { "/-all-proto/", {{ALL_NODES_PROTO/c}} },
};
}
#if !defined(NO_REST) || !(NO_REST)    
namespace rest {
std::string gateway_endpoint;
std::string app_directory("./app");
std::string docs_directory("./docs");
std::string www_directory("./www");

static int log_message(const struct mg_connection *conn, const char *message) {
    FLOG << message << "\n";
	return 1;
}
static void connection_close(const struct mg_connection *) {
    //FLOG << "connection closed\n";
}
static int not_found(struct mg_connection *conn, std::string const &message) {
    std::string j_message = flowc::sfmt() << "{"
        << "\"code\": 404, \"message\":" << flowc::json_string(message) << "}";
       
	mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
              "Content-Type: application/json\r\n"
              "Content-Length: %lu\r\n"
              "\r\n", j_message.length());
	mg_printf(conn, "%s", j_message.c_str());
    return 404;
}
static int text_reply(struct mg_connection *conn, char const *content, size_t length=0, char const *xtra_headers="Content-Type: text/plain\r\n") {
    if(xtra_headers == nullptr) xtra_headers = "";
	mg_printf(conn, "HTTP/1.1 200 OK\r\n"
              "%s"
              "Content-Length: %lu\r\n"
              "\r\n", xtra_headers, length == 0? strlen(content): length);
	mg_printf(conn, "%s", content);
	return 200;
}
static int json_reply(struct mg_connection *conn, int code, char const *msg, char const *content, size_t length=0, char const *xtra_headers=nullptr) {
    if(xtra_headers == nullptr) xtra_headers = "";
	mg_printf(conn, "HTTP/1.1 %d %s\r\n"
              "Content-Type: application/json\r\n"
              "%s"
              "Content-Length: %lu\r\n"
              "\r\n", code, msg, xtra_headers, length == 0? strlen(content): length);
	mg_printf(conn, "%s", content);
	return code;
}
static int json_reply(struct mg_connection *conn, char const *content, size_t length=0, char const *xtra_headers=nullptr) {
    return json_reply(conn, 200, "OK", content, length, xtra_headers);
}
static int protobuf_reply(struct mg_connection *conn, google::protobuf::Message const &message, std::string const &xtra_headers="") {
    std::string data;
    message.SerializeToString(&data);
	mg_printf(conn, "HTTP/1.1 200 OK\r\n"
              "Content-Type: application/x-protobuf\r\n"
              "%s"
              "Content-Length: %lu\r\n"
              "\r\n", xtra_headers.c_str(), data.length());
    mg_write(conn, data.data(), data.length());
    return 200;
}
static int message_reply(struct mg_connection *conn, google::protobuf::Message const &message, std::string const &xtra_headers="") {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = false;
    options.always_print_primitive_fields = false;
    options.preserve_proto_field_names = false;
    std::string json_message;
    google::protobuf::util::MessageToJsonString(message, &json_message, options);
    return json_reply(conn, json_message.c_str(), json_message.length(), xtra_headers.c_str());
}
static int grpc_error(struct mg_connection *conn, ::grpc::ClientContext const &context, ::grpc::Status const &status, std::string const &xtra_headers="") {
    std::string errm = flowc::sfmt() 
        << "{" 
        << "\"code\": 500,"
        << "\"from\":" << flowc::json_string(context.peer()) << ","
        << "\"grpc-code\":" << status.error_code() << ","
        << "\"message\":" << flowc::json_string(status.error_message())
        << "}";
    int code = 500;
    switch(status.error_code()) {
        case grpc::StatusCode::UNAVAILABLE:
            code = 503;
            break;
        case grpc::StatusCode::DEADLINE_EXCEEDED:
            code = 408;
            break;
        default:
            break;
    }
    return json_reply(conn, code, "gRPC Error", errm.c_str(), errm.length(), xtra_headers.c_str());
}
static int conversion_error(struct mg_connection *conn, google::protobuf::util::Status const &status) {
    std::string error_message = flowc::sfmt() << "{"
        << "\"code\": 400,"
        << "\"message\": \"Input failed conversion to protobuf\","
        << "\"description\":" << flowc::json_string(status.ToString())
        << "}";
    return json_reply(conn, 400, "Bad Request", error_message.c_str(), error_message.length());
}
static int bad_request_error(struct mg_connection *conn) {
    std::string error_message = flowc::sfmt() << "{"
        << "\"code\": 422,"
        << "\"message\": \"Unprocessable Entity\""
        << "}";
    return json_reply(conn, 422, "Unprocessable Entity", error_message.c_str(), error_message.length());
}
static int get_info(struct mg_connection *conn, void *cbdata) {
    std::string info = flowc::sfmt() << "{"
        << "\"request-schema\": " << flowinfo::schema_map.find("/-input/{{ENTRY_NAME}}")->second  << ","
        << "\"methods\": [" 
            "{"
                "\"advanced\":false,"
                "\"name\": \"{{MAIN_ENTRY_NAME}}\","
                "\"url\": \"{{MAIN_ENTRY_URL}}\","
                "\"response-schema\": " << flowinfo::schema_map.find("/-output/{{MAIN_ENTRY_NAME}}")->second  
        <<   "}"
{I:ALT_ENTRY_NAME{            << ",{"
                "\"advanced\": true,"
                "\"timeout\": " << flowc::entry_{{ALT_ENTRY_NAME}}_timeout << ","
                "\"name\": \"{{ALT_ENTRY_NAME}}\","
                "\"url\": \"{{ALT_ENTRY_URL}}\","
                "\"response-schema\": " << flowinfo::schema_map.find("/-output/{{ALT_ENTRY_NAME}}")->second
        <<    "}"
}I}
        << "],"
        << "\"nodes\": ["
{C:-CLI_NODE_NAME-1{
        << "{"
                "\"timeout\": " << flowc::ns_{{CLI_NODE_NAME/id}}.timeout << ","
                "\"request-schema\": " << flowinfo::schema_map.find("/-node-input/{{CLI_NODE_NAME/id/option}}")->second << ","
                "\"response-schema\": " << flowinfo::schema_map.find("/-node-output/{{CLI_NODE_NAME/id/option}}")->second << ","
                "\"name\": \"{{CLI_NODE_NAME/lower/id/option}}\","
                "\"url\": \"/-node/{{CLI_NODE_NAME/id/option}}\""
            "}"
}C}
{C:CLI_NODE_NAME+2{
        << ",{"
                "\"timeout\": " << flowc::ns_{{CLI_NODE_NAME/id}}.timeout << ","
                "\"request-schema\": " << flowinfo::schema_map.find("/-node-input/{{CLI_NODE_NAME/id/option}}")->second << ","
                "\"response-schema\": " << flowinfo::schema_map.find("/-node-output/{{CLI_NODE_NAME/id/option}}")->second << ","
                "\"name\": \"{{CLI_NODE_NAME/lower/id/option}}\","
                "\"url\": \"/-node/{{CLI_NODE_NAME/id/option}}\""
            "}"
}C}
        "]"
    "}";
    return json_reply(conn, info.c_str(), info.length());
}
static int get_schema(struct mg_connection *conn, void*) {
	char const *local_uri = mg_get_request_info(conn)->local_uri;
    auto sp = flowinfo::schema_map.find(local_uri);
    if(sp == flowinfo::schema_map.end()) 
        return not_found(conn, "Name not recognized");
    return json_reply(conn, sp->second);
}
static int get_proto(struct mg_connection *conn, void*) {
	char const *local_uri = mg_get_request_info(conn)->local_uri;
    auto sp = flowinfo::schema_map.find(local_uri);
    if(sp == flowinfo::schema_map.end()) 
        return not_found(conn, "Name not recognized");
    return text_reply(conn, sp->second);
}
struct form_data_t {
    std::multimap<std::string, std::string> form;
    std::string key;
    std::string value;
};
static int field_get(const char *key, const char *value, size_t valuelen, void *user_data) {
    auto &fd = *(form_data_t *) user_data;
    if(key == nullptr || *key == '\0') {
        fd.value.append(value, valuelen);
    } else { 
        if(!fd.key.empty()) 
            fd.form.emplace(fd.key, fd.value);
        fd.key = key; 
        fd.value = std::string(value, valuelen);
    }
	return 0;
}
int field_stored(const char *path, long long file_size, void *user_data) {
	//mg_printf(conn, "stored as %s (%lu bytes)\r\n\r\n", path, (unsigned long)file_size);
	return 0;
}
static int field_found(const char *key, const char *filename, char *path, size_t pathlen, void *user_data) {
	if(filename && *filename) {
		snprintf(path, pathlen, "/dev/null");
        return MG_FORM_FIELD_STORAGE_SKIP;
	}
	return MG_FORM_FIELD_STORAGE_GET;
}
static int get_form_data(struct mg_connection *conn, std::string &data) {
    char const *content_type = mg_get_header(conn, "Content-Type");
    /** Default content type is application/json
     */
    if(content_type == nullptr || strncasecmp(content_type, "application/json", strlen("application/json")) == 0) {
        // The expected content type is application/json
        std::vector<char> buffer(65536);
        unsigned long read = 0;
	    int r = mg_read(conn, &buffer[read], buffer.size() - read);
	    while(r > 0) {
		    read += r;
            if(buffer.size() >= MAX_REST_REQUEST_SIZE) {
	            char const *local_uri = mg_get_request_info(conn)->local_uri;
                FLOG << "error: " << local_uri << ": Read exceeded buffer size of " << MAX_REST_REQUEST_SIZE << " bytes\n";
                return -1;
            }
            if(read == buffer.size()) 
                buffer.resize(read*2);
	        r = mg_read(conn, &buffer[read], buffer.size() - read);
	    }
        data = std::string(buffer.begin(), buffer.begin() + read);
        return 1;    
    }

    /** Otherwise attempt to build json out of form fields
     */

    form_data_t fd;
	struct mg_form_data_handler fdh = {field_found, field_get, field_stored, (void *) &fd};
	int ret = mg_handle_form_request(conn, &fdh);
    if(ret <= 0) return ret;
    fd.form.emplace(fd.key, fd.value);

    data += "{"; 
    int c = 0;
    for(auto const &nv: fd.form) {
        if(++c > 1) data += ",";
        if(nv.first.length() > 2 && nv.first.substr(nv.first.length()-2) == "[]")
            data += flowc::json_string(nv.first.substr(0, nv.first.length()-2));
        else 
            data += flowc::json_string(nv.first);
        data += ":";
        data += flowc::json_string(nv.second);
    }
    data += "}";
    return ret;
}
static int file_handler(struct mg_connection *conn, void *cbdata) {
    std::string const &dir = *(std::string const *) cbdata;
	char const *local_uri = mg_get_request_info(conn)->local_uri;
    char const *common = strchr(local_uri+1, '/');
    if(common != nullptr && *(common+1) != '\0') {
        std::string filename = dir + common;
        struct stat buffer;   
        if(stat(filename.c_str(), &buffer) == 0) {
            FLOGC(flowc::trace_calls) << "sending " << common+1 << " from " << dir << "\n";
            mg_send_file(conn, filename.c_str());
            return 200;
        }
    } else if(strcmp(local_uri, "/-docs") == 0) {
        FLOGC(flowc::trace_calls) << "list -docs contents\n";
    }
    FLOG << "get \"" << local_uri << "\" not found...\n";
    return not_found(conn, "File not found");
}
static int root_handler(struct mg_connection *conn, void *cbdata) {
    bool rest_only = (bool) cbdata;
    char const *local_uri = mg_get_request_info(conn)->local_uri;
    if(rest_only || strcmp(local_uri, "/") != 0) 
        return not_found(conn, flowc::sfmt() << "Resource not found");
    
    // Look first in the app directory 
    struct stat buffer;   
    std::string name;
    name = app_directory + "/index.html";
    if(stat(name.c_str(), &buffer) == 0) 
        return mg_send_http_redirect(conn, "/-app/index.html", 307); 

    // Then in the UI directory 
    name = www_directory + "/index.html";
    if(stat(name.c_str(), &buffer) == 0) 
        return mg_send_http_redirect(conn, "/-www/index.html", 307); 
    return not_found(conn, "Resource not found");
}
std::atomic<long> call_counter;
}
namespace rest_api {
{I:ENTRY_NAME{
static int C_{{ENTRY_NAME}}(flowc::call_info const &cif, struct mg_connection *A_conn, std::string const &A_inp_json) {
    std::string xtra_headers;

    std::shared_ptr<::grpc::Channel> L_channel(::grpc::CreateChannel(rest::gateway_endpoint, ::grpc::InsecureChannelCredentials()));
    std::unique_ptr<{{ENTRY_SERVICE_NAME}}::Stub> L_client_stub = {{ENTRY_SERVICE_NAME}}::NewStub(L_channel);                    
    {{ENTRY_OUTPUT_TYPE}} L_outp; 
    {{ENTRY_INPUT_TYPE}} L_inp;

    FLOGC(cif.trace_call) << cif << "body: " << flowc::log_abridge(A_inp_json) << "\n";

    auto L_conv_status = google::protobuf::util::JsonStringToMessage(A_inp_json, &L_inp);
    if(!L_conv_status.ok()) return rest::conversion_error(A_conn, L_conv_status);

    ::grpc::ClientContext L_context;

    if(cif.have_deadline) L_context.set_deadline(cif.deadline);
    L_context.AddMetadata(GFH_OVERLAPPED_CALLS, cif.async_calls? "1": "0");
    L_context.AddMetadata(GFH_TIME_CALL, cif.time_call? "1": "0");
    if(cif.trace_call)
        L_context.AddMetadata(GFH_TRACE_CALL, "1");
    if(!cif.id_str.empty())
        L_context.AddMetadata(GFH_CALL_ID, cif.id_str);

#if defined(REST_CHECK_{{ENTRY_NAME/id/upper}}_BEFORE) || defined(REST_CHECK_{{ENTRY_NAME/id/upper}}_AFTER)
    char const *check_header = mg_get_header(A_conn, RFH_CHECK);
#endif
    
#ifdef REST_CHECK_{{ENTRY_NAME/id/upper}}_BEFORE
    {
        int http_code; std::string http_message, http_body; 
        if(!REST_CHECK_{{ENTRY_NAME/id/upper}}_BEFORE(http_code, http_message, http_body, check_header, L_context, &L_inp, xtra_headers)) {
            if(http_body.empty()) http_body = flowc::sfmt() << "{"
                << "\"code\": " << http_code << ","
                << "\"message\": " << flowc::json_string(http_message) << "}";
            return rest::json_reply(A_conn, http_code, http_message.c_str(), http_body.c_str(), http_body.length(), xtra_headers.c_str());
        }
    }
#endif
    //::grpc::Status L_status = L_client_stub->{{ENTRY_NAME}}(&L_context, L_inp, &L_outp);
    ::grpc::Status L_status;
    ::grpc::CompletionQueue q1;
    char const *tag; bool next_ok = false; 
    auto carr = L_client_stub->PrepareAsync{{ENTRY_NAME}}(&L_context, L_inp, &q1);
    carr->StartCall();
    carr->Finish(&L_outp, &L_status, (void *) "REST-{{ENTRY_NAME}}");
    for(;;) {
        auto ns1 = q1.AsyncNext((void **) &tag, &next_ok, std::chrono::system_clock::now() + std::chrono::milliseconds(REST_CONNECTION_CHECK_INTERVAL));
        if(ns1 == ::grpc::CompletionQueue::NextStatus::GOT_EVENT && next_ok) 
            break;
        if(ns1 != ::grpc::CompletionQueue::NextStatus::TIMEOUT) {
            L_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Invalid internal state");
            break;
        }
        // Check if the connection is still valid
        bool is_valid = true;
        if(!is_valid || (cif.have_deadline && std::chrono::system_clock::now() > cif.deadline)) {
            L_status = ::grpc::Status(::grpc::StatusCode::CANCELLED, "Call exceeded deadline or was cancelled by the client"); 
            break;
        }
    }
    flowc::closeq(q1);

    for(auto const &mde: L_context.GetServerTrailingMetadata()) {
        std::string header(mde.first.data(), mde.first.length());
        if(header == GFH_CALL_TIMES) 
            header = RFH_CALL_TIMES;
        else 
            header = std::string("X-Flow-") + header;
        xtra_headers += header;
        xtra_headers += ": ";
        xtra_headers += std::string(mde.second.data(), mde.second.length());
        xtra_headers += "\r\n";
    }
#ifdef REST_CHECK_{{ENTRY_NAME/id/upper}}_AFTER
    {
        int http_code; std::string http_message, http_body; 
        if(!REST_CHECK_{{ENTRY_NAME/id/upper}}_AFTER(http_code, http_message, http_body, check_header, L_context, &L_inp, L_status, &L_outp, xtra_headers)) {
            if(http_body.empty()) http_body = flowc::sfmt() << "{"
                << "\"code\": " << http_code << ","
                << "\"message\": " << flowc::json_string(http_message) << "}";
            return rest::json_reply(A_conn, http_code, http_message.c_str(), http_body.c_str(), http_body.length(), xtra_headers.c_str());
        }
    }
#endif
    if(!L_status.ok()) return rest::grpc_error(A_conn, L_context, L_status, xtra_headers);
    return cif.return_protobuf? rest::protobuf_reply(A_conn, L_outp, xtra_headers): rest::message_reply(A_conn, L_outp, xtra_headers);
}
static int A_{{ENTRY_NAME}}(struct mg_connection *A_conn, void *A_cbdata) {
    flowc::call_info cif("{{ENTRY_NAME}}", rest::call_counter.fetch_add(1, std::memory_order_seq_cst), A_conn, flowc::entry_{{ENTRY_NAME}}_timeout);
    std::string input_json;
    int rc = rest::get_form_data(A_conn, input_json);

    FLOG << cif << "REST-entry: " << mg_get_request_info(A_conn)->local_uri 
        << " async, time, trace, request-length, response-type: " << cif.async_calls << ", "  << cif.time_call << ", " << cif.trace_call << ", " << input_json.length() << ", " << (cif.return_protobuf? "protobuf": "json") << "\n";

    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0) 
        rc = rest::not_found(A_conn, "Resource not found");
    else if(rc <= 0) 
        rc = rest::bad_request_error(A_conn);
    else 
        rc = C_{{ENTRY_NAME}}(cif, A_conn, input_json);

    FLOG << cif << "REST-return: " << rc << " \n";
    return rc;
}
}I}
{I:CLI_NODE_NAME{
static int NC_{{CLI_NODE_NAME/id}}(flowc::call_info const &cif, struct mg_connection *A_conn, std::string const &A_inp_json) {
    std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> connector = flowc::{{NAME/id}}_service_ptr->{{CLI_NODE_NAME/id}}_get_connector();

    {{CLI_OUTPUT_TYPE}} L_outp; 
    {{CLI_INPUT_TYPE}} L_inp;

    FLOGC(cif.trace_call) << cif << "body: " << flowc::log_abridge(A_inp_json) << "\n";

    auto L_conv_status = google::protobuf::util::JsonStringToMessage(A_inp_json, &L_inp);
    if(!L_conv_status.ok()) return rest::conversion_error(A_conn, L_conv_status);

    ::grpc::ClientContext L_context;
    if(cif.have_deadline) L_context.set_deadline(cif.deadline);
    SET_METADATA_{{CLI_NODE_NAME/id}}(L_context)

    int connection_n = -1;
    //::grpc::Status L_status = connector->stub(connection_n, cif, -1)->{{CLI_METHOD_NAME}}(&L_context, L_inp, &L_outp);
    ::grpc::Status L_status;
    ::grpc::CompletionQueue q1;
    char const *tag; bool next_ok = false; 
    auto carr = connector->stub(connection_n, cif, -1)->PrepareAsync{{CLI_METHOD_NAME}}(&L_context, L_inp, &q1);
    carr->StartCall();
    carr->Finish(&L_outp, &L_status, (void *) "REST-{{ENTRY_NAME}}");
    for(;;) {
        auto ns1 = q1.AsyncNext((void **) &tag, &next_ok, std::chrono::system_clock::now() + std::chrono::milliseconds(REST_CONNECTION_CHECK_INTERVAL));
        if(ns1 == ::grpc::CompletionQueue::NextStatus::GOT_EVENT && next_ok) 
            break;
        if(ns1 != ::grpc::CompletionQueue::NextStatus::TIMEOUT) {
            L_status = ::grpc::Status(::grpc::StatusCode::UNKNOWN, "Invalid internal state");
            break;
        }
        // Check if the connection is still valid
        bool is_valid = true;
        if(!is_valid || (cif.have_deadline && std::chrono::system_clock::now() > cif.deadline)) {
            L_status = ::grpc::Status(::grpc::StatusCode::CANCELLED, "Call exceeded deadline or was cancelled by the client"); 
            break;
        }
    }
    flowc::closeq(q1);

    connector->finished(connection_n, cif, -1, L_status.error_code() == grpc::StatusCode::UNAVAILABLE);

    if(!L_status.ok()) return rest::grpc_error(A_conn, L_context, L_status);
    auto const &metadata = L_context.GetServerTrailingMetadata();
    std::string xtra_headers;
    for(auto const &mde: L_context.GetServerTrailingMetadata()) {
        std::string header(mde.first.data(), mde.first.length());
        if(header == GFH_CALL_TIMES) 
            header = RFH_CALL_TIMES;
        else 
            header = std::string("X-Flow-") + header;
        xtra_headers += header;
        xtra_headers += ": ";
        xtra_headers += std::string(mde.second.data(), mde.second.length());
        xtra_headers += "\r\n";
    }
    return cif.return_protobuf? rest::protobuf_reply(A_conn, L_outp, xtra_headers): rest::message_reply(A_conn, L_outp, xtra_headers);
}
static int N_{{CLI_NODE_NAME/id}}(struct mg_connection *A_conn, void *A_cbdata) {
    flowc::call_info cif("node-{{CLI_NODE_NAME/id/option}}", rest::call_counter.fetch_add(1, std::memory_order_seq_cst), A_conn, flowc::ns_{{CLI_NODE_NAME/id}}.timeout);
    std::string input_json;
    int rc = rest::get_form_data(A_conn, input_json);

    FLOG << cif << "REST-node-entry: " << mg_get_request_info(A_conn)->local_uri
        << " trace, request-length, response-type: " << cif.trace_call << ", " << input_json.length() << ", " << (cif.return_protobuf? "protobuf": "json") << "\n";

    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0) 
        rc = rest::not_found(A_conn, "Resource not found");
    else if(rc <= 0) 
        rc = rest::bad_request_error(A_conn);
    else 
        rc = NC_{{CLI_NODE_NAME/id}}(cif, A_conn, input_json);
    FLOG << cif << "REST-node-return: " << rc << "\n";
    return rc;
}
}I}
}
namespace rest {
struct mg_context *ctx = nullptr;
struct mg_callbacks callbacks;

int stop_civetweb() {
    if(ctx) {
        mg_stop(ctx);
        std::cout << "REST server exited" << std::endl;
        ctx = nullptr;
    }
    return 0;
}
int start_civetweb(std::vector<std::string> &cfg, bool rest_only) {
    std::vector<const char *> optbuf;
    call_counter = 1;
    long request_timeout_ms = 0;
{I:ENTRY_NAME{    request_timeout_ms = std::max(flowc::entry_{{ENTRY_NAME}}_timeout, request_timeout_ms);
}I}
    if(!rest_only) {
{I:CLI_NODE_NAME{        request_timeout_ms = std::max(request_timeout_ms, flowc::ns_{{CLI_NODE_NAME/id}}.timeout);
}I}
    }
    if(flowc::get_cfg(cfg, "rest_request_timeout_ms") == nullptr) {
        cfg.push_back("rest_request_timeout_ms");
        cfg.push_back(std::to_string(request_timeout_ms + request_timeout_ms / 20));
    } else {
        long rto = flowc::strtolong(flowc::get_cfg(cfg, "rest_request_timeout_ms"), 0);
        if(rto < request_timeout_ms) FLOG << "rest request timeout is set to " << rto << ", less than " << request_timeout_ms << "\n";
    }
    std::array<const char *, 10> default_opts = {
        "rest_document_root", "/dev/null",
        "rest_error_log_file", "error.log",
        "rest_extra_mime_types", ".flow=text/plain,.proto=text/plain,.svg=image/svg+xml",
        "rest_enable_auth_domain_check", "no",
        "rest_max_request_size", "65536"
    };
    for(unsigned i = 0; i < default_opts.size(); i += 2) 
        if(flowc::get_cfg(cfg, default_opts[i]) == nullptr) {
            cfg.push_back(default_opts[i]);
            cfg.push_back(default_opts[i+1]);
        }
    for(unsigned i = 0; i < cfg.size(); i += 2) {
        char const *n = cfg[i].c_str();
        if(strncmp(n, "rest_", strlen("rest_")) == 0) {
            if(mg_get_option(nullptr, n+strlen("rest_")) == nullptr) {
                FLOG << "ignoring invalid rest option: " << cfg[i] << ": " << cfg[i+1] << "\n";
            } else {
                optbuf.push_back(n+strlen("rest_"));
                optbuf.push_back(cfg[i+1].c_str());
            }
        }
    }
    optbuf.push_back(nullptr);

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.log_message = rest::log_message;
	ctx = mg_start(&callbacks, 0, &optbuf[0]);

	if(ctx == nullptr) 
        return 1;
{I:ENTRY_NAME{    mg_set_request_handler(ctx, "/{{ENTRY_NAME}}", rest_api::A_{{ENTRY_NAME}}, (void *) "/{{ENTRY_NAME}}");
}I}
	mg_set_request_handler(ctx, "/", root_handler, 0);
    if(!rest_only) {
	    mg_set_request_handler(ctx, "/-input", get_schema, 0);
	    mg_set_request_handler(ctx, "/-output", get_schema, 0);
	    mg_set_request_handler(ctx, "/-proto", get_proto, 0);
	    mg_set_request_handler(ctx, "/-node-input", get_schema, 0);
	    mg_set_request_handler(ctx, "/-node-output", get_schema, 0);
	    mg_set_request_handler(ctx, "/-node-proto", get_proto, 0);
	    mg_set_request_handler(ctx, "/-info", get_info, 0);
{I:CLI_NODE_NAME{        mg_set_request_handler(ctx, "/-node/{{CLI_NODE_NAME/id/option}}", rest_api::N_{{CLI_NODE_NAME/id}}, (void *) "/-node/{{CLI_NODE_NAME/id/option}}");
}I}
	    mg_set_request_handler(ctx, "/-docs", file_handler, (void *) &docs_directory);
	    mg_set_request_handler(ctx, "/-app", file_handler, (void *) &app_directory);
	    mg_set_request_handler(ctx, "/-www", file_handler, (void *) &www_directory);
    }

	// List all listening ports 
	struct mg_server_ports ports[32];
	int port_cnt, n;
	memset(ports, 0, sizeof(ports));
	port_cnt = mg_get_server_ports(ctx, 32, ports);
    std::cout <<  "REST gateway available at";
	for(n = 0; n < port_cnt && n < 32; n++) {
		const char *proto = ports[n].is_ssl ? "https" : "http";
		const char *host;
		if((ports[n].protocol & 1) == 1) {
			// IPv4 
			host = "127.0.0.1";
		}
		if((ports[n].protocol & 2) == 2) {
			// IPv6 
			host = "[::1]";
        }
        std::cout << " " << proto << "://" << host << ":" << ports[n].port;
	}
    std::cout << "\n";
    std::cout << "webapp enabled: " << (rest_only? "no": "yes") << "\n";
    std::cout << "\n";
    for(unsigned i = 0; i+1 < optbuf.size(); i += 2) {
        if(strchr(optbuf[i+1], ' ') == nullptr) 
            std::cout << optbuf[i] << ": " << optbuf[i+1] << "\n";
        else
            std::cout << optbuf[i] << ": \"" << optbuf[i+1] << "\"\n";
    }
    std::cout << "\n";
    return 0;
}
}
#endif
namespace flowc {
static bool read_keycert(std::string &kc, std::string const &fn, std::string const &substr) {
    int tc = 0;
    std::string line;
    std::ifstream pemf(fn.c_str());
    bool acc = false;
    while(std::getline(pemf, line)) {
        if(acc) {
            kc += line;
            kc += "\n";
        }
        if(line.find(substr) != std::string::npos) {
            ++tc;
            acc = !acc;
            if(acc) {
                kc += line;
                kc += "\n";
            }
        }
    }
    return tc != 0 && tc % 2 == 0;
}
bool node_cfg::read_from_cfg(std::vector<std::string> const &cfg) {
    trace = strtobool(get_cfg(cfg, std::string("node_") + id + "_trace"), trace);
    maxcc = (int) strtolong(get_cfg(cfg, std::string("node_") + id + "_maxcc"), maxcc);
    timeout = strtolong(get_cfg(cfg, std::string("node_") + id + "_timeout"), timeout);
    char const *cert_fn = get_cfg(cfg, std::string("node_") + id + "_certificate");
    if(cert_fn != nullptr) cert_filename = cert_fn;
    bool cert_ok = true;
    if(!cert_filename.empty()) {
        grpc::SslCredentialsOptions ssl_opts;
        if(!read_keycert(ssl_opts.pem_root_certs, cert_filename, " CERTIFICATE-")) {
            cert_ok = false;
            std::cerr << "Failed to read certificate chain from " << cert_filename << " for node [" << id << "]\n";
        } else {
            creds = grpc::SslCredentials(ssl_opts);
        }
    } else {
        creds = grpc::InsecureChannelCredentials();
    }

    char const *ep = get_cfg(cfg, std::string("node_") + id + "_endpoint");
    if(ep != nullptr) endpoint = ep;
    bool ep_ok = !endpoint.empty() &&
        casd::parse_endpoint_list(dnames, dendpoints, fendpoints, endpoint);
    if(!ep_ok) 
        std::cerr << "No endpoint found for node [" << id << "]\n";
    return ep_ok && cert_ok;
}
bool node_cfg::check_option(std::string const &opt) const {
    static std::set<std::string> valid_options = { "_trace", "_maxcc", "_timeout", "_endpoint", "_certificate" };
    for(auto const &suff: valid_options)
        if(opt == std::string("node_") + id + suff)
            return true;
    return false;
}
static bool check_node_option(std::string const &opt) {
    return false 
{I:CLI_NODE_NAME{    || ns_{{CLI_NODE_NAME/lower/id}}.check_option(opt)
}I}
    ;    
}
static bool check_entry_option(std::string const &opt) {
    static std::set<std::string> valid_options = { "_timeout" };
    for(auto const &suff: valid_options) {
{I:ENTRY_NAME{
        if(opt == std::string("entry_{{ENTRY_NAME}}") + suff)
            return true;
}I}
    }
    return false;
}
static std::set<std::string> valid_options = {"grpc_listening_ports", "webapp_directory", "enable_webapp", "async_calls", 
    "trace_calls", "server_id", "send_id", "cares_refresh", "grpc_num_threads", 
    "trace_connections", "accumulate_addresses", "ssl_certificate", "grpc_ssl_certificate",
{I:DEFN{"fd_{{DEFN}}", }I}
};
static bool check_option(std::string const &opt, bool print_message, std::string const &location="") {
    if(opt.substr(0, strlen("rest_")) == "rest_") {
#if !defined(NO_REST) || !(NO_REST)    
        if(mg_get_option(nullptr, opt.c_str()+strlen("rest_")) == nullptr) {
            if(print_message) 
                std::cerr << location << "Invalid civetweb option: '" << opt.c_str()+strlen("rest_") << "'\n";
            return false;
        }
#endif        
    } else if(opt.substr(0, strlen("node_")) == "node_") {
        if(!flowc::check_node_option(opt)) {
            if(print_message)
                std::cerr << location << "Invalid node option: '" << opt << "'\n";
            return false;
        }
    } else if(opt.substr(0, strlen("entry_")) == "entry_") {
        if(!flowc::check_entry_option(opt)) {
            if(print_message)
                std::cerr << location << "Invalid entry option: '" << opt << "'\n";
            return false;
        }
    } else if(valid_options.find(opt) == valid_options.end()) {
        if(print_message)
            std::cerr << location << "Invalid option: '" << opt << "'\n";
        return false;
    }
    return true;
}
static unsigned read_cfg(std::vector<std::string> &cfg, std::string const &filename, std::string const &env_prefix) {
    std::ifstream cfgs(filename.c_str());
    std::string line, line_buffer;
    unsigned long line_number = 0, lc = 0;
    int error_count = 0;

    do {
        line_buffer.clear(); line_number = lc+1;
        while(std::getline(cfgs, line)) {
            ++lc;
            auto b = line.find_first_not_of("\t\r\a\b\v\f ");
            if(b == std::string::npos || line[b] == '#') 
                line = "";
            if(line.length() > 0 && line.back() == '\\') {
                line.pop_back();
                line_buffer += line;
                continue;
            }
            line_buffer += line;
            if(!line_buffer.empty()) 
                break;
        }
       
        auto b = line_buffer.find_first_not_of("\t\r\a\b\v\f ");
        if(b == std::string::npos) 
            break;
        line_buffer = line_buffer.substr(b);
        b = line_buffer.find_first_of("\t\r\a\b\v\f =:");
        std::string name = line_buffer.substr(0, b);
        std::string value;
        if(b != std::string::npos) 
            value = line_buffer.substr(b+1);

        unsigned vb = 0;
        for(unsigned ve = value.length(), sepc = 0; vb < ve && sepc <= 1 && strchr("\t\r\a\b\v\f =:", value[vb]) != nullptr; ++vb) 
            if(value[vb] == ':' || value[vb] == '=') {
                if(sepc < 1) 
                    ++sepc;
                else 
                    break;
            }
        value = value.substr(vb); 

        b = value.find_last_not_of("\t\r\a\b\v\f ");
        if(b != std::string::npos) 
            value = value.substr(0, b+1);
        if(value.length() >= 2 && value.front() == '"' && value.back() == '"') 
            value = value.substr(1, value.length()-2);
        if(!check_option(name, true)) {
            ++error_count;
            continue;
        }
        cfg.push_back(name);
        cfg.push_back(value);
    } while(!line_buffer.empty());

    if(!env_prefix.empty()) {
        for(auto envp = environ; *envp != nullptr; ++envp) {
            if(strncasecmp(*envp, env_prefix.c_str(), env_prefix.length()) != 0) 
                continue;
            auto vp = *envp + env_prefix.length();
            if(*vp == '\0' || *vp == '=') 
                continue;
            auto vvp = strchr(vp, '=');
            if(vvp == nullptr) 
                continue;
            std::string opname = std::string(vp, vvp);
            std::transform(opname.begin(), opname.end(), opname.begin(), ::tolower);
            if(!check_option(opname, false, sfmt() << filename << "(" << line_number << "): "))
                continue;
            cfg.push_back(opname);
            cfg.push_back(vvp+1);
        }
    }

    return error_count;
}
inline static 
bool endsw(char const *str, char const *end) {
    return strlen(str) >= strlen(end) && strcmp(str + strlen(str) - strlen(end), end) == 0;
}
inline static 
bool startsw(char const *str, char const *start) {
    return strncmp(str, start, strlen(start)) == 0;
}
// returns: 0 OK, 1 error, 2 show help, 3 show config, 4 show info, 5 show version
static int parse_args(int argc, char *argv[], std::vector<std::string> &cfg, int min_args=1) {
    if(argc == 1)
        return 2;
    int uac = 0, a = 1, rc = 0;
    while(a < argc) {
        if(strcmp(argv[a], "--help") == 0) {
            rc = 2;
            ++a;
        } else if(strcmp(argv[a], "--version") == 0) {
            rc = 5;
            ++a;
        } else if(strcmp(argv[a], "--cfg") == 0) {
            rc = 3;
            ++a;
        } else if((startsw(argv[a], "--node") || startsw(argv[a], "--entry")) && endsw(argv[1], "-proto")) {
            rc = 4;
            ++a;
#if !defined(NO_REST)
        } else if((startsw(argv[a], "--node") || startsw(argv[a], "--entry")) && (endsw(argv[1], "-input") || endsw(argv[1], "-output"))) {
            rc = 4;
            ++a;
#endif
        } else if(strncmp(argv[a], "--", 2) == 0) {
            if(a+1 == argc)  {
                std::cerr << "Missing value for option: '" << argv[a] << "'\n";
                return 1;
            }
            cfg.push_back(argv[a]+2);
            for(auto cp = cfg.back().begin(), ce = cfg.back().end(); cp != ce; ++cp)
                if(*cp == '-') *cp = '_';
            if(!check_option(cfg.back(), true))
                return 1;

            cfg.push_back(argv[a+1]);
            a += 2;
        } else if(uac < 3) {
            switch(++uac) {
                case 1:
                    cfg.push_back("grpc_listening_ports");
                    break;
#if !defined(NO_REST)
                case 2:
                    cfg.push_back("rest_listening_ports");
                    break;
                case 3:
                    cfg.push_back("webapp_directory");
                    break;
#endif
                default:
                    return 1;
            }
            cfg.push_back(argv[a]);
            ++a; 
        } else {
            std::cerr << "Invalid argument: " << argv[a] << "\n";
            return 1;
        }
    }
    if(rc == 0 && uac < min_args) {
        std::cerr << "Invalid number of args, " << min_args << " expected\n";
        rc = 1;
    }
    return rc;
}
static int parse_listening_port_list(std::set<std::string> &list, std::string const &slist) {
    std::set<std::string> dnames, dendpoints;
    return casd::parse_endpoint_list(dnames, dendpoints, list, slist)? 0: 1;
}
static void print_banner(std::ostream &out) {
#if !defined(NO_REST) || !(NO_REST)    
    out << "{{NAME}} gRPC/REST server" << std::endl;
#else
    out << "{{NAME}} gRPC server" << std::endl;
#endif
    out 
        << "{{MAIN_FILE_SHORT}} ({{MAIN_FILE_TS}})\n" 
        << "{{FLOWC_NAME}} {{FLOWC_VERSION}} ({{FLOWC_BUILD}})\n"
        <<  "grpc " << grpc::Version() 
#if !defined(NO_REST) || !(NO_REST)    
        << ", civetweb " << mg_version() 
#endif        
        << ", c-ares " << ares_version(nullptr) << "\n"
#if defined(__clang__)          
        << "clang++ " << __clang_version__ << " (" << __cplusplus <<  ")\n"
#elif defined(__GNUC__) 
        << "g++ " << __VERSION__ << " (" << __cplusplus << ")\n"
#else
#endif
        << std::flush;
}
static std::ostream &print_cfg(std::ostream &out, std::vector<std::string> const &cfg) {
    for(size_t c = 0, s = cfg.size(); c < s; c += 2) 
        out << cfg[c] << "=" << cfg[c+1] << "\n";
    return out;
}
static std::unique_ptr<grpc::Server> grpc_server_ptr;
static void signal_shutdown_handler(int sig) {
    std::cerr << "Signal " << sig << " received";
    if(grpc_server_ptr) {
        std::cout << ", shutting down" << std::endl;
        grpc_server_ptr->Shutdown();
    } else {
        std::cout << std::endl;
    }
}
struct ansiesc_out {
    bool use_escapes;
    std::ostream &out;
    int c1_count = 0, c2_count = 0;
    char const *c1_begin =  "\x1b[38;5;4m", *c1_end = "\x1b[0m";
    char const *c2_begin = "\x1b[1m", *c2_end = "\x1b[0m";
    ansiesc_out(std::ostream &o, bool force_escapes=isatty(fileno(stderr)) && isatty(fileno(stdout))):out(o), use_escapes(force_escapes) {}
};
}
inline static std::ostream &operator <<(std::ostream &out, flowc::node_cfg const &nc) {
    out << "node [" << nc.id << "] timeout " << nc.timeout << " maxcc " << nc.maxcc << " " << nc.endpoint;
    return out;
}
inline static flowc::ansiesc_out &operator <<(flowc::ansiesc_out &out, char const *s) {
    char const *b = s; size_t e;
    do {
        e = strcspn(b, "'`");
        if(b[e] == '\0') 
            break;

        out.out << std::string(b, b+e);
        if(out.use_escapes) {
            if(b[e] == '\'') {
                if(++out.c1_count % 2 == 1) 
                    out.out << out.c1_begin;
                else
                    out.out << out.c1_end;
            } else {
                if(++out.c2_count % 2 == 1) 
                    out.out << out.c2_begin;
                else
                    out.out << out.c2_end;
            }
        }
        b += e + 1;
    } while(true);
    out.out << b;
    return out;
}
template <class V>
inline static flowc::ansiesc_out &operator <<(flowc::ansiesc_out &out, V s) {
    out.out << s;
    return out;
}
int main(int argc, char *argv[]) {
    std::vector<std::string> cfg;
    int cmd = 1; // updated by parse_args(): 0 OK, 1 error, 2 show help, 3 show config, 4 show info, 5 show version
    if(flowc::read_cfg(cfg, "{{NAME}}.cfg", "{{NAME/id/upper}}_") != 0 || (cmd = flowc::parse_args(argc, argv, cfg, 1)) == 1 || cmd == 2 || cmd == 5) {
        if(cmd == 2 || cmd == 5) flowc::print_banner(std::cout);
        if(cmd == 5) return 0;
        char const *argv0 = strrchr(argv[0], '/')? strrchr(argv[0], '/')+1: argv[0];
        flowc::ansiesc_out aout(std::cout);
        if(cmd == 1) aout << "Use `--help` to see the command line help and the list of valid options.\n";  
        if(cmd == 2) aout << "\nUSAGE\n"
#if !defined(NO_REST) || !(NO_REST)    
        "\t" << argv0 << " GRPC-LISTENING-PORTS [REST-LISTENING-PORTS [WEBAPP-DIRECTORY]] [OPTIONS]\n"
#else
        "\t" << argv0 << " GRPC-LISTENING-PORTS [OPTIONS]\n"
#endif
        "\n"
        "\tAll options can be set in the file '{{NAME}}.cfg'."
        " Additionally options can be set in environment variables prefixed with '{{NAME/id/upper}}_'.\n"
        "\n"
        "ARGUMENTS\n\n"
        "\tGRPC-LISTENING-PORTS\n\t\tA comma separated list of ports in the form '[(HOSTNAME|IPv6):]PORT[s]' or 'unix:PATH' or 'unix:///ABSOLUTE-PATH'.\n"
        "\t\tAppend 's' for secure connections. Set port to '0' to allocate a random port. IP numbers must be in IPv6 format.\n"
        "\n"
#if !defined(NO_REST) || !(NO_REST)    
        "\tREST-LISTENING-PORTS\n\t\tA comma separated list of ports in the form '[(HOSTNAME|IP):]PORT[s|r]'\n"
        "\t\tAppend 's' for secure connections. Append 'r' to redirect to the next secure specified address.\n"
        "\n"
        "\tWEBAPP-DIRECTORY\n\t\tDirectory with webapp content. If not sepcified the internally compiled app will be used. See `--enable-webapp` below.\n"
#endif
        "\n"
        "OPTIONS\n\n"
        "\t`--async-calls` TRUE/FALSE\n\t\tSet to false to disable asynchronous client calls. Default is enabled.\n\n"
        "\t`--cares-refresh` SECONDS\n\t\tSet the number of seconds between DNS lookup calls. Default is '" << DEFAULT_CARES_REFRESH << "' seconds.\n\n"
        "\t`--cfg`\n\t\tDisplay current configuration and exit\n\n"
#if !defined(NO_REST) || !(NO_REST)    
        "\t`--enable-webapp` TRUE/FALSE\n\t\tSet to false to disable the webapp. The webapp is automatically enabled when a webapp directory is provided.\n\n"
#endif
        "\t`--grpc-num-threads` NUMBER\n\t\tNumber of threads to run in the gRPC server. Set to '0' to use the default gRPC value.\n\n"
        "\t`--grpc-ssl-certificate` FILE\n\t\tFull path to a '.pem' file with the ssl certificate. Default is '{{NAME}}-grpc.pem' and '{{NAME}}.pem' in the current directory.\n\n"
        "\t`--help`\n\t\tShow this screen and exit\n\n"
#if !defined(NO_REST) || !(NO_REST)    
        "\t`--rest-num-threads` NUMBER\n\t\tNumber of threads to run in the REST server. Default is '" << DEFAULT_REST_THREADS << "'.\n\n"
        "\t`--rest-ssl-certificate` FILE\n\t\tFull path to a '.pem' file with the SSL certificates. Default is '{{NAME}}-rest.pem' and '{{NAME}}.pem' in the current directory.\n\n"
#endif
        "\t`--server-id` STRING\n\t\tString to use as an idendifier for this server. If not set a random string will automatically be generated.\n\n"
        "\t`--send-id` TRUE/FALSE\n\t\tSet to false to disable sending the node id in replies. Enabled by default.\n\n"
        "\t`--ssl-certificate` FILE\n\t\tFull path to a '.pem' file with the SSL certificates to be used by either gRPC or REST\n\n"
        "\t`--trace-calls` TRUE/FALSE\n\t\tEnable trace mode\n\n"
        "\t`--trace-connectios` TRUE/FALSE\n\t\tEnable the trace flag in all node calls\n\n"
        "\t`--version`\n\t\tDisplay version information\n\n"
        "NODE SPECIFIC OPTIONS\n\n"
{I:CLI_NODE_NAME{    
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-certificate` FILE\n\t\tSSL server certificate for node '{{CLI_NODE_NAME}}'\n\n"
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-endpoint` HOST:PORT`*`\n\t\tgRPC edndpoints for node '{{CLI_NODE_NAME}}' ('{{CLI_GRPC_SERVICE_NAME}}.{{CLI_METHOD_NAME}}'). "<<(flowc::ns_{{CLI_NODE_NAME/lower/id}}.endpoint.empty()? std::string("No default."): (std::string("[")+flowc::ns_{{CLI_NODE_NAME/lower/id}}.endpoint+std::string("]"))) << "\n\n"
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-maxcc` NUMBER\n\t\tMaximum number of concurrent requests that can be send to '{{CLI_NODE_NAME}}'. Default is '" << flowc::ns_{{CLI_NODE_NAME/lower/id}}.maxcc << "'.\n\n"
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-timeout` MILLISECONDS\n\t\tTimeout for calls to node '{{CLI_NODE_NAME}}'. Default is '" << flowc::ns_{{CLI_NODE_NAME/lower/id}}.timeout << "'.\n\n"
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-trace` TRUE/FALSE\n\t\tEnable the trace flag in calls to node '{{CLI_NODE_NAME}}'\n\n"
#if !defined(NO_REST) || !(NO_REST)    
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-input`\n\t\tDisplay input schema for '{{CLI_NODE_NAME}}'\n\n"
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-output`\n\t\tDisplay output schema for '{{CLI_NODE_NAME}}'\n\n"
#endif
        "\t`--node-{{CLI_NODE_NAME/id/lower/option}}-proto`\n\t\tDisplay minimal proto file for '{{CLI_NODE_NAME}}'\n\n"
}I}
        "ENTRY SPECIFIC OPTIONS\n\n"
{I:ENTRY_NAME{    "\t`--entry-{{ENTRY_NAME/lower/option}}-timeout` MILLISECONDS\n\t\tTimeout for calls to the '{{ENTRY_NAME}}' entry. Default is '" << flowc::entry_{{ENTRY_NAME}}_timeout << "'.\n\n"
#if !defined(NO_REST) || !(NO_REST)    
        "\t`--entry-{{ENTRY_NAME/lower/option}}-input`\n\t\tDisplay input schema for '{{ENTRY_NAME}}'\n\n"
        "\t`--entry-{{ENTRY_NAME/lower/option}}-output`\n\t\tDisplay output schema for '{{ENTRY_NAME}}'\n\n"
#endif
        "\t`--entry-{{ENTRY_NAME/lower/option}}-proto`\n\t\tDisplay minimal proto file for '{{ENTRY_NAME}}'\n\n"
}I}
{H:HAVE_DEFN{
        "GLOBAL DEFINES\n"
{I:DEFN{   
        "\t`--fd-{{DEFN/option}}` {{DEFT}}\n\t\tOverride the value of variable '{{DEFN}}' ('" << flowdef::v{{DEFN}} << "')\n\n"
}I}
}H}
        "Note `*`\n"
        "Node endpoints in the form 'HOST:PORT' will be sent directly to the gRPC interface.\n"
        "Node endpoints in the form '@HOST:PORT' will be looked up every '30' seconds to produce an IP list that will be used with the gRPC calls.\n"
        "Node endpoints in the form '(command)' will run 'command' every '30' seconds to produce the IP list to be used with the gRPC calls.\n"
        "\n"
        "\n"
        ;
        return cmd == 1? 1 :0;
    }

    if(cmd == 4) {
        for(int a = 1; a < argc; ++a) {
{I:CLI_NODE_NAME{
#if !defined(NO_REST) || !(NO_REST)    
        if(strcmp(argv[a], "--node-{{CLI_NODE_NAME/id/lower/option}}-input") == 0) {
            std::cout << flowinfo::schema_map.find("/-node-input/{{CLI_NODE_NAME/id/option}}")->second << "\n";
            continue;
        }
        if(strcmp(argv[a], "--node-{{CLI_NODE_NAME/id/lower/option}}-output") == 0) {
            std::cout << flowinfo::schema_map.find("/-node-output/{{CLI_NODE_NAME/id/option}}")->second << "\n";
            continue;
        }
#endif
        if(strcmp(argv[a], "--node-{{CLI_NODE_NAME/id/lower/option}}-proto") == 0) {
            std::cout << flowinfo::schema_map.find("/-node-proto/{{CLI_NODE_NAME/id/option}}")->second << "\n";
            continue;
        }
}I}
{I:ENTRY_NAME{
#if !defined(NO_REST) || !(NO_REST)    
        if(strcmp(argv[a], "--entry-{{ENTRY_NAME/lower/option}}-input") == 0) {
            std::cout << flowinfo::schema_map.find("/-input/{{ENTRY_NAME}}")->second << "\n";
            continue;
        }
        if(strcmp(argv[a], "--entry-{{ENTRY_NAME/lower/option}}-output") == 0) {
            std::cout << flowinfo::schema_map.find("/-output/{{ENTRY_NAME}}")->second << "\n";
            continue;
        }
#endif
        if(strcmp(argv[a], "--entry-{{ENTRY_NAME/lower/option}}-proto") == 0) {
            std::cout << flowinfo::schema_map.find("/-proto/{{ENTRY_NAME}}")->second << "\n";
            continue;
        }
}I}
        std::cerr << "Unrecognized option '" << argv[a] << "'\n";
        }
        return 0;
    }
        

    if(cmd != 3) flowc::print_banner(std::cout);

{I:DEFN{    if(flowc::get_cfg(cfg, "fd_{{DEFN}}") != nullptr)
        flowdef::v{{DEFN}} = flowdef::to_vt_{{DEFT}}(flowc::get_cfg(cfg, "fd_{{DEFN}}"));
}I}
    // Use the default grpc health checking service
	//grpc::EnableDefaultHealthCheckService(true);

    int error_count = 0;
    std::set<std::string> dnames;
    {   
{I:CLI_NODE_NAME{
        if(flowc::ns_{{CLI_NODE_NAME/id}}.read_from_cfg(cfg)) {
            dnames.insert(flowc::ns_{{CLI_NODE_NAME/id}}.dnames.begin(), flowc::ns_{{CLI_NODE_NAME/id}}.dnames.end());
        } else {
            ++error_count;
        }
}I}
    }
    struct stat buffer;   
    std::string ssl_certificate;
#if !defined(NO_REST) || !(NO_REST)    
    ssl_certificate = flowc::strtostring(flowc::get_cfg(cfg, "rest_ssl_certificate"), "");
    if(ssl_certificate.empty()) {
        ssl_certificate = flowc::strtostring(flowc::get_cfg(cfg, "ssl_certificate"), "");
        if(!ssl_certificate.empty()) {
            cfg.push_back("rest_ssl_certificate");
            cfg.push_back(ssl_certificate);
        } else if(stat("{{NAME}}-rest.pem", &buffer) == 0) {
            cfg.push_back("rest_ssl_certificate");
            cfg.push_back("{{NAME}}-rest.pem");
        } else if(stat("{{NAME}}.pem", &buffer) == 0) {
            cfg.push_back("rest_ssl_certificate");
            cfg.push_back("{{NAME}}.pem");
        }
    }
#endif
    ssl_certificate = flowc::strtostring(flowc::get_cfg(cfg, "grpc_ssl_certificate"), "");
    if(ssl_certificate.empty()) {
        ssl_certificate = flowc::strtostring(flowc::get_cfg(cfg, "ssl_certificate"), "");
        if(!ssl_certificate.empty()) {
            cfg.push_back("grpc_ssl_certificate");
            cfg.push_back(ssl_certificate);
        } else if(stat("{{NAME}}-grpc.pem", &buffer) == 0) {
            cfg.push_back("grpc_ssl_certificate");
            cfg.push_back("{{NAME}}-grpc.pem");
        } else if(stat("{{NAME}}.pem", &buffer) == 0) {
            cfg.push_back("grpc_ssl_certificate");
            cfg.push_back("{{NAME}}.pem");
        }
    }

    std::set<std::string> rest_listening, grpc_listening;
#if !defined(NO_REST) || !(NO_REST)    
    bool rest_need_certificate = false;
    flowc::parse_listening_port_list(rest_listening, flowc::strtostring(flowc::get_cfg(cfg, "rest_listening_ports"), ""));
    for(auto const &s: rest_listening) if(!s.empty() && s.back() == 's') {
        rest_need_certificate = true; break;
    }
    if(rest_need_certificate) {
        ssl_certificate = flowc::strtostring(flowc::get_cfg(cfg, "rest_ssl_certificate"), "");
        if(!ssl_certificate.empty() && stat(ssl_certificate.c_str(), &buffer) != 0) {
            std::cerr << "SSL certificate file not found: " << ssl_certificate << "\n";
            ++error_count;
        } else if(ssl_certificate.empty()) {
            std::cerr << "SSL certificate is needed for https\n";
            ++error_count;
        }
    }
#endif
    bool grpc_need_certificate = false;
    flowc::parse_listening_port_list(grpc_listening, flowc::strtostring(flowc::get_cfg(cfg, "grpc_listening_ports"), ""));
    for(auto const &s: grpc_listening) if(!s.empty() && s.back() == 's') {
        grpc_need_certificate = true; break;
    }
    if(grpc_need_certificate) {
        ssl_certificate = flowc::strtostring(flowc::get_cfg(cfg, "grpc_ssl_certificate"), "");
        if(!ssl_certificate.empty() && stat(ssl_certificate.c_str(), &buffer) != 0) {
            std::cerr << "SSL certificate file not found: " << ssl_certificate << "\n";
            ++error_count;
        } else if(ssl_certificate.empty()) {
            std::cerr << "SSL certificate is needed for secure grpc\n";
            ++error_count;
        }
    }
    if(cmd == 3) {
        std::cout << "# {{NAME}}.cfg\n";
        flowc::print_cfg(std::cout, cfg);
        return 0;
    }
    if(error_count != 0) return 1;
    signal(SIGTERM, flowc::signal_shutdown_handler);
    signal(SIGINT, flowc::signal_shutdown_handler);
    signal(SIGHUP, flowc::signal_shutdown_handler);

{I:CLI_NODE_NAME{    std::cout << flowc::ns_{{CLI_NODE_NAME/id}} << "\n";
}I}
{I:ENTRY_NAME{    flowc::entry_{{ENTRY_NAME}}_timeout = flowc::strtolong(flowc::get_cfg(cfg, "entry_{{ENTRY_NAME}}_timeout"), flowc::entry_{{ENTRY_NAME}}_timeout);
    std::cout << "rpc [{{ENTRY_NAME}}] timeout " << flowc::entry_{{ENTRY_NAME}}_timeout << "\n";
}I}
{I:DEFN{
    std::cout << "{{DEFN}}=" << flowdef::v{{DEFN}} << "\n";
}I}

    flowc::global_node_ID = flowc::strtostring(flowc::get_cfg(cfg, "server_id"), flowc::server_id());
    flowc::asynchronous_calls = flowc::strtobool(flowc::get_cfg(cfg, "async_calls"), flowc::asynchronous_calls);
    flowc::trace_calls = flowc::strtobool(flowc::get_cfg(cfg, "trace_calls"), flowc::trace_calls);
    flowc::trace_connections = flowc::strtobool(flowc::get_cfg(cfg, "trace_connections"), flowc::trace_connections);
    flowc::send_global_ID = flowc::strtobool(flowc::get_cfg(cfg, "send_id"), flowc::send_global_ID);
    flowc::accumulate_addresses = flowc::strtobool(flowc::get_cfg(cfg, "accumulate_addresses"), flowc::accumulate_addresses);

    // Initialize c-ares
    int status;
    status = ares_library_init(ARES_LIB_INIT_ALL);
    if(status != ARES_SUCCESS) {
        std::cout << "ares_library_init: " << ares_strerror(status) << "\n";
        return 1;
    }
    int cares_refresh = (int) flowc::strtolong(flowc::get_cfg(cfg, "cares_refresh"), DEFAULT_CARES_REFRESH);
    // Start c-ares thread
    std::cout << "lookup thread: every " << cares_refresh << "s, lookig for " << dnames << "\n";
    std::thread cares_thread([&dnames, cares_refresh] {
         casd::keep_looking(dnames, cares_refresh, 0);
    });

    int grpc_threads = (int) flowc::strtolong(flowc::get_cfg(cfg, "grpc_num_threads"), -1);


    grpc::ServerBuilder builder;
    if(grpc_threads > 0) {
        grpc::ResourceQuota grq("{{NAME/id/upper}}");
        grq.SetMaxThreads(grpc_threads);
        builder.SetResourceQuota(grq);
        std::cout << "max gRPC threads: " << grpc_threads << "\n";
    }
    {{NAME/id}}_service service;
    flowc::{{NAME/id}}_service_ptr = &service;
    bool enable_webapp = flowc::strtobool(flowc::get_cfg(cfg, "enable_webapp"), true);
    std::vector<std::pair<int, bool>> grpc_server_ports(grpc_listening.size()+1);
    std::vector<std::string> server_address(grpc_listening.size()+1);
    unsigned ap = 0, gateway_ap = grpc_listening.size()+1;
    for(auto const &a: grpc_listening) {
        ssl_certificate = flowc::strtostring(flowc::get_cfg(cfg, "grpc_ssl_certificate"), "");
        std::string p(a.empty()? std::string("0"): a);
        auto creds = grpc::InsecureServerCredentials();
        if((grpc_server_ports[ap].second = (p.back() == 's'))) {
            p.pop_back();

            grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;
            if(!flowc::read_keycert(pkcp.private_key, ssl_certificate, " RSA PRIVATE KEY-")) {
                std::cout << "failed to read private key from: " << ssl_certificate << "\n";
                ++error_count;
            }
            if(!flowc::read_keycert(pkcp.cert_chain, ssl_certificate, " CERTIFICATE-")) {
                std::cout << "failed to read certificate chain from: " << ssl_certificate << "\n";
                ++error_count;
            }

            grpc::SslServerCredentialsOptions ssl_opts;
            ssl_opts.pem_root_certs = "";
            ssl_opts.pem_key_cert_pairs.push_back(pkcp);
            ssl_opts.force_client_auth = false;
            ssl_opts.client_certificate_request = GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;

            creds = grpc::SslServerCredentials(ssl_opts);
        } else if(gateway_ap == grpc_listening.size()+1) {
            gateway_ap = ap;
        }
        server_address[ap] = p;
        if(p.find_first_not_of("0123456789") == std::string::npos) 
            p = std::string("[::]:") + p;
        else if(p.back() == ']' || p.find_first_of(':') == std::string::npos)
            p += ":0";
        builder.AddListeningPort(p, creds, &grpc_server_ports[ap].first);
        ++ap;
    }
    // When no grpc port is given, add a system allocated port
    if(gateway_ap == grpc_listening.size()+1) {
        gateway_ap = ap;
        grpc_server_ports[ap].second = false;
        server_address[ap] = "localhost:0";
        builder.AddListeningPort(server_address[ap], grpc::InsecureServerCredentials(), &grpc_server_ports[ap].first);
        ++ap;
    }

    // Register services
    builder.RegisterService(&service);
    flowc::grpc_server_ptr = builder.BuildAndStart();

    for(unsigned a = 0; a < ap; ++a) {
        if(grpc_server_ports[a].first == 0) {
            std::cout << "failed to start {{NAME}} gRPC service at " << server_address[a] << "\n";
            ++error_count;
        } else {
            std::cout << "gRPC service {{NAME}} (" << server_address[a] << ") listening on port " << grpc_server_ports[a].first;
            if(grpc_server_ports[a].second) std::cout << " (tls enabled)";
            std::cout << "\n";
        }
    }
    if(error_count == 0) {
        std::cout << "server id: " << flowc::global_node_ID << "\n";
        std::cout << "start time: " << flowc::global_start_time << "\n";
#if !defined(NO_REST) || !(NO_REST)    
        // Set up the REST gateway if enabled
        rest::gateway_endpoint = server_address[gateway_ap];
        if(strncmp(rest::gateway_endpoint.c_str(), "unix:", strlen("unix:")) == 0 || strncmp(rest::gateway_endpoint.c_str(), "unix-abstract:", strlen("unix-abstract:")) == 0) {
            // unix domain socket
        } else if(rest::gateway_endpoint.empty() || rest::gateway_endpoint.find_first_not_of("0123456789") == std::string::npos) {
            // port only 
            rest::gateway_endpoint = flowc::sfmt() << "localhost:" << grpc_server_ports[gateway_ap].first;
        } else {
            // host or host:port
            auto cpos = rest::gateway_endpoint.back() == ']'? std::string::npos: rest::gateway_endpoint.find_last_of(':');
            if(cpos != std::string::npos)
                rest::gateway_endpoint = rest::gateway_endpoint.substr(0, cpos);
            rest::gateway_endpoint = flowc::sfmt() << rest::gateway_endpoint << ':' << grpc_server_ports[gateway_ap].first;        
        }
        char const *rest_port = flowc::get_cfg(cfg, "rest_listening_ports");
        if(rest_port != nullptr) {
            std::cout << "starting REST gatweay for gRPC at " << rest::gateway_endpoint << "\n";
            rest::app_directory = flowc::strtostring(flowc::get_cfg(cfg, "webapp_directory"), rest::app_directory);
            if(flowc::get_cfg(cfg, "rest_num_threads") == nullptr) {
                cfg.push_back("rest_num_threads"); 
                cfg.push_back(std::to_string(DEFAULT_REST_THREADS));
            }
            if(rest::start_civetweb(cfg, !enable_webapp) != 0) {
                std::cout << "failed to start REST gateway service\n";
                ++error_count;
            }
        }
#endif        
    }
    if(error_count == 0) {
        std::cout 
            << "call id: " << (flowc::send_global_ID ? "yes": "no") 
            << ", trace: " << (flowc::trace_calls? "yes": "no")
            << ", asynchronous client calls: " << (flowc::asynchronous_calls? "yes": "no") 
            << "\n";
        std::cout << std::endl;
        // Wait for the server to shutdown. This can only be stopped from the signal handler.
        flowc::grpc_server_ptr->Wait();
        std::cout << "gRPC server exited" << std::endl;
#if !defined(NO_REST) || !(NO_REST)    
        rest::stop_civetweb();
#endif        
    } else {
        if(flowc::grpc_server_ptr)
            flowc::grpc_server_ptr->Shutdown();
    }
    casd::stop_looking = true;
    cares_thread.join();
    ares_library_cleanup();
    flowc::grpc_server_ptr = nullptr;
    return error_count;
}
