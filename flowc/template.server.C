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
}
template <class F, class E>
inline static std::ostream &operator << (std::ostream &out, std::pair<F, E> const &c) {
    char const *sep = "";
    out << "(" << c.first << ", " << c.second << ")";
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
    char tmbuf[64], buf[64];
    strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(buf, sizeof(buf), "%s.%03ld", tmbuf, tv.tv_usec/1000);
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

enum Nodes_Enum {
    NO_NODE = 0 {I:CLI_NODE_UPPERID{, {{CLI_NODE_UPPERID}}}I}
};

{I:CLI_NODE_UPPERID{bool trace_{{CLI_NODE_ID}} = strtobool(std::getenv("{{NAME_UPPERID}}_TRACE_{{CLI_NODE_UPPERID}}"), false);
std::set<std::string> {{CLI_NODE_ID}}_fendpoints;
std::set<std::string> {{CLI_NODE_ID}}_dendpoints;
std::set<std::string> {{CLI_NODE_ID}}_dnames;
int {{CLI_NODE_ID}}_maxcc = (int) strtolong(std::getenv("{{CLI_NODE_UPPERID}}_MAXCC"), {{CLI_NODE_MAX_CONCURRENT_CALLS}});
long {{CLI_NODE_ID}}_timeout = strtolong(std::getenv("{{CLI_NODE_UPPERID}}_TIMEOUT"), {{CLI_NODE_TIMEOUT:3600000}});
}I}
std::string global_node_ID = std::getenv("{{NAME_UPPERID}}_NODE_ID") == nullptr? server_id(): std::string(std::getenv("{{NAME_UPPERID}}_NODE_ID"));
bool asynchronous_calls = strtobool(std::getenv("{{NAME_UPPERID}}_ASYNC"), true);
bool trace_calls = strtobool(std::getenv("{{NAME_UPPERID}}_TRACE"), false);
bool send_global_ID = strtobool(std::getenv("{{NAME_UPPERID}}_SEND_ID"), true);
bool trace_connections = strtobool(std::getenv("{{NAME_UPPERID}}_TRACE_CONNECTIONS"), false);
std::string global_start_time = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1000); 
bool accumulate_addresses = strtobool(std::getenv("{{NAME_UPPERID}}_CARES_ACCUMULATE"), false);

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
struct callid {
    long scid; int ccid;
    callid(long s, int c=0):scid(s), ccid(c) {
    }
};
class sfmt {
    std::ostringstream os;
public:
    inline operator std::string() {
        return os.str();
    }
    template <class T> inline sfmt &operator <<(T v);
};
template <> sfmt&sfmt::operator <<(callid const c) {
    os << '[' << c.scid;
    if(c.ccid > 0) os << ':' << c.ccid;
    os << "] ";
    return *this;
}
template <class T> sfmt &sfmt::operator <<(T v) {
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
template <class C>
static bool get_metadata_bool(C mm, std::string const &key, bool default_value=false) { 
    auto vp = mm.find(key);
    if(vp == mm.end()) return default_value;
    return stringtobool(std::string((vp->second).data(), (vp->second).length()), default_value);
}
#define TIME_INFO_BEGIN(enabled)  std::stringstream Time_info; if(enabled) Time_info << "[";
#define TIME_INFO_GET(enabled)    ((Time_info << "]"), Time_info.str())
#define TIME_INFO_END(enabled)

static void record_time_info(std::ostream &out, int stage, std::string const &method_name, std::string const &stage_name,
    std::chrono::steady_clock::duration call_elapsed_time, std::chrono::steady_clock::duration stage_duration, int calls) {
    if(stage != 1) out << ",";
    out << "{" 
           "\"method\":" << json_string(method_name) << ","
           "\"stage-name\":" << json_string(stage_name) << ","
           "\"stage\":" << stage << ","
           "\"calls\":" << calls << ","
           "\"duration\":" << double(stage_duration.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den << ","
           "\"started\":" << double(call_elapsed_time.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den << ","
           "\"duration-u\":\"" << stage_duration << "\","
           "\"started-u\":\"" << call_elapsed_time << "\""
           "}";
}

#define GRPC_ERROR(cid, ccid, text, status, context) FLOG << flowc::callid(cid, ccid) << (text) << " grpc error: " << (status).error_code() << " context: " << (context).debug_error_string() << "\n";

#define PRINT_TIME(method, stage, stage_name, call_elapsed_time, stage_duration, calls) {\
    if(Time_call) flowc::record_time_info(Time_info, stage, method, stage_name, (call_elapsed_time), (stage_duration), calls);\
    FLOGC(Trace_call) << flowc::callid(CID) << "time-call: " << method << " stage " << stage << " (" << stage_name \
    << ") started after " << call_elapsed_time << " and took " << stage_duration << " for " << calls << " call(s)\n"; \
    }



template<class CSERVICE> class connector {
    typedef typename CSERVICE::Stub Stub_t;
    typedef decltype(std::chrono::system_clock::now()) ts_t;
    std::vector<std::tuple<int, ts_t, std::unique_ptr<Stub_t>, std::string, std::string>> stubs;
    std::vector<int> activity_index;
    std::atomic<unsigned long> cc;
    std::mutex allocator;
    std::unique_ptr<Stub_t> dead_end;
public:
    std::string label; 
    std::map<std::string, std::vector<std::string>> addresses;
    connector(std::string const &a_label, int maxcc, std::set<std::string> const &d_names,
            std::set<std::string> const &f_endpoints, std::set<std::string> const &d_endpoints, 
            std::map<std::string, std::vector<std::string>> const &a_addresses): 
        label(a_label), addresses(a_addresses) {
        //FLOG << "connector for @" << a_label << " fixed: " << f_endpoints << " dynamic: " << d_endpoints  << " addresses: " << a_addresses << "\n";
        if(a_addresses.size() > 0) maxcc = 1;
        int i = 0;
        for(auto const &aep: f_endpoints) for(int j = 0; j < maxcc; ++j) {
            FLOGC(flowc::trace_connections) << "creating @" << label << " stub " << i << " -> " << aep << "\n";
            std::shared_ptr<::grpc::Channel> channel(::grpc::CreateChannel(aep, ::grpc::InsecureChannelCredentials()));
            stubs.emplace_back(std::make_tuple(0, std::chrono::system_clock::now(), CSERVICE::NewStub(channel), aep, std::string()));
            activity_index.emplace_back(i);
            ++i;
        }
        for(auto const &aep: d_endpoints) {
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
        cc.store(0);
    }
    size_t count() const {
        return stubs.size();
    }
    Stub_t *stub(int &connection_number, long scid, int ccid) {
        auto index = cc.fetch_add(1, std::memory_order_seq_cst);
        auto time_now = std::chrono::system_clock::now();
        if(stubs.size() == 0)  {
            connection_number = -1;
            return dead_end.get();
        }
        std::lock_guard<std::mutex> guard(allocator);
        std::sort(activity_index.begin(), activity_index.end(), [this](int const &x1, int const &x2) -> bool {
            if(std::get<0>(stubs[x1]) != std::get<0>(stubs[x2]))
                return std::get<0>(stubs[x1]) < std::get<0>(stubs[x2]);
            return std::get<0>(stubs[x1]) < std::get<0>(stubs[x2]);
        });
        connection_number = activity_index[0]; 
        auto &stubt = stubs[connection_number];
        FLOGC(flowc::trace_connections) << flowc::callid(scid, ccid) << "using @" << label << " stub[" << connection_number << "] for call #" << index 
            << ", to: " << std::get<3>(stubt) << (std::get<4>(stubt).empty() ? "": "(") << std::get<4>(stubt) << (std::get<4>(stubt).empty() ? "": ")")
            << ", active: " << std::get<0>(stubt) 
            << "\n";
        std::get<1>(stubt) = std::chrono::system_clock::now();
        std::get<0>(stubt) += 1;
        if(flowc::trace_connections) {
            std::stringstream slog;
            slog << "allocation @" << label << " " << stubs.size() << "[";
            for(auto const &s: stubs) slog << " " << std::get<0>(s);
            slog << "]\n";
            FLOG << flowc::callid(scid, ccid) << slog.str();
        }
        return std::get<2>(stubt).get();
    }
    
    void finished(int connection_number, long scid, int ccid, bool in_error) {
        if(connection_number < 0)
            return;
        FLOGC(flowc::trace_connections) << flowc::callid(scid, ccid) << "releasing @" << label << " stub[" << connection_number << "]\n";
        std::lock_guard<std::mutex> guard(allocator);
        auto &stubt = stubs[connection_number];
        std::get<0>(stubt) -= 1;
        if(in_error && flowc::accumulate_addresses && !std::get<4>(stubt).empty()) {
            FLOGC(flowc::trace_connections) << flowc::callid(scid, ccid) << "dropping @" << label << " stub[" << connection_number << "] to: " 
                << std::get<3>(stubt) << "(" << std::get<4>(stubt) <<  ")\n";
            // Mark with a very high count so it won't be allocated anymore
            std::get<0>(stubt) -= 100000;
            casd::remove_address(std::get<4>(stubt));
        }
    }
};

}

{I:SERVER_XTRA_H{#include "{{SERVER_XTRA_H}}"
}I}
#ifndef GRPC_RECEIVED
#define GRPC_RECEIVED(ENTRY_NAME, NODE_NAME, SERVER_CALL_ID, CLIENT_CALL_ID, NODE_ID, STATUS, CONTEXT, RESPONSE_PTR)
#endif
#ifndef GRPC_SENDING
#define GRPC_SENDING(ENTRY_NAME, NODE_NAME, SERVER_CALL_ID, CLIENT_CALL_ID, NODE_ID, CONTEXT, REQUEST_PTR)
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
        if(flowc::{{CLI_NODE_ID}}_dendpoints.size() == 0) {
            return {{CLI_NODE_ID}}_conp;
        }

        std::map<std::string, std::vector<std::string>> addresses = {{CLI_NODE_ID}}_conp->addresses;
        int ver = casd::get_latest_addresses(addresses, {{CLI_NODE_ID}}_nversion,  flowc::{{CLI_NODE_ID}}_dnames);
        if(ver == {{CLI_NODE_ID}}_nversion) 
            return {{CLI_NODE_ID}}_conp;
        {{CLI_NODE_ID}}_nversion = ver;

        FLOGC(flowc::trace_connections) << "new @{{CLI_NODE_NAME}} connector to " << flowc::{{CLI_NODE_ID}}_fendpoints << " " << flowc::{{CLI_NODE_ID}}_dendpoints << "\n"; 
        auto {{CLI_NODE_ID}}_cp = new ::flowc::connector<{{CLI_SERVICE_NAME}}>("{{CLI_NODE_NAME}}", flowc::{{CLI_NODE_ID}}_maxcc, flowc::{{CLI_NODE_ID}}_dnames,  
                    flowc::{{CLI_NODE_ID}}_fendpoints, flowc::{{CLI_NODE_ID}}_dendpoints, 
                    addresses);
        {{CLI_NODE_ID}}_conp.reset({{CLI_NODE_ID}}_cp);
        return {{CLI_NODE_ID}}_conp;
    }
    std::unique_ptr<::grpc::ClientAsyncResponseReader<{{CLI_OUTPUT_TYPE}}>> {{CLI_NODE_ID}}_prep(int &ConN, long CID, int CCid,
            std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> ConP, ::grpc::CompletionQueue &CQ, ::grpc::ClientContext &CTX, {{CLI_INPUT_TYPE}} *A_inp, bool Trace_call) {
        Trace_call = Trace_call || flowc::trace_{{CLI_NODE_ID}};
        FLOGC(Trace_call || flowc::trace_{{CLI_NODE_ID}}) << "[" << CID << ":" << CCid << "] {{CLI_NODE_NAME}} prepare " << flowc::log_abridge(*A_inp) << "\n";
        if(flowc::send_global_ID) {
            CTX.AddMetadata("node-id", flowc::global_node_ID);
            CTX.AddMetadata("start-time", flowc::global_start_time);
        }
        SET_METADATA_{{CLI_NODE_ID}}(CTX)
        GRPC_SENDING("{{ENTRY_NAME}}", "{{CLI_NODE_ID}}", CID, CCid, flowc::{{CLI_NODE_UPPERID}}, CTX, A_inp)

        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds(flowc::{{CLI_NODE_ID}}_timeout);
        CTX.set_deadline(deadline);
        if(ConP->count() == 0) 
            return nullptr;
        return ConP->stub(ConN, CID, CCid)->PrepareAsync{{CLI_METHOD_NAME}}(&CTX, *A_inp, &CQ);
    }
    ::grpc::Status {{CLI_NODE_ID}}_call(long CID, int CCid, std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> ConP, {{CLI_OUTPUT_TYPE}} *A_outp, {{CLI_INPUT_TYPE}} *A_inp, bool Trace_call) {
        Trace_call = Trace_call || flowc::trace_{{CLI_NODE_ID}};
        ::grpc::ClientContext L_context;
        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds(flowc::{{CLI_NODE_ID}}_timeout);
        L_context.set_deadline(deadline);
        if(flowc::send_global_ID) {
            L_context.AddMetadata("node-id", flowc::global_node_ID);
            L_context.AddMetadata("start-time", flowc::global_start_time);
        }
        SET_METADATA_{{CLI_NODE_ID}}(L_context)
        GRPC_SENDING("{{ENTRY_NAME}}", "{{CLI_NODE_ID}}", CID, CCid, flowc::{{CLI_NODE_UPPERID}}, CTX, A_inp)
        ::grpc::Status L_status;
        if(ConP->count() == 0) {
            L_status = ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, ::flowc::sfmt() << "Failed to connect to");
        } else {
            int ConN = -1;
            L_status = ConP->stub(ConN, CID, CCid)->{{CLI_METHOD_NAME}}(&L_context, *A_inp, A_outp);
            ConP->finished(ConN, CID, CCid, L_status.error_code() == grpc::StatusCode::UNAVAILABLE);
            GRPC_RECEIVED("{{ENTRY_NAME}}", "{{CLI_NODE_ID}}", CID, CCid, flowc::{{CLI_NODE_UPPERID}}, L_status, L_context, A_outp)
        }
        FLOGC(Trace_call) << flowc::callid(CID, CCid) << " {{CLI_NODE_NAME}} request: " << flowc::log_abridge(*A_inp) << "\n";
        if(!L_status.ok()) {
            GRPC_ERROR(CID, CCid, "{{CLI_NODE_NAME}} ", L_status, L_context);
        } else {
            FLOGC(Trace_call) << flowc::callid(CID, CCid) << " {{CLI_NODE_NAME}} reply: " << flowc::log_abridge(*A_outp) << "\n";
        }
        return L_status;
    }}I}
    // Constructor
    {{NAME_ID}}_service() {
        Call_Counter = 1;
        {I:CLI_NODE_ID{
        {{CLI_NODE_ID}}_nversion = 0;
        auto {{CLI_NODE_ID}}_cp = new ::flowc::connector<{{CLI_SERVICE_NAME}}>("{{CLI_NODE_NAME}}", flowc::{{CLI_NODE_ID}}_maxcc, flowc::{{CLI_NODE_ID}}_dnames, flowc::{{CLI_NODE_ID}}_fendpoints, flowc::{{CLI_NODE_ID}}_dendpoints, std::map<std::string, std::vector<std::string>>());
        {{CLI_NODE_ID}}_conp.reset({{CLI_NODE_ID}}_cp);
        }I}
    }
{I:ENTRY_CODE{
    // {{ENTRY_NAME}}: {{ENTRY_SERVICE_NAME}}({{ENTRY_INPUT_TYPE}} L_inp, {{ENTRY_OUTPUT_TYPE}} L_outp)
{{ENTRY_CODE}}
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
{I:ENTRY_NAME{ 
long {{ENTRY_NAME}}_entry_timeout = flowc::strtolong(std::getenv("{{NAME_UPPERID}}_{{ENTRY_UPPERID}}_TIMEOUT"), {{ENTRY_TIMEOUT:3600000}});
}I}
static int log_message(const struct mg_connection *conn, const char *message) {
    std::cerr << message << std::flush;
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
               "\"timeout\": " << {{ENTRY_NAME}}_entry_timeout << ","
               "\"input-schema\": " << schema_map.find("/-input/{{ENTRY_NAME}}")->second << "," 
               "\"output-schema\": " << schema_map.find("/-output/{{ENTRY_NAME}}")->second << "" 
               "},"
        }I}
        {I:CLI_NODE_NAME{
          <<   "\"/-node/{{CLI_NODE_NAME}}\": {"
               "\"timeout\": " << flowc::{{CLI_NODE_ID}}_timeout << ","
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

static int get_form_data(struct mg_connection *conn, std::string &data, bool &use_asynchronous_calls, bool &time_call) {
    use_asynchronous_calls = flowc::strtobool(mg_get_header(conn, "x-flow-overlapped-calls"), use_asynchronous_calls);
    time_call = flowc::strtobool(mg_get_header(conn, "x-flow-time-call"), time_call);
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
    
static bool is_protobuf(char const *content_type) {
    return content_type != nullptr && (strcasecmp(content_type, "application/protobuf") == 0 || 
        strcasecmp(content_type, "application/x-protobuf") == 0 || 
        strcasecmp(content_type, "application/vnd.google.protobuf") == 0);
}
}
std::atomic<long> call_counter;
{I:ENTRY_NAME{
static int REST_{{ENTRY_NAME}}_call(long call_id, struct mg_connection *A_conn, void *A_cbdata, bool trace_call) {
    std::string xtra_headers;
    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0)
        return rest::not_found(A_conn, "Resource not found");

    std::shared_ptr<::grpc::Channel> L_channel(::grpc::CreateChannel(rest::gateway_endpoint, ::grpc::InsecureChannelCredentials()));
    std::unique_ptr<{{ENTRY_SERVICE_NAME}}::Stub> L_client_stub = {{ENTRY_SERVICE_NAME}}::NewStub(L_channel);                    
    {{ENTRY_OUTPUT_TYPE}} L_outp; 
    {{ENTRY_INPUT_TYPE}} L_inp;
    std::string L_inp_json;
    bool use_asynchronous_calls = flowc::asynchronous_calls, time_call = false;

    if(rest::get_form_data(A_conn, L_inp_json, use_asynchronous_calls, time_call) <= 0) return rest::bad_request_error(A_conn);

    char const *accept_header = mg_get_header(A_conn, "accept");
    bool return_protobuf = rest::is_protobuf(accept_header);

    FLOG << "[" << call_id << "] info: overlapped, time, trace: " << use_asynchronous_calls << ", "  << time_call << ", " << trace_call 
        << ", reply: " << (return_protobuf? "protobuf": "json") << ", body: " << flowc::log_abridge(L_inp_json) << "\n";

    auto L_conv_status = google::protobuf::util::JsonStringToMessage(L_inp_json, &L_inp);
    if(!L_conv_status.ok()) return rest::conversion_error(A_conn, L_conv_status);

    ::grpc::ClientContext L_context;
    auto const L_start_time = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const L_deadline = L_start_time + std::chrono::milliseconds(rest::{{ENTRY_NAME}}_entry_timeout);
    L_context.set_deadline(L_deadline);
    L_context.AddMetadata("overlapped-calls", use_asynchronous_calls? "1": "0");
    L_context.AddMetadata("time-call", time_call? "1": "0");
    if(trace_call)
        L_context.AddMetadata("trace-call", "1");

#if defined(REST_CHECK_{{ENTRY_UPPERID}}_BEFORE) || defined(REST_CHECK_{{ENTRY_UPPERID}}_AFTER)
    char const *check_header = mg_get_header(A_conn, "x-flow-check");
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
    ::grpc::Status L_status = L_client_stub->{{ENTRY_NAME}}(&L_context, L_inp, &L_outp);
    for(auto const &mde: L_context.GetServerTrailingMetadata()) {
        std::string header(mde.first.data(), mde.first.length());
        if(header == "times-bin") 
            header = "X-Flow-Call-Times";
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
    return return_protobuf? rest::protobuf_reply(A_conn, L_outp, xtra_headers): rest::message_reply(A_conn, L_outp, xtra_headers);
}
static int REST_{{ENTRY_NAME}}_handler(struct mg_connection *A_conn, void *A_cbdata) {
    long call_id = call_counter.fetch_add(1, std::memory_order_seq_cst);
    bool trace_call = flowc::strtobool(mg_get_header(A_conn, "x-flow-trace-call"), flowc::trace_calls);
    FLOG << "[" << call_id << "] REST-entry: " << mg_get_request_info(A_conn)->local_uri << "\n";
    int rc = REST_{{ENTRY_NAME}}_call(call_id, A_conn, A_cbdata, trace_call);
    FLOGC(trace_call) << "[" << call_id << "] REST-return: " << rc << "\n";
    return rc;
}
}I}
{I:CLI_NODE_NAME{
static int REST_node_{{CLI_NODE_ID}}_call(long call_id, struct mg_connection *A_conn, void *A_cbdata, bool trace_call) {
    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0)
        return rest::not_found(A_conn, "Resource not found");

    std::shared_ptr<::flowc::connector<{{CLI_SERVICE_NAME}}>> connector = {{NAME_ID}}_service_ptr->{{CLI_NODE_ID}}_get_connector();

    {{CLI_OUTPUT_TYPE}} L_outp; 
    {{CLI_INPUT_TYPE}} L_inp;
    std::string L_inp_json;
    bool use_asynchronous_calls = flowc::asynchronous_calls, time_call = false;
    if(rest::get_form_data(A_conn, L_inp_json, use_asynchronous_calls, time_call) <= 0) return rest::bad_request_error(A_conn);

    char const *accept_header = mg_get_header(A_conn, "accept");
    bool return_protobuf = rest::is_protobuf(accept_header);
    FLOG << "[" << call_id << "] reply: " << (return_protobuf? "protobuf": "json") << ", body: " << flowc::log_abridge(L_inp_json) << "\n";

    auto L_conv_status = google::protobuf::util::JsonStringToMessage(L_inp_json, &L_inp);
    if(!L_conv_status.ok()) return rest::conversion_error(A_conn, L_conv_status);

    auto const L_start_time = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const L_deadline = L_start_time + std::chrono::milliseconds(flowc::{{CLI_NODE_ID}}_timeout);

    ::grpc::ClientContext L_context;
    L_context.set_deadline(L_deadline);
    SET_METADATA_{{CLI_NODE_ID}}(L_context)

    int connection_n = -1;
    ::grpc::Status L_status = connector->stub(connection_n, call_id, -1)->{{CLI_METHOD_NAME}}(&L_context, L_inp, &L_outp);
    connector->finished(connection_n, call_id, -1, L_status.error_code() == grpc::StatusCode::UNAVAILABLE);

    if(!L_status.ok()) return rest::grpc_error(A_conn, L_context, L_status);
    auto const &metadata = L_context.GetServerTrailingMetadata();
    std::string xtra_headers;
    for(auto const &mde: L_context.GetServerTrailingMetadata()) {
        std::string header(mde.first.data(), mde.first.length());
        if(header == "times-bin") 
            header = "X-Flow-Call-Times";
        else 
            header = std::string("X-Flow-") + header;
        xtra_headers += header;
        xtra_headers += ": ";
        xtra_headers += std::string(mde.second.data(), mde.second.length());
        xtra_headers += "\r\n";
    }
    return return_protobuf? rest::protobuf_reply(A_conn, L_outp, xtra_headers): rest::message_reply(A_conn, L_outp, xtra_headers);
}
static int REST_node_{{CLI_NODE_ID}}_handler(struct mg_connection *A_conn, void *A_cbdata) {
    long call_id = call_counter.fetch_add(1, std::memory_order_seq_cst);
    bool trace_call = flowc::strtobool(mg_get_header(A_conn, "x-flow-trace-call"), flowc::trace_calls);
    FLOG << "[" << call_id << "] REST-node-entry: " << mg_get_request_info(A_conn)->local_uri << "\n";
    int rc = REST_node_{{CLI_NODE_ID}}_call(call_id, A_conn, A_cbdata, trace_call);
    FLOGC(trace_call) << "[" << call_id << "] REST-node-return: " << rc << "\n";
    return rc;
}
}I}
namespace rest {
int start_civetweb(char const *rest_port, int num_threads, bool rest_only) {
    call_counter = 1;
    std::string num_threads_s(std::to_string(num_threads));
    const char *options[] = {
        "document_root", "/dev/null",
        "listening_ports", rest_port,
        "request_timeout_ms", "3600000",
        "error_log_file", "error.log",
        "extra_mime_types", ".flow=text/plain,.proto=text/plain,.svg=image/svg+xml",
        "enable_auth_domain_check", "no",
        "max_request_size", "65536", 
        "num_threads", num_threads_s.c_str(),
        0
    };
	struct mg_callbacks callbacks;
	struct mg_context *ctx;

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.log_message = rest::log_message;
	ctx = mg_start(&callbacks, 0, options);

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
    std::cerr <<  "REST gateway at";
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
        std::cerr << " " << proto << "://" << host << ":" << ports[n].port;
	}
    std::cerr << " running " << num_threads << " threads\n";
    std::cerr << "web app enabled: " << (rest_only? "no": "yes") << "\n";
    return 0;
}
}

int main(int argc, char *argv[]) {
    if(argc < 2 || argc > 4) {
       std::cout << "Usage: " << argv[0] << " GRPC-PORT [REST-PORT [APP-DIRECTORY]] \n\n";
       std::cout << "Endpoints (host:port) for each node:\n";
       {I:CLI_NODE_NAME{std::cout << "{{CLI_NODE_UPPERID}}_ENDPOINT= for node {{CLI_NODE_NAME}}/{{CLI_GRPC_SERVICE_NAME}}.{{CLI_METHOD_NAME}}\n";
       }I}
       std::cout << "\n";
       std::cout << "The maximum number of concurrent calls allowed for each node, or set to 0 to use service discovery:\n";
       {I:CLI_NODE_NAME{std::cout << "{{CLI_NODE_UPPERID}}_MAXCC= for node {{CLI_NODE_NAME}} ("<< flowc::{{CLI_NODE_ID}}_maxcc <<")\n";
       }I}
       std::cout << "\n";
       std::cout << "Timeout used to set deadline for each node, in milliseconds:\n";
       {I:CLI_NODE_NAME{std::cout << "{{CLI_NODE_UPPERID}}_TIMEOUT= for node {{CLI_NODE_NAME}} ("<< flowc::{{CLI_NODE_ID}}_timeout <<")\n";
       }I}
       std::cout << "\n";
       std::cout << "Enable trace for eaach node:\n";
       {I:CLI_NODE_NAME{std::cout << "{{CLI_NODE_UPPERID}}_TRACE=1 to enable tracing for node {{CLI_NODE_NAME}}\n";
       }I}
       std::cout << "\n";
       std::cout << "Set {{NAME_UPPERID}}_REST_PORT= to enable the REST gateway service\n";
       std::cout << "Set {{NAME_UPPERID}}_REST_THREADS= to change the number of REST worker threads (" << DEFAULT_REST_THREADS << ")\n";
       std::cout << "\n";
       std::cout << "Set the timeout for each REST entry, in milliseconds:\n";
       {I:ENTRY_NAME{std::cout << "{{NAME_UPPERID}}_{{ENTRY_UPPERID}}_TIMEOUT= for REST entry /{{ENTRY_NAME}} (" << rest::{{ENTRY_NAME}}_entry_timeout << ")\n";
       }I}
       std::cout << "\n";
       std::cout << "Set {{NAME_UPPERID}}_WEBAPP=0 to disable the web-app when the REST service is enabled\n";
       std::cout << "Set {{NAME_UPPERID}}_TRACE=1 to enable trace mode\n";
       std::cout << "Set {{NAME_UPPERID}}_ASYNC=0 to disable asynchronous client calls\n";
       std::cout << "Set {{NAME_UPPERID}}_NODE_ID= to override the server ID\n"; 
       std::cout << "Set {{NAME_UPPERID}}_SEND_ID=0 to disable sending the server ID\n"; 
       std::cout << "Set {{NAME_UPPERID}}_CARES_REFRESH= to the number of seconds between DNS lookups (" << DEFAULT_CARES_REFRESH << ")\n"; 
       std::cout << "Set {{NAME_UPPERID}}_GRPC_THREADS= to change the number of gRPC threads, leave 0 for no change (" << DEFAULT_GRPC_THREADS << ")\n";
       std::cout << "\n";
       return 1;
    }
    std::cerr 
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
    // Use the default grpc health checking service
	grpc::EnableDefaultHealthCheckService(true);
    int error_count = 0;
    std::set<std::string> dnames;
    {   
        {I:CLI_NODE_ID{
        char const *{{CLI_NODE_ID}}_epenv = std::getenv("{{CLI_NODE_UPPERID}}_ENDPOINT");

        if({{CLI_NODE_ID}}_epenv == nullptr || !casd::parse_endpoint_list(flowc::{{CLI_NODE_ID}}_dnames, flowc::{{CLI_NODE_ID}}_dendpoints, flowc::{{CLI_NODE_ID}}_fendpoints, {{CLI_NODE_ID}}_epenv)) {
            std::cerr << "Endpoint environment variable ({{CLI_NODE_UPPERID}}_ENDPOINT) not set ior invalid for node {{CLI_NODE_NAME}}\n";
            ++error_count;
        } else {
            dnames.insert(flowc::{{CLI_NODE_ID}}_dnames.begin(), flowc::{{CLI_NODE_ID}}_dnames.end());
            std::cerr << "{{CLI_NODE_ID}}: timeout " << flowc::{{CLI_NODE_ID}}_timeout << "ms";
            if(0 < flowc::{{CLI_NODE_ID}}_fendpoints.size()) std::cerr << ", fixed " << flowc::{{CLI_NODE_ID}}_fendpoints;
            if(0 < flowc::{{CLI_NODE_ID}}_dendpoints.size()) std::cerr << ", auto " << flowc::{{CLI_NODE_ID}}_dendpoints;
            std::cerr << "\n";
        }
        }I}
    }
    if(error_count != 0) return 1;
    // Initialize c-ares
    int status;
    status = ares_library_init(ARES_LIB_INIT_ALL);
    if(status != ARES_SUCCESS) {
        std::cerr << "ares_library_init: " << ares_strerror(status) << "\n";
        return 1;
    }
    int cares_refresh = (int) flowc::strtolong(std::getenv("{{NAME_UPPERID}}_CARES_REFRESH"), DEFAULT_CARES_REFRESH);
    // Start c-ares thread
    std::cerr << "lookup thread: every " << cares_refresh << "s, lookig for " << dnames << "\n";
    std::thread cares_thread([&dnames, cares_refresh]{
         casd::keep_looking(dnames, cares_refresh, 0);
    });

    int grpc_threads = (int) flowc::strtolong(std::getenv("{{NAME_UPPERID}}_GRPC_THREADS"), DEFAULT_GRPC_THREADS);

    int listening_port;
    grpc::ServerBuilder builder;
    if(grpc_threads > 0) {
        grpc::ResourceQuota grq("{{NAME_UPPERID}}");
        grq.SetMaxThreads(grpc_threads);
        builder.SetResourceQuota(grq);
        std::cerr << "max gRPC threads: " << grpc_threads << "\n";
    }

    {{NAME_ID}}_service service;
    {{NAME_ID}}_service_ptr = &service;
    bool enable_webapp = flowc::strtobool(std::getenv("{{NAME_UPPERID}}_WEBAPP"), true);
    if(argc > 3) 
        rest::app_directory = argv[3];

    // Listen on the given address without any authentication mechanism.
    std::string server_address("[::]:"); server_address += argv[1];
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &listening_port);

    // Register services
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    if(listening_port == 0) {
        std::cerr << "failed to start {{NAME}} gRPC service at " << listening_port << "\n";
        return 1;
    }
    std::cerr << "node id: " << flowc::global_node_ID << "\n";
    std::cerr << "start time: " << flowc::global_start_time << "\n";
    rest::gateway_endpoint = flowc::sfmt() << "localhost:" << listening_port;
    std::cerr << "gRPC service {{NAME}} listening on port: " << listening_port << "\n";

    // Set up the REST gateway if enabled
    char const *rest_port = std::getenv("{{NAME_UPPERID}}_REST_PORT");
    if(rest_port == nullptr || argc >= 3) rest_port = argv[2];
    if(rest_port != nullptr && strspn("\t\r\n ", rest_port) < strlen(rest_port)) {
        long num_rest_threads = flowc::strtolong(std::getenv("{{NAME_UPPERID}}_REST_THREADS"), DEFAULT_REST_THREADS);
        if(rest::start_civetweb(rest_port, (int) num_rest_threads, !enable_webapp) != 0) {
            std::cerr << "Failed to start REST gateway service\n";
            return 1;
        }
    }
    std::cerr 
        << "call id: " << (flowc::send_global_ID ? "yes": "no") 
        << ", trace: " << (flowc::trace_calls? "yes": "no")
        << ", asynchronous client calls: " << (flowc::asynchronous_calls? "yes": "no") 
        << "\n";

    std::cerr << std::endl;
    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();

    cares_thread.join();
    ares_library_cleanup();
    return 0;
}
