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
int call_timeout = 0;
int concurrent_calls = 1;
bool show_headers = false;
bool show_help = false;
std::vector<std::pair<char, std::string>> show;
std::vector<std::pair<std::string, std::string>> headers;

static void add_header(std::string const &header) {
    auto eqp = header.find_first_of('=');
    if(eqp == std::string::npos) 
        headers.push_back(std::make_pair(header, std::string()));
    else 
        headers.push_back(std::make_pair(header.substr(0, eqp), header.substr(eqp+1)));
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
    { "input-schema",         required_argument, nullptr, 'I' },
    { "output-schema",        required_argument, nullptr, 'O' },
    { "proto",                required_argument, nullptr, 'P' },
    { nullptr,                0,                 nullptr,  0 }
};

static bool parse_command_line(int &argc, char **&argv) {
    int ch;
    while((ch = getopt_long(argc, argv, "hbgjT:n:H:I:O:P:", long_opts, nullptr)) != -1) {
        switch (ch) {
            case 'b': use_blocking_calls = true; break;
            case 'g': ignore_grpc_errors = true; break;
            case 'j': ignore_json_errors = true; break;
            case 'v': show_headers = true; break;
            case 't': time_calls = true; break;
            case 'h': show_help = true; break;
            case 'H': add_header(optarg); break;
            case 'n': concurrent_calls = atoi(optarg); break;
            case 'I': case 'O': case 'P':
                      show.push_back(std::make_pair(ch, std::string(optarg))); break;
            case 'T': call_timeout = std::atoi(optarg); break;
            default:
                return false;
        }
    }
    argv[optind-1] = argv[0];
    argv+=optind-1;
    argc-=optind-1;
    return true;
}

enum entry_id {
{I:ENTRY_FULL_NAME{    {{ENTRY_FULL_NAME/id/upper}},
}I}
    NONE
};
struct entry_info {
    char const *entry_full_name;
    entry_id id;
    
    char const *input_schema;
    char const *output_schema;
    char const *proto;
};
static std::array<entry_info, {{ENTRY_COUNT}}> entry_table = { {
{I:ENTRY_FULL_NAME{    {"{{ENTRY_FULL_NAME}}",  {{ENTRY_FULL_NAME/id/upper}}, {{ENTRY_INPUT_SCHEMA_JSON/c}}, {{ENTRY_OUTPUT_SCHEMA_JSON/c}}, {{ENTRY_PROTO/c}} },
}I}
} };

template <class INT, class OUTT>
struct call_info {
    INT request;
    OUTT response;
    std::unique_ptr<::grpc::ClientAsyncResponseReader<OUTT>> carr;
    grpc::Status status;
    grpc::ClientContext context;
    unsigned long id;
};

struct output_queue {
    std::ostream &outs; 
    unsigned long line_count;
    output_queue(std::ostream &o): outs(o), line_count(0) {
    }
    void output(unsigned long input_line, std::string &line) {
        outs << line << "\n";
        ++line_count;
    }
};

