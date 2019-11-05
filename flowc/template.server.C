/************************************************************************************************************
 *
 * {{NAME}}-server.C 
 * generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
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
static std::string Message_to_json(google::protobuf::Message const &message, int max_length=1024) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    std::string json_reply;
    google::protobuf::util::MessageToJsonString(message, &json_reply, options);
    if(json_reply.length() > max_length) 
        return json_reply.substr(0, (max_length-5)/2) + " ... " + json_reply.substr(json_reply.length()-(max_length-5)/2);
    
    return json_reply;
}
inline std::ostream &operator << (std::ostream &out, std::chrono::steady_clock::duration time_diff) {
    auto td = double(time_diff.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
    char const *unit = "s";
    if(td < 1.0) { td *= 1000; unit = "ms"; }
    if(td < 1.0) { td *= 1000; unit = "us"; }
    if(td < 1.0) { td *= 1000; unit = "ns"; }
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
        if(msgp != nullptr) os << "\n" <<  Message_to_json(*msgp);
        os << "\n";
        return *this;
    }
};
#define MAX_LOG_WAIT_SECONDS 600
// 60*60*24*7
#define MAX_LOG_LIFE_SECONDS 1200 
struct Logbuffer_info {
    std::string call_id;
    std::string call_info;
    long expires;
    std::vector<std::string> buffer;
    std::mutex mtx;
    std::condition_variable cv;
    bool finished;

    Logbuffer_info(std::string const &cid, std::chrono::time_point<std::chrono::steady_clock> st): 
        call_id(cid), 
        expires(std::chrono::time_point_cast<std::chrono::seconds>(st+std::chrono::duration<int>(MAX_LOG_LIFE_SECONDS)).time_since_epoch().count()), 
        finished(false) {

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
static bool Get_metadata_value(C mm, std::string const &key, bool default_value=false) { 
    auto vp = mm.find(key);
    if(vp == mm.end()) return default_value;
    return stringtobool(vp->second, default_value);
}

class Flow_logger_service final: public FlowService::Service {
    std::mutex buffer_store_mutex;
    std::map<std::string, std::shared_ptr<Logbuffer_info>> buffers;
public:
    ::grpc::Status log(::grpc::ServerContext *context, const ::FlowCallInfo *request, ::grpc::ServerWriter<::FlowLogMessage> *writer) override {
        std::string call_id(request->callid());
        std::shared_ptr<Logbuffer_info> bufinfop = Get_buffer(call_id);

        if(!bufinfop) 
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, sfmt() << "Invalid or expired call id: \"" << call_id << "\"");
        /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^begin streaming log for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }
        */
        
        unsigned curpos = 0;
        bool finished = false;
        { 
            std::unique_lock<std::mutex> lck(bufinfop->mtx);
            while(!finished) {
                finished = bufinfop->finished;
                while(curpos < bufinfop->buffer.size()) {
                    ::FlowLogMessage msg;
                    msg.set_message(bufinfop->buffer[curpos++]);
                    writer->Write(msg);
                }
                if(finished) break;
                /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^waiting to stream for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }
        */
                int wait_count = MAX_LOG_WAIT_SECONDS/2;
                std::cv_status status;
                do {
                    if(wait_count-- == 0) 
                        return ::grpc::Status(::grpc::StatusCode::CANCELLED, sfmt() << "Wait for log message excedeed " << MAX_LOG_WAIT_SECONDS << " seconds");
                    if(context->IsCancelled()) 
                        return ::grpc::Status(::grpc::StatusCode::CANCELLED, "Call exceeded deadline or was cancelled by the client");

                    status = bufinfop->cv.wait_until(lck, std::chrono::system_clock::now()+std::chrono::seconds(2));
                } while(status == std::cv_status::timeout);
            }
        }
        /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^finished streaming for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }
        */
        // garbage collect the log queues
        long now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count(); 
        {
            std::unique_lock<std::mutex> lck(buffer_store_mutex);
            auto p = buffers.begin();
            while(p != buffers.end()) {
                auto c = p; ++p; 
                if(c->second->expires > now) continue;
                buffers.erase(c);
            }
        }
        /*
        {
            global_display_mutex.lock();
            std::cerr << "^^^done for " << call_id << "\n" << std::flush;
            global_display_mutex.unlock();
        }*/
        return ::grpc::Status::OK;
    }
    std::shared_ptr<Logbuffer_info> New_call(std::string const &call_id, std::chrono::time_point<std::chrono::steady_clock> started) {
        std::unique_lock<std::mutex> lck(buffer_store_mutex);
        auto f = buffers.emplace(call_id, std::make_shared<Logbuffer_info>(call_id, started));
        auto r = f.first->second;
        return r;
    }
    std::shared_ptr<Logbuffer_info> Get_buffer(std::string const &call_id) {
        std::unique_lock<std::mutex> lck(buffer_store_mutex);
        auto f = buffers.find(call_id);
        if(f != buffers.end()) return f->second;
        return nullptr;
    }
};

