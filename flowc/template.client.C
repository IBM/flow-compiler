/************************************************************************************************************
 *
 * {{NAME}}-client.C 
 * generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include <ratio>
#include <chrono>
#include <getopt.h>
#include <grpc++/grpc++.h>
#include <google/protobuf/util/json_util.h>

static std::string message_to_json(google::protobuf::Message const &message, bool pretty=true) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = pretty;
    options.always_print_primitive_fields = false;
    options.preserve_proto_field_names = true;
    std::string json_reply;
    google::protobuf::util::MessageToJsonString(message, &json_reply, options);
    return json_reply;
}
{I:GRPC_GENERATED_H{#include "{{GRPC_GENERATED_H}}"
}I}

bool ignore_json_errors = false;
bool ignore_grpc_errors = false;
bool time_calls = false;
bool use_blocking_calls = false;
bool enable_trace = false;
bool show_help = false;
bool show_headers = false;
int call_timeout = 0;
std::string trace_filename;

std::map<std::string, std::string> added_headers;

static void add_header(std::string const &header) {
    auto eqp = header.find_first_of('=');
    if(eqp == std::string::npos) 
        added_headers[header] = "";
    else 
        added_headers[header.substr(0, eqp)] = header.substr(eqp+1);
}

struct option long_opts[] {
    { "help",                 no_argument, nullptr, 'h' },
    { "blocking-calls",       no_argument, nullptr, 'b' },
    { "ignore-grpc-errors",   no_argument, nullptr, 'g' },
    { "ignore-json-errors",   no_argument, nullptr, 'j' },
    { "time-calls",           no_argument, nullptr, 'c' },
    { "show-headers",         no_argument, nullptr, 'v' },
    { "header",               required_argument, nullptr, 'H' },
    { "timeout",              required_argument, nullptr, 'T' },
    { "trace",                required_argument, nullptr, 't' },
    { nullptr,                0,                 nullptr,  0 }
};

static bool parse_command_line(int &argc, char **&argv) {
    int ch;
    while((ch = getopt_long(argc, argv, "hbgjT:t:", long_opts, nullptr)) != -1) {
        switch (ch) {
            case 'b': use_blocking_calls = true; break;
            case 'g': ignore_grpc_errors = true; break;
            case 'j': ignore_json_errors = true; break;
            case 'v': show_headers = true; break;
            case 'c': time_calls = true; break;
            case 'h': show_help = true; break;
            case 't': trace_filename = optarg; break;
            case 'H': add_header(optarg); break;
            case 'T': 
                call_timeout = std::atoi(optarg); 
            break;
            default:
                return false;
        }
    }
    argv[optind-1] = argv[0];
    argv+=optind-1;
    argc-=optind-1;
    return true;
}

enum rpc_method_id {
{I:METHOD_FULL_UPPERID{    {{METHOD_FULL_UPPERID}},
}I}
    NONE
};
static std::map<std::string, rpc_method_id> rpc_methods = {
{I:METHOD_FULL_UPPERID{    {"{{METHOD_FULL_NAME}}", {{METHOD_FULL_UPPERID}}},
}I}
};
struct arg_field_descriptor { 
    int next; 
    char type; 
    char const *name; 
};
int help(std::string const &name, arg_field_descriptor const *d, int p = 0, int indent = 0) {
    if(indent == 0) std::cerr << "gRPC method " << name << " {\n";
    indent += 4;
    while(d[p].type) {
        std::cerr << std::string(indent, ' ') << d[p].name << ": ";
        switch(tolower(d[p].type)) {
            case 'm': 
                std::cerr << "{\n";
                indent += 4;
                break;
            case 'n':
                std::cerr << " NUMBER\n";
                break;
            case 'e':
                std::cerr << " ENUM\n";
                break;
            case 's':
                std::cerr << " STRING\n";
                break;
            default:
                break;
        }
        if(d[p].next == 0) {
            indent -= 4;
            std::cerr << std::string(indent, ' ') << "}\n";
        }
        ++p;
    }
    std::cerr << "}\n";
    return p;
}
/*
        bool Trace_call = Get_metadata_value(Client_metadata, "trace-call");
        bool Time_call = Get_metadata_value(Client_metadata, "time-call");
        bool Async_call = !Get_metadata_value(Client_metadata, "blocking-calls", !Async_Flag);
        */
{I:METHOD_FULL_UPPERID{/****** {{METHOD_FULL_NAME}}
 */
static int file_{{METHOD_FULL_ID}}(std::string const &label, std::istream &ins, std::unique_ptr<{{SERVICE_NAME}}::Stub> const &stub) {
    {{SERVICE_INPUT_TYPE}} input;
    {{SERVICE_OUTPUT_TYPE}} output;
    std::string input_line;
    unsigned line_count = 0;
    while(std::getline(ins, input_line)) {
        ++line_count;
        auto b = input_line.find_first_not_of("\t\r\a\b\v\f ");
        if(b == std::string::npos || input_line[b] == '#') {
            // Empty or comment line in the input. Reflect it as is in the output.
            std::cout << input_line << "\n";
            continue;
        }
        input.Clear(); 
        auto conv_status = google::protobuf::util::JsonStringToMessage(input_line, &input);
        if(!conv_status.ok()) {
            std::cerr << label << "(" << line_count << "): " << conv_status.ToString() << std::endl;
            if(!ignore_json_errors) 
                return 1;
            // Ouput a new line to keep the input and the output files in sync
            std::cout << "\n";
        }
        output.Clear();
        grpc::ClientContext context;
        if(use_blocking_calls) context.AddMetadata("overlapped-calls", "0");
        if(time_calls) context.AddMetadata("time-call", "1");
        for(auto const &xhe: added_headers) 
            context.AddMetadata(xhe.first, xhe.second);
        
        if(call_timeout > 0) {
            auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(call_timeout);
            context.set_deadline(deadline);
        }
        grpc::Status status = stub->{{METHOD_NAME}}(&context, input, &output);
        if(show_headers) for(auto const &mde: context.GetServerTrailingMetadata()) {
            std::string header(mde.first.data(), mde.first.length());
            std::cerr << header << ": " << std::string(mde.second.data(), mde.second.length()) << "\n";
        }
        if(!status.ok()) {
            std::cerr << label << "(" << line_count << ") from " << context.peer() << ": " << status.error_code() << ": " << status.error_message() << "\n" << status.error_details() << std::endl;
            if(!ignore_grpc_errors)
                return 1;
            // Otput a comment line with the error message to keep the input and output file in sync
            std::cout << "# " << status.error_code() << ": " << status.error_message() << "\n";
        }
        std::cout << message_to_json(output, false)  << "\n";
    }
    return 0;
}
static arg_field_descriptor const {{METHOD_FULL_ID}}_di_{{SERVICE_INPUT_ID}}[] { 
    {{SERVICE_INPUT_DESC}}, 
    { 0, '\0', "" }
};
}I}
static rpc_method_id get_rpc_method_id(std::string const &name) {
    rpc_method_id id = NONE;
    std::vector<std::string> matches;
    for(auto const &rme: rpc_methods) 
        if(rme.first == name || (rme.first.length() > name.length() + 1 && rme.first.substr(rme.first.length()-name.length()-1) == std::string(".")+name)) {
            matches.push_back(rme.first); 
            id = rme.second;
        }
    switch(matches.size()) {
        case 0:
            std::cerr << "Unknown method \"" << name << "\"\n";
        case 1: 
            return id;
        default:
            std::cerr << "Ambiguous method name \"" << name << "\", matches ";
            for(unsigned u = 0; u+2 < matches.size(); ++u) std::cerr << "\"" << name << "\", ";
            std::cerr << "\"" << matches[matches.size()-2] << "\" and \"" << matches[matches.size()-1] << "\"\n";
            return NONE;
    }
}
int main(int argc, char *argv[]) {
    bool show_method_help = argc == 3 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0);
    if(!show_method_help && (!parse_command_line(argc, argv) || argc != 4 || show_help)) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] <PORT|ENDPOINT> <[SERVICE.]RPC> <JSON-INPUT-FILE>\n";
        std::cerr << "    or " << argv[0] << " --help [[SERVICE.]RPC]\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "      --blocking-calls        Disable asynchronous calls in the aggregator\n";
        std::cerr << "      --ignore-grpc-errors    Keep going when grpc errors are encountered\n";
        std::cerr << "      --ignore-json-errors    Keep going even if json conversion fails for a request\n";
        std::cerr << "      --time-calls            Retrieve timing information for each call\n";
        std::cerr << "      --header NAME=VALUE     Add header to the request\n";
        std::cerr << "      --show-headers          Show headers\n";
        std::cerr << "      --timeout MILLISECONDS  Limit each call to the given amount of time\n";
        std::cerr << "      --trace FILE, -t FILE   Enable the trace flag and output the trace to FILE\n";
        std::cerr << "      --help                  Display this screen and exit\n";
        std::cerr << "\n";
        std::cerr << "gRPC Methods:\n";
        for(auto mid: rpc_methods) 
            std::cerr << "\t" << mid.first << "\n";
        std::cerr << "\n";
        return show_help? 1: 0;
    }
    rpc_method_id mid = get_rpc_method_id(argv[2]);
    if(mid == NONE) return 1;
    if(show_method_help) {
        switch(mid) {
{I:METHOD_FULL_UPPERID{        case {{METHOD_FULL_UPPERID}}: 
                help("{{METHOD_FULL_NAME}}", {{METHOD_FULL_ID}}_di_{{SERVICE_INPUT_ID}});
                break;
}I}
            case NONE:
                break;
        }
        return 0;
    }
    std::string endpoint(strchr(argv[1], ':') == nullptr? std::string("localhost:")+argv[1]: std::string(argv[1]));
    std::shared_ptr<grpc::Channel> channel(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
    std::istream *in = nullptr;
    std::ifstream infs;
    std::string input_label;
    if(strcmp("-", argv[3]) == 0) {
        in = &std::cin;
        input_label = "<stdin>";
    } else {
        infs.open(argv[3]);
        if(!infs.is_open()) {
            std::cerr << "Could not open file: " << argv[3] << "\n";
            return 1;
        }
        in = &infs;
        input_label = argv[3];
    }
    int rc = 1;
    switch(mid) {
{I:METHOD_FULL_UPPERID{        case {{METHOD_FULL_UPPERID}}: {
            std::cerr << "method: {{METHOD_FULL_NAME}}\n";
            std::unique_ptr<{{SERVICE_NAME}}::Stub> client_stub({{SERVICE_NAME}}::NewStub(channel));
            rc = file_{{METHOD_FULL_ID}}(input_label, *in, client_stub);
        } break;
}I}
        case NONE:
            break;
    }
    return rc;
}

