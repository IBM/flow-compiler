/************************************************************************************************************
 *
 * {{NAME}}-server.C 
 * generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
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

extern "C" {
#include <civetweb.h>
}
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
    if(td < 1.0) { td *= 1000; unit = "us"; }
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
    std::string so(s);
    std::transform(so.begin(), so.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(s) != 0;
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
#if defined(OSSP_UUID) || defined(NO_UUID)
#include <uuid.h>
static std::string server_id() {
#if defined(OSSP_UUID)    
    uuid_t *puid = nullptr;
    char *sid = nullptr;;
    if(uuid_create(&puid) == UUID_RC_OK) {
        uuid_make(puid, UUID_MAKE_V4);
        uuid_export(puid, UUID_FMT_STR, &sid, nullptr);
        std::string id = std::string("{{NAME}}-")+sid;
        uuid_destroy(puid);
        return id;
    }
#endif
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

static unsigned read_cfg(std::vector<std::string> &cfg, std::string const &filename, std::string const &env_prefix) {
    std::ifstream cfgs(filename.c_str());
    std::string line, line_buffer;

    do {
        line_buffer.clear();
        while(std::getline(cfgs, line)) {
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
            cfg.push_back(std::string(vp, vvp));
            std::transform(cfg.back().begin(), cfg.back().end(), cfg.back().begin(), ::tolower);
            cfg.push_back(vvp+1);
        }
    }

    return cfg.size();
}
static char const *get_cfg(std::vector<std::string> const &cfg, std::string const &name) {
    for(auto p = cfg.rbegin(), e = cfg.rend(); p != e; ++p, ++p) {
        auto n = p + 1;
        if(*n == name) return p->c_str();
    }
    return nullptr;
}

enum Nodes_Enum {
    NO_NODE = 0 {I:CLI_NODE_UPPERID{, {{CLI_NODE_UPPERID}}}I}
};

struct node_cfg {
    std::string id;
    std::set<std::string> fendpoints;
    std::set<std::string> dendpoints;
    std::set<std::string> dnames;
    std::string endpoint;
    int maxcc;
    long timeout;
    bool trace;

    node_cfg(std::string const &a_id, int a_maxcc, long a_timeout, std::string const &a_endpoint):
        id(a_id), maxcc(a_maxcc), timeout(a_timeout), endpoint(a_endpoint), trace(false) {
        }

    bool read_from_cfg(std::vector<std::string> const &cfg);
};

{I:CLI_NODE_UPPERID{node_cfg ns_{{CLI_NODE_ID}}("{{CLI_NODE_ID}}", /*maxcc*/{{CLI_NODE_MAX_CONCURRENT_CALLS}}, /*timeout*/{{CLI_NODE_TIMEOUT:DEFAULT_NODE_TIMEOUT}}, "{{CLI_NODE_ENDPOINT}}");
}I}

{I:ENTRY_NAME{long entry_{{ENTRY_NAME}}_timeout = {{ENTRY_TIMEOUT:DEFAULT_ENTRY_TIMEOUT}};
}I}

std::string global_node_ID;
bool asynchronous_calls = true;
bool trace_calls = false;
bool send_global_ID = true;
bool trace_connections = false;
bool accumulate_addresses = false;

std::string global_start_time = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1000); 

