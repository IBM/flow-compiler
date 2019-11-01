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

    {I:CLI_NODE_ID{std::string {{CLI_NODE_ID}}_endpoint(strchr({{CLI_NODE_ID}}_ep, ':') == nullptr? (std::string("localhost:")+{{CLI_NODE_ID}}_ep): std::string({{CLI_NODE_ID}}_ep));
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
    std::cerr << "gRPC service {{NAME}} listening on port: " << listening_port 
        << ", debug: " << (service.Debug_Flag? "yes": "no")
        << ", trace: " << (service.Trace_Flag? "yes": "no")
        << ", asynchronous client calls: " << (service.Async_Flag? "yes": "no") 
        << ", logger service: " << (Flow_logger_service_enabled? "enabled": "disabled") 
        << std::endl;

    // Set up the REST gateway if enabled
    char const *rest_port = std::getenv("{{NAME_UPPERID}}_REST_PORT");

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
    return 0;
}