bool Flow_logger_service_enabled;
static Flow_logger_service Flow_logger;
static void Print_message(std::shared_ptr<Logbuffer_info> lbip,  std::string const &message, bool finish=false) {
    if(Flow_logger_service_enabled && lbip) {
        std::unique_lock<std::mutex> lck(lbip->mtx);
        lbip->buffer.push_back(message);
        if(finish) lbip->finished = true;
        lbip->cv.notify_all();
    }
    global_display_mutex.lock();
    std::cerr << message << std::flush;
    global_display_mutex.unlock();
}
static std::string Grpc_error(long cid, int call, char const *message, grpc::Status &status, grpc::ClientContext &context, google::protobuf::Message const *reqp=nullptr, google::protobuf::Message const *repp=nullptr) {
    sfmt out;
    out.message(sfmt() << (message == nullptr? "": message) << "error " << status.error_code(), cid, call, reqp);
    out << "context info: " << context.debug_error_string() << "\n";
    if(repp != nullptr) std::cerr << "the response was: " << Message_to_json(*repp) << "\n";
    return out;
}
#define TRACECMF(c, n, text, messagep, finish) if((Trace_Flag || Trace_call) && (c)) { Print_message(LOGBIP, sfmt().message(text, CID, n, messagep), finish); } 
#define TRACECM(c, n, text, messagep) TRACECMF((c), (n), (text), (messagep), false) 
#define TRACE(c, text, messagep) TRACECM((c), -1, (text), (messagep)) 
#define TRACEA(text, messagep) TRACECM(true, -1, (text), (messagep))
#define TRACEAF(text, messagep) TRACECMF(true, -1, (text), (messagep), true)
#define TRACEC(n, text) TRACECM(true, (n), (text), nullptr)
#define ERROR(text) {Print_message(LOGBIP, sfmt().message((text), CID, -1, nullptr), true); } 
#define GRPC_ERROR(n, text, status, context, reqp, repp) Print_message(LOGBIP, Grpc_error(CID, (n), (text), (status), (context), (reqp), (repp)), true);
#define PTIME2(method, stage, stage_name, cet, td, calls) {\
    if(Time_call) Time_info << method << "\t" << stage << "\t" << stage_name << "\t" << cet << "\t" << td << "\t" << calls << "\n"; \
    if(Debug_Flag||Trace_Flag||Trace_call) Print_message(LOGBIP, sfmt().message(\
        sfmt() << method << " stage " << stage << " (" << stage_name << ") started after " << cet << " and took " << td << " for " << calls << " call(s)", \
        CID, -1, nullptr)); }

