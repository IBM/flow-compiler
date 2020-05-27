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
#include <forward_list>
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
int concurrent_calls = 1;
std::string show_input, show_output;

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
    { "time-calls",           no_argument, nullptr, 't' },
    { "show-headers",         no_argument, nullptr, 'v' },
    { "header",               required_argument, nullptr, 'H' },
    { "timeout",              required_argument, nullptr, 'T' },
    { "streams",              required_argument, nullptr, 'n' },
    { "input-schema",         required_argument, nullptr, 's' },
    { "output-schema",        required_argument, nullptr, 'S' },
    { nullptr,                0,                 nullptr,  0 }
};

static bool parse_command_line(int &argc, char **&argv) {
    int ch;
    while((ch = getopt_long(argc, argv, "hbgjT:n:s:S:", long_opts, nullptr)) != -1) {
        switch (ch) {
            case 'b': use_blocking_calls = true; break;
            case 'g': ignore_grpc_errors = true; break;
            case 'j': ignore_json_errors = true; break;
            case 'v': show_headers = true; break;
            case 't': time_calls = true; break;
            case 'h': show_help = true; break;
            case 'H': add_header(optarg); break;
            case 'n': concurrent_calls = atoi(optarg); break;
            case 'S': show_output = optarg; break;
            case 's': show_input = optarg; break;
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
{I:METHOD_FULL_ID{
static std::string input_schema_{{METHOD_FULL_ID}} = {{SERVICE_INPUT_SCHEMA_JSON_C}};
static std::string output_schema_{{METHOD_FULL_ID}} = {{SERVICE_OUTPUT_SCHEMA_JSON_C}};
}I}

{I:METHOD_FULL_UPPERID{/****** {{METHOD_FULL_NAME}}
 */
static int file_{{METHOD_FULL_ID}}(unsigned concurrent_calls, std::string const &label, std::istream &ins, std::unique_ptr<{{SERVICE_NAME}}::Stub> const &stub) {
    grpc::CompletionQueue cq;
/*
    std::vector<{{SERVICE_INPUT_TYPE}}> inputs(concurrent_calls);
    std::vector<{{SERVICE_OUTPUT_TYPE}}> outputs(concurrent_calls);
    std::vector<grpc::ClientContext> contexts(concurrent_calls);
    std::vector<grpc::Status> statuses(concurrent_calls);
    std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReader<{{SERVICE_OUTPUT_TYPE}}>>> carrs(concurrent_calls);
    std::vector<bool> busy(concurrent_calls, false);
*/
    typedef {{SERVICE_OUTPUT_TYPE}} outm_t;
    typedef std::unique_ptr<::grpc::ClientAsyncResponseReader<outm_t>> carr_up_t;

    std::forward_list<std::tuple< bool, grpc::ClientContext, grpc::Status, carr_up_t, outm_t, std::string>> queued_calls; 

    std::string input_line;
    unsigned line_count = 0;
    bool have_input;

    while((have_input = !!std::getline(ins, input_line)) || queued_calls.begin() != queued_calls.end()) {
        if(have_input) {
            ++line_count;
            auto b = input_line.find_first_not_of("\t\r\a\b\v\f ");
            if(b == std::string::npos || input_line[b] == '#') {
                // Empty or comment line in the input. Reflect it as is in the output.
                /*
                queued_calls.emplace_after(queued_calls.end(), std::make_tuple< bool, grpc::ClientContext, grpc::Status, carr_up_t, outm_t, std::string>(
                        true, grpc::ClientContext(), grpc::Status(), carr_up_t(nullptr), outm_t(), std::string(input_line)));
                        */
            }
        }

        // find the first available slot
/*
        unsigned x = 0; while(x < concurrent_calls && busy[x]) ++x;
        if(x == concurrent_calls || !have_input) {
            
            //while(cq.Next(&TAG, &NextOK)) {
            //}
            
            if(show_headers) {
                for(auto const &mde: contexts[x]->GetServerInitialMetadata()) {
                    std::string header(mde.first.data(), mde.first.length());
                    std::cerr << "- " << header << ": " << std::string(mde.second.data(), mde.second.length()) << "\n";
                }
                for(auto const &mde: contexts[x]->GetServerTrailingMetadata()) {
                    std::string header(mde.first.data(), mde.first.length());
                    std::cerr << "= " << header << ": " << std::string(mde.second.data(), mde.second.length()) << "\n";
                }
            }
            if(!statuses[x].ok()) {
                std::cerr << label << "(" << line_count << ") from " << contexts[x]->peer() << ": " << statuses[x].error_code() << ": " << statuses[x].error_message() << "\n" << statuses[x].error_details() << std::endl;
                if(!ignore_grpc_errors)
                    return 1;
                // Otput a comment line with the error message to keep the input and output file in sync
                std::cout << "# " << statuses[x].error_code() << ": " << statuses[x].error_message() << "\n";
            }
            std::cout << message_to_json(outputs[x], false)  << "\n";
        }

        if(have_input) {
            // prepare the input

            inputs[x].Clear();
            auto conv_status = google::protobuf::util::JsonStringToMessage(input_line, &inputs[x]);
            if(!conv_status.ok()) {
                std::cerr << label << "(" << line_count << "): " << conv_status.ToString() << std::endl;
                if(!ignore_json_errors) 
                    return 1;
                // Output a new line to keep the input and the output files in sync
                std::cout << "\n";
            }
            outputs[x].Clear();
            contexts[x] = std::unique_ptr<::grpc::ClientContext>(new ::grpc::ClientContext);

            if(use_blocking_calls) contexts[x]->AddMetadata("overlapped-calls", "0");
            if(time_calls) contexts[x]->AddMetadata("time-call", "1");
            for(auto const &xhe: added_headers) 
                contexts[x]->AddMetadata(xhe.first, xhe.second);

            if(call_timeout > 0) {
                auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(call_timeout);
                contexts[x]->set_deadline(deadline);
            }
            stub->PrepareAsync{{METHOD_NAME}}(contexts[x].get(), inputs[x], &cq);
        }
        */
    }
    return 0;
}
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
    if(!parse_command_line(argc, argv) || show_help || (argc != 4 && show_input.empty() && show_output.empty())) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] PORT|ENDPOINT [SERVICE.]RPC JSON-INPUT-FILE\n";
        std::cerr << "    or " << argv[0] << " --input-schema|--output-schema [SERVICE.]RPC\n";
        std::cerr << "    or " << argv[0] << " --help\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "  -b, --blocking-calls        Disable asynchronous calls in the aggregator\n";
        std::cerr << "  -g, --ignore-grpc-errors    Keep going when grpc errors are encountered\n";
        std::cerr << "  -j, --ignore-json-errors    Keep going even if input JSON fails conversion to protobuf\n";
        std::cerr << "  -H, --header NAME=VALUE     Add header to the request\n";
        std::cerr << "  -h, --help                  Display this help\n";
        std::cerr << "  -s, --input-schema RPC      Input JSON schema for RPC\n";
        std::cerr << "  -S, --output-schema RPC     Output JSON schema for RPC\n";
        std::cerr << "  -v, --show-headers          Display headers returned with the reply\n";
        std::cerr << "  -n, --streams INTEGER       Number of concurrent calls to make through the connection\n";         
        std::cerr << "  -t, --time-calls            Retrieve timing information for each call\n";
        std::cerr << "  -T, --timeout MILLISECONDS  Limit each call to the given amount of time\n";
        std::cerr << "\n";
        std::cerr << "gRPC Methods:\n";
        for(auto mid: rpc_methods) 
            std::cerr << "  " << mid.first << "\n";
        std::cerr << "\n";
        return show_help? 1: 0;
    }
    std::string mname;
    if(!show_input.empty())
        mname = show_input;
    else if(!show_output.empty())
        mname = show_output;
    else 
        mname = argv[2];

    rpc_method_id mid = get_rpc_method_id(mname);
    if(mid == NONE) 
        return 1;
    if(!show_input.empty()) {
        switch(mid) {
{I:METHOD_FULL_UPPERID{        case {{METHOD_FULL_UPPERID}}: 
                std::cout << input_schema_{{METHOD_FULL_ID}};
                break;
}I}
            case NONE:
                break;
        }
        return 0;
    }
    if(!show_output.empty()) {
        switch(mid) {
{I:METHOD_FULL_UPPERID{        case {{METHOD_FULL_UPPERID}}: 
                std::cout << output_schema_{{METHOD_FULL_ID}};
                break;
}I}
            case NONE:
                break;
        }
        return 0;
    }
    if(concurrent_calls <= 0) {
        std::cerr << "Invalid number of concurrent calls: " << concurrent_calls << "\n";
        return 1;
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
            rc = file_{{METHOD_FULL_ID}}(concurrent_calls, input_label, *in, client_stub);
        } break;
}I}
        case NONE:
            break;
    }
    return rc;
}