template<class INT, class OUTT, class UPSTUB>
static int process_file(unsigned concurrent_calls, std::string const &label, std::istream &ins, std::ostream &outs, UPSTUB const &stub) {
    grpc::CompletionQueue cq;
    std::vector<call_info<INT, OUTT>> ciq;
    std::map<unsigned long, unsigned> ccp;
    output_queue oq(outs);

    std::string input_line;
    unsigned long line_count = 0;
    unsigned long out_line = 0;
    bool have_input;

    while((have_input = !!std::getline(ins, input_line)) || ccp.size() > 0) {
        if(have_input) {
            ++line_count;
            auto b = input_line.find_first_not_of("\t\r\a\b\v\f ");
            // Empty or comment line in the input. Reflect it as is in the output.
            if(b == std::string::npos || input_line[b] == '#') {
                oq.output(line_count, input_line);
                continue;
            }
            // If no free slot, wait for completion
            unsigned long got_tag;
            bool ok = false;
            cq.Next((void **) &got_tag, &ok);
            if(ok && got_tag == 1) {
                auto cci = ccp.find(got_tag);
                // check reply and status
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
static entry_info const *find_entry(std::string const &name) {
    entry_info const *p = nullptr;
    std::vector<std::string> matches;
    for(auto const &rme: entry_table) {
        std::string mn = rme.entry_full_name;
        if(mn == name || (mn.length() > name.length() + 1 && mn.substr(mn.length()-name.length()-1) == std::string(".")+name)) {
            matches.push_back(mn); 
            p = &rme;
        }
    }
    switch(matches.size()) {
        case 0:
            std::cerr << "Unknown method \"" << name << "\"\n";
            break;
        case 1: 
            break;
        default:
            std::cerr << "Ambiguous method name \"" << name << "\", matches ";
            for(unsigned u = 0; u+2 < matches.size(); ++u) std::cerr << "\"" << name << "\", ";
            std::cerr << "\"" << matches[matches.size()-2] << "\" and \"" << matches[matches.size()-1] << "\"\n";
            break;
    }
    return p;
}
int main(int argc, char *argv[]) {
    if(!parse_command_line(argc, argv) || show_help || (show.size() == 0 && (argc < 2 || argc > 4))) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] PORT|ENDPOINT [[SERVICE.]RPC] [JSONL-INPUT-FILE]\n";
        std::cerr << "    or " << argv[0] << " --input-schema|--output-schema|--proto [SERVICE.]RPC\n";
        std::cerr << "    or " << argv[0] << " --help\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "  -b, --blocking-calls        Disable asynchronous calls in the aggregator\n";
        std::cerr << "  -g, --ignore-grpc-errors    Keep going when grpc errors are encountered\n";
        std::cerr << "  -j, --ignore-json-errors    Keep going even if input JSON fails conversion to protobuf\n";
        std::cerr << "  -H, --header NAME=VALUE     Add header to every request\n";
        std::cerr << "  -h, --help                  Display this help\n";
        std::cerr << "  -v, --show-headers          Display headers returned with the reply\n";
        std::cerr << "  -n, --streams INTEGER       Number of concurrent calls to make through the connection\n";         
        std::cerr << "  -t, --time-calls            Retrieve timing information for each call\n";
        std::cerr << "  -T, --timeout MILLISECONDS  Limit each call to the given amount of time\n";
        std::cerr << "\n";
        std::cerr << "gRPC Entries:\n";
        for(auto const &mid: entry_table)
            std::cerr << "  " << mid.entry_full_name << "\n";
        std::cerr << "\n";
        return show_help? 1: 0;
    }

    if(show.size() > 0) {
        for(auto const &se: show) {
            auto pi = find_entry(se.second);
            if(pi != nullptr) switch (se.first) {
                case 'I':
                    std::cout << pi->input_schema << "\n";
                    break;
                case 'O':
                    std::cout << pi->output_schema << "\n";
                    break;
                case 'P':
                    std::cout << pi->proto << "\n";
                    break;
                default:
                    break;
            }
        }
        return 0;
    }

    std::string endpoint(strchr(argv[1], ':') == nullptr? std::string("localhost:")+argv[1]: std::string(argv[1]));
    entry_info const *eip = nullptr;
    if(argc > 2) {
        eip = find_entry(argv[2]);
    } else if(entry_table.size() == 1) {
        eip = &entry_table[0];
    } else {
        std::cerr << "RPC name must be specified as one of:\n";
        for(auto const &mid: entry_table)
            std::cerr << "  " << mid.entry_full_name << "\n";
        std::cerr << "\n";
    }
    if(eip == nullptr) 
        return 1;

    if(concurrent_calls <= 0) {
        std::cerr << "Invalid number of concurrent calls: " << concurrent_calls << "\n";
        return 1;
    }

    std::shared_ptr<grpc::Channel> channel(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
    std::istream *in = nullptr;
    std::ifstream infs;
    std::string input_label;
    if(argc < 4 || strcmp("-", argv[3]) == 0) {
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
    switch(eip->id) {
{I:ENTRY_FULL_NAME{        case {{ENTRY_FULL_NAME/id/upper}}: {
            // rpc {{METHOD_FULL_NAME}}
            std::unique_ptr<{{ENTRY_SERVICE_NAME}}::Stub> client_stub({{ENTRY_SERVICE_NAME}}::NewStub(channel));
            rc = process_file<{{ENTRY_INPUT_TYPE}}, {{ENTRY_OUTPUT_TYPE}}, std::unique_ptr<{{ENTRY_SERVICE_NAME}}::Stub>>(concurrent_calls, input_label, *in, std::cout, client_stub);
        } break;
}I}
        default:
            break;
    }
    return rc;
}