class {{NAME_ID}}_service final: public {{CPP_SERVER_BASE}}::Service {
public:
    bool Debug_Flag = false;
    bool Trace_Flag = false;
    bool Async_Flag;
    std::atomic<long> Call_Counter;

{I:CLI_NODE_NAME{
    /* {{CLI_NODE_NAME}} line {{CLI_NODE_LINE}}
     */
    int const {{CLI_NODE_ID}}_maxcc = {{CLI_NODE_MAX_CONCURRENT_CALLS}};
    std::unique_ptr<{{CLI_SERVICE_NAME}}::Stub> {{CLI_NODE_ID}}_stub[{{CLI_NODE_MAX_CONCURRENT_CALLS}}];
    std::unique_ptr<::grpc::ClientAsyncResponseReader<{{CLI_OUTPUT_TYPE}}>> {{CLI_NODE_ID}}_prep(long CID, int call_number, ::grpc::CompletionQueue &CQ, ::grpc::ClientContext &CTX, {{CLI_INPUT_TYPE}} *A_inp, bool Debug_Flag, bool Trace_call, std::shared_ptr<Logbuffer_info> LOGBIP) {
        TRACECM(true, call_number, "{{CLI_NODE_NAME}} prepare request: ", A_inp);
        auto result = {{CLI_NODE_ID}}_stub[call_number % {{CLI_NODE_ID}}_maxcc]->PrepareAsync{{CLI_METHOD_NAME}}(&CTX, *A_inp, &CQ);
        return result;
    }

    ::grpc::Status {{CLI_NODE_ID}}_call(long CID, {{CLI_OUTPUT_TYPE}} *A_outp, {{CLI_INPUT_TYPE}} *A_inp, bool Debug_Flag, bool Trace_call, std::shared_ptr<Logbuffer_info> LOGBIP) {
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

std::map<std::string, std::pair<char const *, char const *>> node_schemas = {
{I:CLI_NODE_NAME{    { "{{CLI_NODE_NAME}}", { {{CLI_INPUT_JSON_SCHEMA_C}}, {{CLI_OUTPUT_JSON_SCHEMA_C}} } },
}I}
};
std::map<std::string, std::pair<char const *, char const *>> entry_schemas = {
{I:ENTRY_DOT_NAME{// {{ENTRY_SERVICE_NAME}} i.e. {{ENTRY_DOT_NAME}}
    { "{{ENTRY_NAME}}", { {{ENTRY_INPUT_JSON_SCHEMA_C}}, {{ENTRY_OUTPUT_JSON_SCHEMA_C}} } }, 
}I}
};
static int not_found(struct mg_connection *conn, std::string const &message) {
	mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: %lu\r\n"
              "\r\n", message.length());
	mg_printf(conn, "%s", message.c_str());
    return 1;
}
static int json_reply(struct mg_connection *conn, int code, char const *msg, char const *content, size_t length=0) {
	mg_printf(conn, "HTTP/1.1 %d %s\r\n"
              "Content-Type: application/json\r\n"
              "Content-Length: %lu\r\n"
              "\r\n", code, msg, length == 0? strlen(content): length);
	mg_printf(conn, "%s", content);
	return 1;
}
static int json_reply(struct mg_connection *conn, char const *content, size_t length=0) {
    return json_reply(conn, 200, "OK", content, length);
}
static int message_reply(struct mg_connection *conn, google::protobuf::Message const &message) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = false;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    std::string json_message;
    google::protobuf::util::MessageToJsonString(message, &json_message, options);
    return json_reply(conn, json_message.c_str(), json_message.length());
}
static int rest_grpc_error(struct mg_connection *conn, ::grpc::ClientContext const &context, ::grpc::Status const &status) {
    std::string errm = sfmt() 
        << "{" 
        << "\"code\": 500,"
        << "\"from\":" << json_string(context.peer()) << ","
        << "\"grpc-code\":" << status.error_code() << ","
        << "\"message\":\"" << json_string(status.error_message()) << "\","
        << "\"details\":\"" << json_string(status.error_details()) << "\""
        << "}";
    return json_reply(conn, 500, "gRPC Error", errm.c_str(), errm.length());
}
static int methods_handler(struct mg_connection *conn, void *cbdata) {
    static char const *methods_reply = "{"
        {I:ENTRY_DOT_NAME{
               "\"/{{ENTRY_NAME}}\": {"
               "\"input-schema-url\": \"/-input/{{ENTRY_NAME}}\","
               "\"output-schema-url\": \"/-output/{{ENTRY_NAME}}\""
               "},"
        }I}
        {I:CLI_NODE_NAME{
               "\"/-node/{{CLI_NODE_NAME}}\": {"
               "\"input-schema-url\": \"/-node-input/{{CLI_NODE_NAME}}\","
               "\"output-schema-url\": \"/-node-output/{{CLI_NODE_NAME}}\""
               "},"
        }I}
        "\"/-methods\": {}"
    "}";
    return json_reply(conn, methods_reply, strlen(methods_reply));
}
static int entry_schema_handler(struct mg_connection *conn, void *cbdata) {
	char const *local_uri = mg_get_request_info(conn)->local_uri;
    char const *uri = cbdata? "/-input": "/-output";
    if(strlen(uri) + 1 >= strlen(local_uri)) 
        return not_found(conn, "Entry name not spefified");
    auto sp = entry_schemas.find(local_uri + strlen(uri) + 1);
    if(sp == entry_schemas.end()) 
        return not_found(conn, "Entry name not recognized");
    return json_reply(conn, cbdata? sp->second.first: sp->second.second);
}
static int node_schema_handler(struct mg_connection *conn, void *cbdata) {
	char const *local_uri = mg_get_request_info(conn)->local_uri;
    char const *uri = cbdata? "/-node-input": "/-node-output";
    if(strlen(uri) + 1 >= strlen(local_uri)) 
        return not_found(conn, "Node name not spefified");
    auto sp = node_schemas.find(local_uri + strlen(uri) + 1);
    if(sp == node_schemas.end()) 
        return not_found(conn, "Node name not recognized");
    return json_reply(conn, cbdata? sp->second.first: sp->second.second);
}
static int rest_log_message(const struct mg_connection *conn, const char *message) {
    std::cerr << message << std::flush;
	return 1;
}
{I:ENTRY_DOT_NAME{
static int method_{{ENTRY_NAME}}_handler(struct mg_connection *conn, void *cbdata) {
    return not_found(conn, "not implemented");
}
}I}
{I:CLI_NODE_NAME{
static int method_node_{{CLI_NODE_ID}}_handler(struct mg_connection *A_conn, void *) {
    std::shared_ptr<::grpc::Channel> L_channel(::grpc::CreateChannel({{CLI_NODE_ID}}_endpoint, ::grpc::InsecureChannelCredentials()));
    std::unique_ptr<{{CLI_SERVICE_NAME}}::Stub> L_client_stub = {{CLI_SERVICE_NAME}}::NewStub(L_channel);                    
    {{CLI_OUTPUT_TYPE}} L_outp; 
    {{CLI_INPUT_TYPE}} L_inp;
    std::string L_inp_json;
    // TODO grab the form data
    auto L_conv_status = google::protobuf::util::JsonStringToMessage(L_inp_json, &L_inp);
    if(!L_conv_status.ok()) {
        std::string error_message = sfmt() << "{"
            << "\"code\": 400,"
            << "\"message\": \"Input failed conversion to protobuf\","
            << "\"description\":" <<json_string(L_conv_status.ToString())
            << "}";
        return json_reply(A_conn, 400, "Bad Request", error_message.c_str(), error_message.length());
    }
    ::grpc::ClientContext L_context;
    auto const L_start_time = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const L_deadline = L_start_time + std::chrono::milliseconds({{CLI_NODE_TIMEOUT:120000}});
    L_context.set_deadline(L_deadline);
    ::grpc::Status L_status = L_client_stub->{{CLI_METHOD_NAME}}(&L_context, L_inp, &L_outp);
    if(!L_status.ok()) 
        return rest_grpc_error(A_conn, L_context, L_status);
    return message_reply(A_conn, L_outp);
}
}I}
int start_civetweb(char const *rest_port) {
    const char *options[] = {
        "document_root", ".",
        "listening_ports", rest_port,
        "request_timeout_ms", "3600000",
        "error_log_file", "error.log",
        "extra_mime_types", ".proto=text/plain,.svg=image/svg",
        "enable_auth_domain_check", "no",
        "max_request_size", "65536", 
        "num_threads", "12",
        0
    };
	struct mg_callbacks callbacks;
	struct mg_context *ctx;

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.log_message = rest_log_message;
	ctx = mg_start(&callbacks, 0, options);

	if(ctx == nullptr) return 1;
	mg_set_request_handler(ctx, "/-input", entry_schema_handler, (void*) 1);
	mg_set_request_handler(ctx, "/-output", entry_schema_handler, 0);
	mg_set_request_handler(ctx, "/-node-input", node_schema_handler, (void*) 1);
	mg_set_request_handler(ctx, "/-node-output", node_schema_handler, 0);
	mg_set_request_handler(ctx, "/-methods", methods_handler, 0);
{I:ENTRY_DOT_NAME{    mg_set_request_handler(ctx, "/{{ENTRY_NAME}}", method_{{ENTRY_NAME}}_handler, 0);
}I}
{I:CLI_NODE_NAME{    mg_set_request_handler(ctx, "/-node/{{CLI_NODE_NAME}}", method_node_{{CLI_NODE_ID}}_handler, 0);
}I}
/*
	mg_set_request_handler(ctx, EXAMPLE_URI, ExampleHandler, 0);
	mg_set_request_handler(ctx, EXIT_URI, ExitHandler, 0);
	mg_set_request_handler(ctx, "/A/B", ABHandler, 0);
	mg_set_request_handler(ctx, "/B$", BXHandler, (void *)0);
	mg_set_request_handler(ctx, "/B/A$", BXHandler, (void *)1);
	mg_set_request_handler(ctx, "/B/B$", BXHandler, (void *)2);
	mg_set_request_handler(ctx, "**.foo$", FooHandler, 0);
	mg_set_request_handler(ctx, "/close", CloseHandler, 0);
	mg_set_request_handler(ctx, "/form", FileHandler, (void *)"../../test/form.html");
	mg_set_request_handler(ctx, "/handle_form.embedded_c.example.callback", FormHandler, (void *)0); 
	mg_set_request_handler(ctx, "/on_the_fly_form", FileUploadForm, (void *)"/on_the_fly_form.md5.callback");
    mg_set_request_handler(ctx, "/on_the_fly_form.md5.callback", CheckSumHandler, (void *)0);
	mg_set_request_handler(ctx, "/cookie", CookieHandler, 0);
	mg_set_request_handler(ctx, "/postresponse", PostResponser, 0);
	mg_set_request_handler(ctx, "/websocket", WebSocketStartHandler, 0);
	mg_set_request_handler(ctx, "/auth", AuthStartHandler, 0);
*/
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

int main(int argc, char *argv[]) {
    if(argc != 2) {
       std::cout << "Usage: " << argv[0] << " GRPC-PORT\n\n";
       std::cout << "Set the ENDPOINT variable with the host:port for each node:\n";
       {I:CLI_NODE_NAME{std::cout << "{{CLI_NODE_UPPERID}}_ENDPOINT for node {{CLI_NODE_NAME}} ({{GRPC_SERVICE_NAME}}.{{CLI_METHOD_NAME}})\n";
       }I}
       std::cout << "\n";
       std::cout << "Set {{NAME_UPPERID}}_REST_PORT=0 to disable the REST gateway service ({{REST_NODE_PORT}})\n";
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

    {{NAME_ID}}_service service(strtobool(std::getenv("{{NAME_UPPERID}}_ASYNC"), true){I:CLI_NODE_ID{, {{CLI_NODE_ID}}_endpoint}I});
    service.Debug_Flag = strtobool(std::getenv("{{NAME_UPPERID}}_DEBUG"));
    service.Trace_Flag = strtobool(std::getenv("{{NAME_UPPERID}}_TRACE"));

    // Listen on the given address without any authentication mechanism.
    std::string server_address("[::]:"); server_address += argv[1];
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &listening_port);

    // Register services
    builder.RegisterService(&service);
    if((Flow_logger_service_enabled = strtobool(std::getenv("{{NAME_UPPERID}}_LOGGER"), true)))
        builder.RegisterService(&Flow_logger);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    if(listening_port == 0) {
        std::cerr << "failed to start {{NAME}} gRPC service at " << listening_port << "\n";
        return 1;
    }
    std::cerr << "gRPC service {{NAME}} listening on port: " << listening_port << "\n";

    // Set up the REST gateway if enabled
    char const *rest_port = std::getenv("{{NAME_UPPERID}}_REST_PORT");
    if(rest_port != nullptr && strspn("\t\r\n ", rest_port) < strlen(rest_port)) {
        if(start_civetweb(rest_port) != 0) {
            std::cerr << "Failed to start REST gateway service\n";
            return 1;
        }
    }
    std::cerr 
        << "debug: " << (service.Debug_Flag? "yes": "no")
        << ", trace: " << (service.Trace_Flag? "yes": "no")
        << ", asynchronous client calls: " << (service.Async_Flag? "yes": "no") 
        << ", logger service: " << (Flow_logger_service_enabled? "enabled": "disabled")
        << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
    return 0;
}