static std::string json_escape(std::string const &s) {
    std::string r;
    for(auto c: s) switch (c) {
        case '\t': r+= "\\t"; break;
        case '\r': r+= "\\r"; break;
        case '\a': r+= "\\a"; break;
        case '\b': r+= "\\b"; break;
        case '\v': r+= "\\v"; break;
        case '\f': r+= "\\f"; break;
        case '\n': r+= "\\n"; break;
        case '"':  r+= "\\\""; break;
        case '\\': r+= "\\\\"; break;
        default: r+= c; break;
    }
    return r;
}
static std::string json_string(std::string const &s) {
    return std::string("\"") + json_escape(s) + "\"";
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
/*
template<>
sfmt &sfmt::operator <<(call_info const &v) {
    os << v; return *this;
}
*/
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
        FLOGC(flowc::trace_connections) << "ADDRESESS for " << entry << " version=" << std::get<2>(asp->second) << ", " << std::get<0>(asp->second) << "\n";
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

    for(current_iteration = 1; loops == 0 || current_iteration <= loops; ++current_iteration) {
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

class {{NAME_ID}}_service *{{NAME_ID}}_service_ptr = nullptr;

{I:GRPC_GENERATED_H{#include "{{GRPC_GENERATED_H}}"
}I}

namespace flowc {


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
            std::shared_ptr<::grpc::Channel> channel(::grpc::CreateChannel(aep, ::grpc::InsecureChannelCredentials()));
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
                std::shared_ptr<::grpc::Channel> channel(::grpc::CreateChannel(ipep, ::grpc::InsecureChannelCredentials()));
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
#define GRPC_RECEIVED(NODE_NAME, SERVER_CALL_ID, CLIENT_CALL_ID, NODE_ID, STATUS, CONTEXT, RESPONSE_PTR)
#endif
#ifndef GRPC_SENDING
#define GRPC_SENDING(NODE_NAME, SERVER_CALL_ID, CLIENT_CALL_ID, NODE_ID, CONTEXT, REQUEST_PTR)
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
// to use, define REST_CHECK_{{ENTRY_UPPERID}}_BEFORE and/or REST_CHECK_{{ENTRY_UPPERID}}_AFTER with the name of function
}I}

class {{NAME_ID}}_service final: public {{CPP_SERVER_BASE}}::Service {
public:
    bool Async_Flag = flowc::asynchronous_calls;
    // Global call counter used to generate an unique id for each call regardless of entry
    std::atomic<long> Call_Counter;
    // Current number of active calls for regardless of entry
    std::atomic<int> Active_Calls;

        /**
         * Some notes on error codes (from https://grpc.io/grpc/cpp/classgrpc_1_1_status.html):
         * UNAUTHENTICATED - The request does not have valid authentication credentials for the operation.
         * PERMISSION_DENIED - Must not be used for rejections caused by exhausting some resource (use RESOURCE_EXHAUSTED instead for those errors). 
         *                     Must not be used if the caller can not be identified (use UNAUTHENTICATED instead for those errors).
         * INVALID_ARGUMENT - Client specified an invalid argument. 
         * FAILED_PRECONDITION - Operation was rejected because the system is not in a state required for the operation's execution.
         */

{I:CLI_NODE_NAME{
    /* {{CLI_NODE_NAME}} line {{CLI_NODE_LINE}}
     */
#define SET_METADATA_{{CLI_NODE_ID}}(context) {{CLI_NODE_METADATA}}

    std::mutex {{CLI_NODE_ID}}_conm; // mutex to guard {{CLI_NODE_ID}}_nversion and {CLI_NODE_ID}}_conp
    int {{CLI_NODE_ID}}_nversion = 0;
    std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> {{CLI_NODE_ID}}_conp;

    std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> {{CLI_NODE_ID}}_get_connector() {
        std::lock_guard<std::mutex> guard({{CLI_NODE_ID}}_conm);
        if(flowc::ns_{{CLI_NODE_ID}}.dendpoints.size() == 0) {
            return {{CLI_NODE_ID}}_conp;
        }
        std::map<std::string, std::vector<std::string>> addresses = {{CLI_NODE_ID}}_conp->addresses;
        int ver = casd::get_latest_addresses(addresses, {{CLI_NODE_ID}}_nversion,  flowc::ns_{{CLI_NODE_ID}}.dnames);
        if(ver == {{CLI_NODE_ID}}_nversion) 
            return {{CLI_NODE_ID}}_conp;
        {{CLI_NODE_ID}}_nversion = ver;

        FLOGC(flowc::trace_connections) << "new @{{CLI_NODE_NAME}} connector to " << flowc::ns_{{CLI_NODE_ID}}.fendpoints << " " << flowc::ns_{{CLI_NODE_ID}}.dendpoints << "\n"; 
        auto {{CLI_NODE_ID}}_cp = new ::flowc::connector<{{CLI_SERVICE_NAME}}>({{CLI_NODE_ID}}_conp->active_calls, flowc::ns_{{CLI_NODE_ID}}, addresses);
        {{CLI_NODE_ID}}_conp.reset({{CLI_NODE_ID}}_cp);
        return {{CLI_NODE_ID}}_conp;
    }
    std::unique_ptr<::grpc::ClientAsyncResponseReader<{{CLI_OUTPUT_TYPE}}>> {{CLI_NODE_ID}}_prep(int &ConN, flowc::call_info const &CIF, int CCid,
            std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> ConP, ::grpc::CompletionQueue &CQ, ::grpc::ClientContext &CTX, {{CLI_INPUT_TYPE}} *A_inp) {
        FLOGC(CIF.trace_call || flowc::ns_{{CLI_NODE_ID}}.trace) << std::make_tuple(&CIF, CCid) << "{{CLI_NODE_NAME}} prepare " << flowc::log_abridge(*A_inp) << "\n";
        if(flowc::send_global_ID) {
            CTX.AddMetadata("node-id", flowc::global_node_ID);
            CTX.AddMetadata("start-time", flowc::global_start_time);
        }
        SET_METADATA_{{CLI_NODE_ID}}(CTX)
        GRPC_SENDING("{{CLI_NODE_ID}}", CIF, CCid, flowc::{{CLI_NODE_UPPERID}}, CTX, A_inp)
        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds(flowc::ns_{{CLI_NODE_ID}}.timeout);
        CTX.set_deadline(std::min(deadline, CIF.deadline));
        if(ConP->count() == 0) 
            return nullptr;
        return ConP->stub(ConN, CIF, CCid)->PrepareAsync{{CLI_METHOD_NAME}}(&CTX, *A_inp, &CQ);
    }
    ::grpc::Status {{CLI_NODE_ID}}_call(flowc::call_info const &CIF, int CCid, std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> ConP, {{CLI_OUTPUT_TYPE}} *A_outp, {{CLI_INPUT_TYPE}} *A_inp) {
        ::grpc::ClientContext L_context;
        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds(flowc::ns_{{CLI_NODE_ID}}.timeout);
        L_context.set_deadline(std::min(deadline, CIF.deadline));
        if(flowc::send_global_ID) {
            L_context.AddMetadata("node-id", flowc::global_node_ID);
            L_context.AddMetadata("start-time", flowc::global_start_time);
        }
        SET_METADATA_{{CLI_NODE_ID}}(L_context)
        GRPC_SENDING("{{CLI_NODE_ID}}", CIF, CCid, flowc::{{CLI_NODE_UPPERID}}, CTX, A_inp)
        ::grpc::Status L_status;
        if(ConP->count() == 0) {
            L_status = ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, ::flowc::sfmt() << "No addresses found for {{CLI_NODE_ID}}: " << flowc::ns_{{CLI_NODE_ID}}.endpoint << "\n");
        } else {
            int ConN = -1;
            L_status = ConP->stub(ConN, CIF, CCid)->{{CLI_METHOD_NAME}}(&L_context, *A_inp, A_outp);
            ConP->finished(ConN, CIF, CCid, L_status.error_code() == grpc::StatusCode::UNAVAILABLE);
            GRPC_RECEIVED("{{CLI_NODE_ID}}", CIF, CCid, flowc::{{CLI_NODE_UPPERID}}, L_status, L_context, A_outp)
        }
        FLOGC(CIF.trace_call || flowc::ns_{{CLI_NODE_ID}}.trace) << std::make_tuple(&CIF, CCid) << "{{CLI_NODE_NAME}} request: " << flowc::log_abridge(*A_inp) << "\n";
        if(!L_status.ok()) {
            GRPC_ERROR(CIF, CCid, "{{CLI_NODE_NAME}} ", L_status, L_context);
        } else {
            FLOGC(CIF.trace_call || flowc::ns_{{CLI_NODE_ID}}.trace) << std::make_tuple(&CIF, CCid) << "{{CLI_NODE_NAME}} reply: " << flowc::log_abridge(*A_outp) << "\n";
        }
        return L_status;
    }}I}
    // Constructor
    {{NAME_ID}}_service() {
        Call_Counter = 1;
        Active_Calls = 0;
        {I:CLI_NODE_ID{
        {{CLI_NODE_ID}}_nversion = 0;
        auto {{CLI_NODE_ID}}_cp = new ::flowc::connector<{{CLI_SERVICE_NAME}}>(Active_Calls, flowc::ns_{{CLI_NODE_ID}}, std::map<std::string, std::vector<std::string>>());
        {{CLI_NODE_ID}}_conp.reset({{CLI_NODE_ID}}_cp);
        }I}
        {I:ENTRY_CODE{
}I}
    }
{I:ENTRY_CODE{
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

namespace rest {
std::string gateway_endpoint;
std::string app_directory("./app");
std::string docs_directory("./docs");
std::string www_directory("./www");

std::map<std::string, char const *> schema_map = {
{I:CLI_NODE_NAME{    { "/-node-output/{{CLI_NODE_NAME}}", {{CLI_OUTPUT_SCHEMA_JSON_C}} },
    { "/-node-input/{{CLI_NODE_NAME}}", {{CLI_INPUT_SCHEMA_JSON_C}} },
}I}
{I:ENTRY_NAME{   { "/-output/{{ENTRY_NAME}}", {{ENTRY_OUTPUT_SCHEMA_JSON_C}} }, 
    { "/-input/{{ENTRY_NAME}}", {{ENTRY_INPUT_SCHEMA_JSON_C}} }, 
}I}
};
static int log_message(const struct mg_connection *conn, const char *message) {
    FLOG << message << std::flush;
	return 1;
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
        {I:ENTRY_NAME{
            << "\"/{{ENTRY_NAME}}\": {"
               "\"timeout\": " << flowc::entry_{{ENTRY_NAME}}_timeout << ","
               "\"input-schema\": " << schema_map.find("/-input/{{ENTRY_NAME}}")->second << "," 
               "\"output-schema\": " << schema_map.find("/-output/{{ENTRY_NAME}}")->second << "" 
               "},"
        }I}
        {I:CLI_NODE_NAME{
          <<   "\"/-node/{{CLI_NODE_NAME}}\": {"
               "\"timeout\": " << flowc::ns_{{CLI_NODE_ID}}.timeout << ","
               "\"input-schema\": " << schema_map.find("/-node-input/{{CLI_NODE_NAME}}")->second << "," 
               "\"output-schema\": " << schema_map.find("/-node-output/{{CLI_NODE_NAME}}")->second << "" 
               "},"
        }I}
        "\"/-info\": {}"
    "}";
    return json_reply(conn, info.c_str(), info.length());
}
static int get_schema(struct mg_connection *conn, void *) {
	char const *local_uri = mg_get_request_info(conn)->local_uri;
    auto sp = schema_map.find(local_uri);
    if(sp == schema_map.end()) 
        return not_found(conn, "Name not recognized");
    return json_reply(conn, sp->second);
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
                FLOG << "error: " << local_uri << ": Read execeeded buffer size of " << MAX_REST_REQUEST_SIZE << " bytes\n";
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
    
}
std::atomic<long> call_counter;
{I:ENTRY_NAME{
static int REST_{{ENTRY_NAME}}_call(flowc::call_info const &cif, struct mg_connection *A_conn, std::string const &A_inp_json) {
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

#if defined(REST_CHECK_{{ENTRY_UPPERID}}_BEFORE) || defined(REST_CHECK_{{ENTRY_UPPERID}}_AFTER)
    char const *check_header = mg_get_header(A_conn, RFH_CHECK);
#endif
    
#ifdef REST_CHECK_{{ENTRY_UPPERID}}_BEFORE
    {
        int http_code; std::string http_message, http_body; 
        if(!REST_CHECK_{{ENTRY_UPPERID}}_BEFORE(http_code, http_message, http_body, check_header, L_context, &L_inp, xtra_headers)) {
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
#ifdef REST_CHECK_{{ENTRY_UPPERID}}_AFTER
    {
        int http_code; std::string http_message, http_body; 
        if(!REST_CHECK_{{ENTRY_UPPERID}}_AFTER(http_code, http_message, http_body, check_header, L_context, &L_inp, L_status, &L_outp, xtra_headers)) {
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
static int REST_{{ENTRY_NAME}}_handler(struct mg_connection *A_conn, void *A_cbdata) {
    flowc::call_info cif("{{ENTRY_NAME}}", call_counter.fetch_add(1, std::memory_order_seq_cst), A_conn, flowc::entry_{{ENTRY_NAME}}_timeout);
    std::string input_json;
    int rc = rest::get_form_data(A_conn, input_json);

    FLOG << cif << "REST-entry: " << mg_get_request_info(A_conn)->local_uri 
        << " async, time, trace, request-length, response-type: " << cif.async_calls << ", "  << cif.time_call << ", " << cif.trace_call << ", " << input_json.length() << ", " << (cif.return_protobuf? "protobuf": "json") << "\n";

    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0) 
        rc = rest::not_found(A_conn, "Resource not found");
    else if(rc <= 0) 
        rc = rest::bad_request_error(A_conn);
    else 
        rc = REST_{{ENTRY_NAME}}_call(cif, A_conn, input_json);

    FLOG << cif << "REST-return: " << rc << " \n";
    return rc;
}
}I}
{I:CLI_NODE_NAME{
static int REST_node_{{CLI_NODE_ID}}_call(flowc::call_info const &cif, struct mg_connection *A_conn, std::string const &A_inp_json) {
    std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> connector = {{NAME_ID}}_service_ptr->{{CLI_NODE_ID}}_get_connector();

    {{CLI_OUTPUT_TYPE}} L_outp; 
    {{CLI_INPUT_TYPE}} L_inp;

    FLOGC(cif.trace_call) << cif << "body: " << flowc::log_abridge(A_inp_json) << "\n";

    auto L_conv_status = google::protobuf::util::JsonStringToMessage(A_inp_json, &L_inp);
    if(!L_conv_status.ok()) return rest::conversion_error(A_conn, L_conv_status);

    ::grpc::ClientContext L_context;
    if(cif.have_deadline) L_context.set_deadline(cif.deadline);
    SET_METADATA_{{CLI_NODE_ID}}(L_context)

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
static int REST_node_{{CLI_NODE_ID}}_handler(struct mg_connection *A_conn, void *A_cbdata) {
    flowc::call_info cif("node-{{CLI_NODE_NAME}}", call_counter.fetch_add(1, std::memory_order_seq_cst), A_conn, flowc::ns_{{CLI_NODE_ID}}.timeout);
    std::string input_json;
    int rc = rest::get_form_data(A_conn, input_json);

    FLOG << cif << "REST-node-entry: " << mg_get_request_info(A_conn)->local_uri
        << " trace, request-length, response-type: " << cif.trace_call << ", " << input_json.length() << ", " << (cif.return_protobuf? "protobuf": "json") << "\n";

    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0) 
        rc = rest::not_found(A_conn, "Resource not found");
    else if(rc <= 0) 
        rc = rest::bad_request_error(A_conn);
    else 
        rc = REST_node_{{CLI_NODE_ID}}_call(cif, A_conn, input_json);
    FLOG << cif << "REST-node-return: " << rc << "\n";
    return rc;
}
}I}
namespace rest {

int start_civetweb(std::vector<std::string> const &cfg, bool rest_only) {
    call_counter = 1;
    long request_timeout_ms = 0;
{I:ENTRY_NAME{    request_timeout_ms = std::max(flowc::entry_{{ENTRY_NAME}}_timeout, request_timeout_ms);
}I}
    if(!rest_only) {
{I:CLI_NODE_NAME{        request_timeout_ms = std::max(request_timeout_ms, flowc::ns_{{CLI_NODE_ID}}.timeout);
}I}
    }
    request_timeout_ms += request_timeout_ms / 20;
    std::string request_timeout_ms_s(std::to_string(request_timeout_ms));
    std::vector<const char *> optbuf = {
        "document_root", "/dev/null",
        "request_timeout_ms", request_timeout_ms_s.c_str(),
        "error_log_file", "error.log",
        "extra_mime_types", ".flow=text/plain,.proto=text/plain,.svg=image/svg+xml",
        "enable_auth_domain_check", "no",
        "max_request_size", "65536"
    };
    for(unsigned i = 0; i < cfg.size(); i += 2) {
        char const *n = cfg[i].c_str();
        if(strncmp(n, "rest_", strlen("rest_")) == 0) {
            if(mg_get_option(nullptr, n+strlen("rest_")) == nullptr) {
                std::cout << "ignoring invalid rest option: " << cfg[i] << ": " << cfg[i+1] << "\n";
            } else {
                optbuf.push_back(n+strlen("rest_"));
                optbuf.push_back(cfg[i+1].c_str());
            }
        }
    }
    optbuf.push_back(nullptr);

	struct mg_callbacks callbacks;
	struct mg_context *ctx;

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.log_message = rest::log_message;
	ctx = mg_start(&callbacks, 0, &optbuf[0]);

	if(ctx == nullptr) return 1;
{I:ENTRY_NAME{    mg_set_request_handler(ctx, "/{{ENTRY_NAME}}", REST_{{ENTRY_NAME}}_handler, (void *) "/{{ENTRY_NAME}}");
}I}
	mg_set_request_handler(ctx, "/", root_handler, 0);
    if(!rest_only) {
	    mg_set_request_handler(ctx, "/-input", get_schema, 0);
	    mg_set_request_handler(ctx, "/-output", get_schema, 0);
	    mg_set_request_handler(ctx, "/-node-input", get_schema, 0);
	    mg_set_request_handler(ctx, "/-node-output", get_schema, 0);
	    mg_set_request_handler(ctx, "/-info", get_info, 0);
{I:CLI_NODE_NAME{        mg_set_request_handler(ctx, "/-node/{{CLI_NODE_NAME}}", REST_node_{{CLI_NODE_ID}}_handler, (void *) "/-node/{{CLI_NODE_NAME}}");
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
    std::cout <<  "REST gateway at";
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
    std::cout << "web app enabled: " << (rest_only? "no": "yes") << "\n";
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

namespace flowc {
bool node_cfg::read_from_cfg(std::vector<std::string> const &cfg) {
    trace = strtobool(get_cfg(cfg, std::string("node_") + id + "_trace"), trace);
    maxcc = (int) strtolong(get_cfg(cfg, std::string("node_") + id + "_maxcc"), maxcc);
    timeout = strtolong(get_cfg(cfg, std::string("node_") + id + "_timeout"), timeout);
    char const *ep = get_cfg(cfg, std::string("node_") + id + "_endpoint");
    if(ep != nullptr) endpoint = ep;
    return !endpoint.empty() &&
        casd::parse_endpoint_list(dnames, dendpoints, fendpoints, endpoint);
}

}
inline static std::ostream &operator <<(std::ostream &out, flowc::node_cfg const &nc) {
    out << "node [" << nc.id << "] timeout " << nc.timeout << " " << nc.endpoint;
    return out;
}

int main(int argc, char *argv[]) {
    if(argc < 2 || argc > 4) {
       std::cout << "Usage: " << argv[0] << " GRPC-PORT [REST-PORT [APP-DIRECTORY]] \n\n";
       std::cout << "Endpoints (host:port) for each node:\n";
       {I:CLI_NODE_NAME{std::cout << "{{NAME_UPPERID}}_NODE_{{CLI_NODE_UPPERID}}_ENDPOINT= for node {{CLI_NODE_NAME}}/{{CLI_GRPC_SERVICE_NAME}}.{{CLI_METHOD_NAME}}\n";
       }I}
       std::cout << "\n";
       std::cout << "\n";
       std::cout << "Set {{NAME_UPPERID}}_ENABLE_WEBAPP=0 to disable the web-app when the REST service is enabled\n";
       std::cout << "Set {{NAME_UPPERID}}_TRACE_CALLS=1 to enable trace mode\n";
       std::cout << "Set {{NAME_UPPERID}}_ASYNC_CALLS=0 to disable asynchronous client calls\n";
       std::cout << "Set {{NAME_UPPERID}}_NODE_ID= to override the server ID\n"; 
       std::cout << "Set {{NAME_UPPERID}}_SEND_ID=0 to disable sending the server ID\n"; 
       std::cout << "Set {{NAME_UPPERID}}_CARES_REFRESH= to the number of seconds between DNS lookups (" << DEFAULT_CARES_REFRESH << ")\n"; 
       std::cout << "Set {{NAME_UPPERID}}_GRPC_NUM_THREADS= to change the number of gRPC threads, leave 0 for no change (" << DEFAULT_GRPC_THREADS << ")\n";
       std::cout << "\n";
       return 1;
    }
    std::cout 
        << "{{INPUT_FILE}} ({{MAIN_FILE_TS}})\n" 
        << "{{FLOWC_NAME}} {{FLOWC_VERSION}} ({{FLOWC_BUILD}})\n"
        <<  "grpc " << grpc::Version() << "\n"
#if defined(__clang__)          
        << "clang++ " << __clang_version__ << "\n"
#elif defined(__GNUC__) 
        << "g++ " << __VERSION__ << "\n"
#else
#endif
        << std::endl;
    
    std::vector<std::string> cfg;
    flowc::read_cfg(cfg, "{{NAME}}.cfg", "{{NAME_UPPERID}}_");

    // Use the default grpc health checking service
	grpc::EnableDefaultHealthCheckService(true);

    int error_count = 0;
    std::set<std::string> dnames;
    {   
        {I:CLI_NODE_ID{
        if(flowc::ns_{{CLI_NODE_ID}}.read_from_cfg(cfg)) {
            dnames.insert(flowc::ns_{{CLI_NODE_ID}}.dnames.begin(), flowc::ns_{{CLI_NODE_ID}}.dnames.end());
            std::cout << flowc::ns_{{CLI_NODE_ID}} << "\n";
        } else {
            ++error_count;
            std::cout << "Failed to find endpoint for node [{{CLI_NODE_ID}}]\n";
        }
        }I}
    }
    if(error_count != 0) return 1;
    {I:ENTRY_NAME{flowc::entry_{{ENTRY_NAME}}_timeout = flowc::strtolong(flowc::get_cfg(cfg, "entry_{{ENTRY_NAME}}_timeout"), flowc::entry_{{ENTRY_NAME}}_timeout);
    }I}

    flowc::global_node_ID = flowc::strtostring(flowc::get_cfg(cfg, "node_id"), flowc::server_id());
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

    int listening_port;
    grpc::ServerBuilder builder;
    if(grpc_threads > 0) {
        grpc::ResourceQuota grq("{{NAME_UPPERID}}");
        grq.SetMaxThreads(grpc_threads);
        builder.SetResourceQuota(grq);
        std::cout << "max gRPC threads: " << grpc_threads << "\n";
    }

    {{NAME_ID}}_service service;
    {{NAME_ID}}_service_ptr = &service;
    bool enable_webapp = flowc::strtobool(flowc::get_cfg(cfg, "enable_webapp"), true);

    // Listen on the given address without any authentication mechanism.
    std::string server_address("[::]:"); server_address += argv[1];
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &listening_port);

    // Register services
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    if(listening_port == 0) {
        std::cout << "failed to start {{NAME}} gRPC service at " << listening_port << "\n";
        return 1;
    }
    std::cout << "node id: " << flowc::global_node_ID << "\n";
    std::cout << "start time: " << flowc::global_start_time << "\n";
    rest::gateway_endpoint = flowc::sfmt() << "localhost:" << listening_port;
    std::cout << "gRPC service {{NAME}} listening on port: " << listening_port << "\n";

    // Set up the REST gateway if enabled
    if(argc > 2) {
        cfg.push_back("rest_listening_ports");
        cfg.push_back(argv[2]);
    }
    char const *rest_port = flowc::get_cfg(cfg, "rest_listening_ports");
    if(rest_port != nullptr) {
        if(argc > 3) {
            cfg.push_back("webapp_directory");
            cfg.push_back(argv[2]);
        }
        rest::app_directory = flowc::strtostring(flowc::get_cfg(cfg, "webapp_directory"), rest::app_directory);
        if(flowc::get_cfg(cfg, "rest_num_threads") == nullptr) {
            cfg.push_back("rest_num_threads"); 
            cfg.push_back(std::to_string(DEFAULT_REST_THREADS));
        }
        if(rest::start_civetweb(cfg, !enable_webapp) != 0) {
            std::cout << "Failed to start REST gateway service\n";
            return 1;
        }
    }
    std::cout 
        << "call id: " << (flowc::send_global_ID ? "yes": "no") 
        << ", trace: " << (flowc::trace_calls? "yes": "no")
        << ", asynchronous client calls: " << (flowc::asynchronous_calls? "yes": "no") 
        << "\n";

    std::cout << std::endl;
    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();

    cares_thread.join();
    ares_library_cleanup();
    return 0;
}
