/************************************************************************************************************
 *
 * {{NAME}}-server.C 
 * generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <ratio>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <grpc++/grpc++.h>
#include <grpc++/health_check_service_interface.h>
#include <google/protobuf/util/json_util.h>
extern "C" {
#include <civetweb.h>
}
#ifndef MAX_REST_REQUEST_SZIE
/** Size limit for the  REST request **/
#define MAX_REST_REQUEST_SIZE 1024ul*1024ul*100ul
#endif

bool Global_Debug_Enabled = false;
bool Global_Asynchronous_Calls = true;
bool Global_Trace_Calls_Enabled = false;

template <class S>
static inline bool stringtobool(S s, bool default_value=false) {
    if(s.empty()) return default_value;
    std::string so(s.length(), ' ');
    std::transform(s.begin(), s.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(so.c_str()) != 0;
}
static inline bool strtobool(char const *s, bool default_value=false) {
    if(s == nullptr || *s == '\0') return default_value;
    std::string so(s);
    std::transform(so.begin(), so.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(s) != 0;
}
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
static std::string Log_abridge(std::string const &message, unsigned max_length=256) {
    if(max_length == 0 || message.length() <= max_length) return message;
    return message.substr(0, (max_length - 5)/2) + " ... " + message.substr(message.length()-(max_length-5)/2);
}
static std::string Log_abridge(google::protobuf::Message const &message, unsigned max_length=256) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = false;
    options.always_print_primitive_fields = false;
    options.preserve_proto_field_names = true;
    std::string json_reply;
    google::protobuf::util::MessageToJsonString(message, &json_reply, options);
    return Log_abridge(json_reply, max_length);
}
inline std::ostream &operator << (std::ostream &out, std::chrono::steady_clock::duration time_diff) {
    auto td = double(time_diff.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
    char const *unit = "s";
    if(td < 1.0) { td *= 1000; unit = "ms"; }
    if(td < 1.0) { td *= 1000; unit = "us"; }
    if(td < 1.0) { td *= 1000; unit = "ns"; }
    if(td < 1.0) { td = 0; unit = ""; }
    return out << td << unit;
}
class sfmt {
    std::ostringstream os;
public:
    inline operator std::string() {
        return os.str();
    }
    template <class T> sfmt &operator <<(T v) {
        os << v; return *this;
    }
    sfmt &message(std::string const &message, long cid, int call, google::protobuf::Message const *msgp=nullptr) {
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        auto timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char cstr[128];
        std::strftime(cstr, sizeof(cstr), "%a %F %T %Z (%z)", timeinfo);
        os << cstr;
        os << " [" << cid;
        if(call >= 0) os << ": " << call;
        os << "] " << message;
        if(msgp != nullptr) os << "\n" <<  Log_abridge(*msgp);
        os << "\n";
        return *this;
    }
};
std::mutex global_display_mutex;
{I:GRPC_GENERATED_H{#include "{{GRPC_GENERATED_H}}"
}I}
/* Client endpoints - initialzied in main from environment variables {CLIENT_NODE_ID}_ENDPOINT
 */
{I:CLI_NODE_ID{std::string {{CLI_NODE_ID}}_endpoint;
}I}
template <class C>
static bool Get_metadata_bool(C mm, std::string const &key, bool default_value=false) { 
    auto vp = mm.find(key);
    if(vp == mm.end()) return default_value;
    return stringtobool(std::string((vp->second).data(), (vp->second).length()), default_value);
}
static void Print_message(std::string const &message, bool finish=false) {
    global_display_mutex.lock();
    std::cerr << message << std::flush;
    global_display_mutex.unlock();
}
static std::string Grpc_error(long cid, int call, char const *message, grpc::Status &status, grpc::ClientContext &context, google::protobuf::Message const *reqp=nullptr, google::protobuf::Message const *repp=nullptr) {
    sfmt out;
    out.message(sfmt() << (message == nullptr? "": message) << "error " << status.error_code(), cid, call, reqp);
    out << "context info: " << context.debug_error_string() << "\n";
    if(repp != nullptr) std::cerr << "the response was: " << Log_abridge(*repp) << "\n";
    return out;
}

#define TIME_INFO_BEGIN(enabled)  std::stringstream Time_info; if(enabled) Time_info << "[";
#define TIME_INFO_GET(enabled)    ((Time_info << "]"), Time_info.str())
#define TIME_INFO_END(enabled)

static void Record_time_info(std::ostream &out, int stage, std::string const &method_name, std::string const &stage_name,
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
#define TRACECMF(c, n, text, messagep, finish) if(Trace_call && (c)) { Print_message(sfmt().message(text, CID, n, messagep), finish); } 
#define TRACECM(c, n, text, messagep) TRACECMF((c), (n), (text), (messagep), false) 
#define TRACE(c, text, messagep) TRACECM((c), -1, (text), (messagep)) 
#define TRACEA(text, messagep) TRACECM(true, -1, (text), (messagep))
#define TRACEAF(text, messagep) TRACECMF(true, -1, (text), (messagep), true)
#define TRACEC(n, text) TRACECM(true, (n), (text), nullptr)
#define ERROR(text) {Print_message(sfmt().message((text), CID, -1, nullptr), true); } 
#define GRPC_ERROR(n, text, status, context, reqp, repp) Print_message(Grpc_error(CID, (n), (text), (status), (context), (reqp), (repp)), true);
#define PTIME2(method, stage, stage_name, call_elapsed_time, stage_duration, calls) {\
    if(Time_call) Record_time_info(Time_info, stage, method, stage_name, (call_elapsed_time), (stage_duration), calls);\
    if(Debug_Flag||Trace_call) Print_message(sfmt().message(\
        sfmt() << "time-call " << Time_call << ": " << method << " stage " << stage << " (" << stage_name << ") started after " << call_elapsed_time << " and took " << stage_duration << " for " << calls << " call(s)", \
        CID, -1, nullptr)); }

class {{NAME_ID}}_service final: public {{CPP_SERVER_BASE}}::Service {
public:
    bool Debug_Flag = false;
    bool Async_Flag;
    std::atomic<long> Call_Counter;

{I:CLI_NODE_NAME{
    /* {{CLI_NODE_NAME}} line {{CLI_NODE_LINE}}
     */

#define SET_METADATA_{{CLI_NODE_ID}}(context) {{CLI_NODE_METADATA}}

    int const {{CLI_NODE_ID}}_maxcc = {{CLI_NODE_MAX_CONCURRENT_CALLS}};
    std::unique_ptr<{{CLI_SERVICE_NAME}}::Stub> {{CLI_NODE_ID}}_stub[{{CLI_NODE_MAX_CONCURRENT_CALLS}}];
    std::unique_ptr<::grpc::ClientAsyncResponseReader<{{CLI_OUTPUT_TYPE}}>> {{CLI_NODE_ID}}_prep(long CID, int call_number, ::grpc::CompletionQueue &CQ, ::grpc::ClientContext &CTX, {{CLI_INPUT_TYPE}} *A_inp, bool Debug_Flag, bool Trace_call) {
        TRACECM(true, call_number, "{{CLI_NODE_NAME}} prepare request: ", A_inp);
        SET_METADATA_{{CLI_NODE_ID}}(CTX)
        auto result = {{CLI_NODE_ID}}_stub[call_number % {{CLI_NODE_ID}}_maxcc]->PrepareAsync{{CLI_METHOD_NAME}}(&CTX, *A_inp, &CQ);
        return result;
    }

    ::grpc::Status {{CLI_NODE_ID}}_call(long CID, {{CLI_OUTPUT_TYPE}} *A_outp, {{CLI_INPUT_TYPE}} *A_inp, bool Debug_Flag, bool Trace_call) {
        ::grpc::ClientContext L_context;
        /**
         * Some notes on error codes (from https://grpc.io/grpc/cpp/classgrpc_1_1_status.html):
         * UNAUTHENTICATED - The request does not have valid authentication credentials for the operation.
         * PERMISSION_DENIED - Must not be used for rejections caused by exhausting some resource (use RESOURCE_EXHAUSTED instead for those errors). 
         *                     Must not be used if the caller can not be identified (use UNAUTHENTICATED instead for those errors).
         * INVALID_ARGUMENT - Client specified an invalid argument. 
         * FAILED_PRECONDITION - Operation was rejected because the system is not in a state required for the operation's execution.
         */
        auto const start_time = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point const deadline = start_time + std::chrono::milliseconds({{CLI_NODE_TIMEOUT:120000}});
        L_context.set_deadline(deadline);
        SET_METADATA_{{CLI_NODE_ID}}(L_context)
        ::grpc::Status L_status = {{CLI_NODE_ID}}_stub[0]->{{CLI_METHOD_NAME}}(&L_context, *A_inp, A_outp);
        if(!L_status.ok()) {
            GRPC_ERROR(-1, "{{CLI_NODE_NAME}}", L_status, L_context, A_inp, nullptr);
        } else {
            TRACE(true, "{{CLI_NODE_NAME}} request: ", A_inp);
            TRACE(true, "{{CLI_NODE_NAME}} reply: ", A_outp);
        }
        return L_status;
    }}I}
    // Constructor
    {{NAME_ID}}_service(bool async_a{I:CLI_NODE_ID{, std::string const &{{CLI_NODE_ID}}_endpoint}I}): Async_Flag(async_a) {
        Call_Counter.store(1);   
        
        {I:CLI_NODE_ID{for(int i = 0; i < {{CLI_NODE_ID}}_maxcc; ++i) {
            std::shared_ptr<::grpc::Channel> {{CLI_NODE_ID}}_channel(::grpc::CreateChannel({{CLI_NODE_ID}}_endpoint, ::grpc::InsecureChannelCredentials()));
            {{CLI_NODE_ID}}_stub[i] = {{CLI_SERVICE_NAME}}::NewStub({{CLI_NODE_ID}}_channel);                    
        }
        }I}
    }
{I:ENTRY_CODE{{{ENTRY_CODE}}
}I}
};

#define FLOGC(c) if(c) ::rest::flog() <<= sfmt()
#define FLOG FLOGC(true)

namespace rest {
class flog {
public:
    flog &operator <<= (std::string const &v) {
        global_display_mutex.lock();
        std::cerr << v << std::flush; 
        global_display_mutex.unlock();
        return *this;
    }
};

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
    std::cerr << message << std::flush;
	return 1;
}
static int not_found(struct mg_connection *conn, std::string const &message) {
    std::string j_message = sfmt() << "{"
        << "\"code\": 404, \"message\":" << json_string(message) << "}";
       
	mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
              "Content-Type: application/json\r\n"
              "Content-Length: %lu\r\n"
              "\r\n", j_message.length());
	mg_printf(conn, "%s", j_message.c_str());
    return 1;
}
static int json_reply(struct mg_connection *conn, int code, char const *msg, char const *content, size_t length=0, char const *xtra_headers=nullptr) {
    if(xtra_headers == nullptr) xtra_headers = "";
	mg_printf(conn, "HTTP/1.1 %d %s\r\n"
              "Content-Type: application/json\r\n"
              "%s"
              "Content-Length: %lu\r\n"
              "\r\n", code, msg, xtra_headers, length == 0? strlen(content): length);
	mg_printf(conn, "%s", content);
	return 1;
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
    return 1;
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
static int grpc_error(struct mg_connection *conn, ::grpc::ClientContext const &context, ::grpc::Status const &status) {
    std::string errm = sfmt() 
        << "{" 
        << "\"code\": 500,"
        << "\"from\":" << json_string(context.peer()) << ","
        << "\"grpc-code\":" << status.error_code() << ","
        << "\"message\":" << json_string(status.error_message())
        << "}";
    return json_reply(conn, 500, "gRPC Error", errm.c_str(), errm.length());
}
static int conversion_error(struct mg_connection *conn, google::protobuf::util::Status const &status) {
    std::string error_message = sfmt() << "{"
        << "\"code\": 400,"
        << "\"message\": \"Input failed conversion to protobuf\","
        << "\"description\":" <<json_string(status.ToString())
        << "}";
    return json_reply(conn, 400, "Bad Request", error_message.c_str(), error_message.length());
}
static int bad_request_error(struct mg_connection *conn) {
    std::string error_message = sfmt() << "{"
        << "\"code\": 422,"
        << "\"message\": \"Unprocessable Entity\""
        << "}";
    return json_reply(conn, 422, "Unprocessable Entity", error_message.c_str(), error_message.length());
}
static int get_info(struct mg_connection *conn, void *cbdata) {
    std::string info = sfmt() << "{"
        {I:ENTRY_NAME{
            << "\"/{{ENTRY_NAME}}\": {"
               "\"timeout\": {{ENTRY_TIMEOUT:120000}},"
               "\"input-schema\": " << schema_map.find("/-input/{{ENTRY_NAME}}")->second << "," 
               "\"output-schema\": " << schema_map.find("/-output/{{ENTRY_NAME}}")->second << "" 
               "},"
        }I}
        {I:CLI_NODE_NAME{
          <<   "\"/-node/{{CLI_NODE_NAME}}\": {"
               "\"timeout\": {{CLI_NODE_TIMEOUT:120000}},"
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
    use_asynchronous_calls = strtobool(mg_get_header(conn, "x-flow-overlapped-calls"), use_asynchronous_calls);
    time_call = strtobool(mg_get_header(conn, "x-flow-time-call"), time_call);
    char const *content_type = mg_get_header(conn, "Content-Type");
    /** Default content type is application/json
     */
    if(content_type == nullptr || strcasecmp(content_type, "application/json") == 0) {
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
            data += json_string(nv.first.substr(0, nv.first.length()-2));
        else 
            data += json_string(nv.first);
        data += ":";
        data += json_string(nv.second);
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
            FLOGC(Global_Trace_Calls_Enabled) << "sending " << common+1 << " from " << dir << "\n";
            mg_send_file(conn, filename.c_str());
            return 1;
        }
    } else if(strcmp(local_uri, "/-docs") == 0) {
        FLOGC(Global_Trace_Calls_Enabled) << "list -docs contents\n";
    }
    FLOG << "get \"" << local_uri << "\" not found...\n";
    return not_found(conn, "File not found");
}
static int root_handler(struct mg_connection *conn, void *cbdata) {
    bool rest_only = (bool) cbdata;
    char const *local_uri = mg_get_request_info(conn)->local_uri;
    if(rest_only || strcmp(local_uri, "/") != 0) 
        return not_found(conn, sfmt() << "Resource not found");
    
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
{I:ENTRY_NAME{
static int REST_{{ENTRY_NAME}}_handler(struct mg_connection *A_conn, void *A_cbdata) {
    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0)
        return rest::not_found(A_conn, "Resource not found");

    std::shared_ptr<::grpc::Channel> L_channel(::grpc::CreateChannel(rest::gateway_endpoint, ::grpc::InsecureChannelCredentials()));
    std::unique_ptr<{{ENTRY_SERVICE_NAME}}::Stub> L_client_stub = {{ENTRY_SERVICE_NAME}}::NewStub(L_channel);                    
    {{ENTRY_OUTPUT_TYPE}} L_outp; 
    {{ENTRY_INPUT_TYPE}} L_inp;
    std::string L_inp_json;
    bool use_asynchronous_calls = Global_Asynchronous_Calls, time_call = false;

    if(rest::get_form_data(A_conn, L_inp_json, use_asynchronous_calls, time_call) <= 0) return rest::bad_request_error(A_conn);

    char const *trace_header = mg_get_header(A_conn, "x-flow-trace-call");
    bool trace_call = strtobool(trace_header, Global_Trace_Calls_Enabled);
    FLOG << "rest: " << mg_get_request_info(A_conn)->local_uri << " [overlapped, time, trace: " << use_asynchronous_calls << ", "  << time_call << ", " << trace_call << "]\n" 
        << Log_abridge(L_inp_json, trace_call? 0: 256) << "\n";

    char const *accept_header = mg_get_header(A_conn, "accept");
    bool return_protobuf = rest::is_protobuf(accept_header);
    FLOG << "accept: " << (accept_header? "null": accept_header) << "\n";

    auto L_conv_status = google::protobuf::util::JsonStringToMessage(L_inp_json, &L_inp);
    if(!L_conv_status.ok()) return rest::conversion_error(A_conn, L_conv_status);

    ::grpc::ClientContext L_context;
    auto const L_start_time = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const L_deadline = L_start_time + std::chrono::milliseconds({{ENTRY_TIMEOUT:120000}});
    L_context.set_deadline(L_deadline);
    L_context.AddMetadata("overlapped-calls", use_asynchronous_calls? "1": "0");
    L_context.AddMetadata("time-call", time_call? "1": "0");
    if(trace_header != nullptr && *trace_header != '\0')
        L_context.AddMetadata("trace-call", trace_header);

    ::grpc::Status L_status = L_client_stub->{{ENTRY_NAME}}(&L_context, L_inp, &L_outp);
    if(!L_status.ok()) return rest::grpc_error(A_conn, L_context, L_status);
    if(time_call) {
        auto const &metadata = L_context.GetServerTrailingMetadata();
        auto tbmp = metadata.find("times-bin");
        if(tbmp != metadata.end()) {
            std::string tb((tbmp->second).data(), (tbmp->second).length());
            return return_protobuf?
                rest::protobuf_reply(A_conn, L_outp, std::string("x-flow-call-times: ") + tb + "\r\n"):
                rest::message_reply(A_conn, L_outp, std::string("x-flow-call-times: ") + tb + "\r\n");
        }
    }
    return return_protobuf? rest::protobuf_reply(A_conn, L_outp): rest::message_reply(A_conn, L_outp);
}
}I}
{I:CLI_NODE_NAME{
static int REST_node_{{CLI_NODE_ID}}_handler(struct mg_connection *A_conn, void *A_cbdata) {
    if(strcmp(mg_get_request_info(A_conn)->local_uri, (char const *)A_cbdata) != 0)
        return rest::not_found(A_conn, "Resource not found");

    std::shared_ptr<::grpc::Channel> L_channel(::grpc::CreateChannel({{CLI_NODE_ID}}_endpoint, ::grpc::InsecureChannelCredentials()));
    std::unique_ptr<{{CLI_SERVICE_NAME}}::Stub> L_client_stub = {{CLI_SERVICE_NAME}}::NewStub(L_channel);                    
    {{CLI_OUTPUT_TYPE}} L_outp; 
    {{CLI_INPUT_TYPE}} L_inp;
    std::string L_inp_json;
    bool use_asynchronous_calls = Global_Asynchronous_Calls, time_call = false;

    if(rest::get_form_data(A_conn, L_inp_json, use_asynchronous_calls, time_call) <= 0) return rest::bad_request_error(A_conn);
    auto trace_header = mg_get_header(A_conn, "x-flow-trace-call");
    bool trace_call = strtobool(trace_header, Global_Trace_Calls_Enabled);

    FLOG << "rest: " << mg_get_request_info(A_conn)->local_uri << "\n" << Log_abridge(L_inp_json, trace_call? 0: 256) << "\n";
    char const *accept_header = mg_get_header(A_conn, "accept");
    bool return_protobuf = rest::is_protobuf(accept_header);
    FLOG << "accept: " << (accept_header? "null": accept_header) << "\n";

    auto L_conv_status = google::protobuf::util::JsonStringToMessage(L_inp_json, &L_inp);
    if(!L_conv_status.ok()) return rest::conversion_error(A_conn, L_conv_status);
    ::grpc::ClientContext L_context;
    auto const L_start_time = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const L_deadline = L_start_time + std::chrono::milliseconds({{CLI_NODE_TIMEOUT:120000}});
    L_context.set_deadline(L_deadline);
    SET_METADATA_{{CLI_NODE_ID}}(L_context)
    ::grpc::Status L_status = L_client_stub->{{CLI_METHOD_NAME}}(&L_context, L_inp, &L_outp);
    if(!L_status.ok()) return rest::grpc_error(A_conn, L_context, L_status);
    return return_protobuf? rest::protobuf_reply(A_conn, L_outp): rest::message_reply(A_conn, L_outp);
}
}I}
namespace rest {
int start_civetweb(char const *rest_port, bool rest_only=false) {
    const char *options[] = {
        "document_root", "/dev/null",
        "listening_ports", rest_port,
        "request_timeout_ms", "3600000",
        "error_log_file", "error.log",
        "extra_mime_types", ".flow=text/plain,.proto=text/plain,.svg=image/svg+xml",
        "enable_auth_domain_check", "no",
        "max_request_size", "65536", 
        "num_threads", "12",
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
    std::cerr <<  "REST gateway at:";
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
    std::cerr << "\n";
    return 0;
}
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
       std::cout << "Usage: " << argv[0] << " GRPC-PORT\n\n";
       std::cout << "Set the ENDPOINT variable with the host:port for each node:\n";
       {I:CLI_NODE_NAME{std::cout << "{{CLI_NODE_UPPERID}}_ENDPOINT for node {{CLI_NODE_NAME}} ({{CLI_GRPC_SERVICE_NAME}}.{{CLI_METHOD_NAME}})\n";
       }I}
       std::cout << "\n";
       std::cout << "Set {{NAME_UPPERID}}_REST_PORT= to disable the REST gateway service ({{REST_NODE_PORT}})\n";
       std::cout << "\n";
       std::cout << "Set {{NAME_UPPERID}}_DEBUG=1 to enable debug mode\n";
       std::cout << "Set {{NAME_UPPERID}}_TRACE=1 to enable trace mode\n";
       std::cout << "Set {{NAME_UPPERID}}_ASYNC=0 to disable asynchronous client calls\n";
       std::cout << "Set {{NAME_UPPERID}}_LOGGER=0 to disable the logger service\n";
       std::cout << "\n";
       return 1;
    }
    // Use the default grpc health checking service
	grpc::EnableDefaultHealthCheckService(true);
    int error_count = 0;

    {I:CLI_NODE_NAME{char const *{{CLI_NODE_ID}}_ep = std::getenv("{{CLI_NODE_UPPERID}}_ENDPOINT");
    if({{CLI_NODE_ID}}_ep == nullptr) {
        std::cout << "Endpoint environment variable ({{CLI_NODE_UPPERID}}_ENDPOINT) not set for node {{CLI_NODE_NAME}}\n";
        ++error_count;
    }
    }I}
    if(error_count != 0) return 1;

    {I:CLI_NODE_ID{{{CLI_NODE_ID}}_endpoint = strchr({{CLI_NODE_ID}}_ep, ':') == nullptr? (std::string("localhost:")+{{CLI_NODE_ID}}_ep): std::string({{CLI_NODE_ID}}_ep);
    }I}
    int listening_port;
    grpc::ServerBuilder builder;

    Global_Asynchronous_Calls = strtobool(std::getenv("{{NAME_UPPERID}}_ASYNC"), true);
    {{NAME_ID}}_service service(Global_Asynchronous_Calls{I:CLI_NODE_ID{, {{CLI_NODE_ID}}_endpoint}I});
    Global_Debug_Enabled = service.Debug_Flag = strtobool(std::getenv("{{NAME_UPPERID}}_DEBUG"));
    Global_Trace_Calls_Enabled = strtobool(std::getenv("{{NAME_UPPERID}}_TRACE"));

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
    rest::gateway_endpoint = sfmt() << "localhost:" << listening_port;
    std::cerr << "gRPC service {{NAME}} listening on port: " << listening_port << "\n";

    // Set up the REST gateway if enabled
    char const *rest_port = std::getenv("{{NAME_UPPERID}}_REST_PORT");
    if(rest_port != nullptr && strspn("\t\r\n ", rest_port) < strlen(rest_port)) {
        if(rest::start_civetweb(rest_port) != 0) {
            std::cerr << "Failed to start REST gateway service\n";
            return 1;
        }
    }
    std::cerr 
        << "debug: " << (Global_Debug_Enabled? "yes": "no")
        << ", trace: " << (Global_Trace_Calls_Enabled? "yes": "no")
        << ", asynchronous client calls: " << (Global_Asynchronous_Calls? "yes": "no") 
        << "\n";

    std::cerr 
        << "from {{INPUT_FILE}} ({{MAIN_FILE_TS}})\n" 
        << "{{FLOWC_NAME}} {{FLOWC_VERSION}} ({{FLOWC_BUILD}})\n"
        <<  "grpc " << grpc::Version() << "\n"
#if defined(__clang__)          
        << "clang++ " << __clang_version__ << "\n"
#elif defined(__GNUC__) 
        << "g++ " << __VERSION__ << "\n"
#else
#endif
        << std::endl;
    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
    return 0;
}
