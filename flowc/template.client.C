/************************************************************************************************************
 *
 * {{NAME}}-client.C 
 * generated from {{MAIN_FILE_SHORT}} ({{MAIN_FILE_TS}})
 * with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
 * 
 */
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <forward_list>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <memory>
#include <ratio>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include <grpc++/grpc++.h>
#include <google/protobuf/util/json_util.h>
#include <ares.h>
static bool stringtobool(std::string const &s, bool default_value=false) {
    if(s.empty()) return default_value;
    std::string so(s.length(), ' ');
    std::transform(s.begin(), s.end(), so.begin(), ::tolower);
    if(so == "yes" || so == "y" || so == "t" || so == "true" || so == "on")
        return true;
    return std::atof(so.c_str()) != 0;
}
static std::string message_to_json(google::protobuf::Message const &message, bool pretty=true) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = pretty;
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
bool async_calls = false;
bool set_async_calls = false;
bool enable_trace = false;
int call_timeout = 3600000;
int concurrent_calls = 1;
bool show_headers = false;
bool show_help = false;
bool show_version = false;
std::vector<std::pair<char, std::string>> show;
std::vector<std::pair<std::string, std::string>> headers;

static void add_header(std::string const &header) {
    auto eqp = header.find_first_of('=');
    if(eqp == std::string::npos) 
        headers.push_back(std::make_pair(header, std::string()));
    else 
        headers.push_back(std::make_pair(header.substr(0, eqp), header.substr(eqp+1)));
}


enum entry_id {
{I:MDP_FULL_NAME{    {{MDP_FULL_NAME/id/upper}},
}I}
    eNONE
};
struct entry_info {
    bool entry;
    char const *entry_full_name;
    entry_id id;
    
    char const *input_schema;
    char const *output_schema;
    char const *proto;
};
static std::array<entry_info, {{MDP_COUNT}}> entry_table = { {
{I:MDP_FULL_NAME{    {{{MDP_IS_ENTRY}}, "{{MDP_FULL_NAME}}",  {{MDP_FULL_NAME/id/upper}}, {{MDP_INPUT_SCHEMA_JSON/c}}, {{MDP_OUTPUT_SCHEMA_JSON/c}}, {{MDP_PROTO/c}} },
}I}
} };

struct output_queue {
    std::ostream &outs; 
    unsigned long line_count;
    std::map<unsigned long, std::string> outq;
    std::string label;
    output_queue(std::ostream &o, std::string const &o_label): outs(o), line_count(0), label(o_label) {
    }
    void output(unsigned long input_line, std::string const &line) {
        if(line_count + 1 != input_line) {
            outq[input_line] = line;
            return;
        }
        outs << line;
        ++line_count;
        while(outq.size() > 0 && outq.begin()->first == line_count + 1) {
            outs << outq.begin()->second;
            ++line_count;
            outq.erase(outq.begin());
        }
    }
};
template <class INT, class OUTT>
struct call_info {
    unsigned long id = 0;
    INT request;
    OUTT response;
    std::unique_ptr<::grpc::ClientAsyncResponseReader<OUTT>> carr;
    std::unique_ptr<grpc::ClientContext> contextp;
    grpc::Status status;
};
template<class INT, class OUTT, class PPASP>
static int process_file(unsigned concurrent_calls, std::string const &label, std::istream &ins, output_queue &oq, output_queue &hq, PPASP prepare_async) {
    int error_count = 0;
    grpc::CompletionQueue cq;
    std::vector<call_info<INT, OUTT>> ciq(concurrent_calls);
    bool use_hq = hq.label != oq.label;

    unsigned active_calls = 0;
    std::string input_line;
    unsigned long line_count = 0;
    bool have_input = true, ok;
    unsigned slot = 0;
    INT request;

    while((have_input = have_input && !!std::getline(ins, input_line)) || active_calls > 0) {
        if(have_input) {
            ++line_count;
            auto b = input_line.find_first_not_of("\t\r\a\b\v\f ");
            // Empty or comment line in the input. Reflect it as is in the output.
            if(b == std::string::npos || input_line[b] == '#') {
                oq.output(line_count, input_line);
                continue;
            }
            request.Clear();
            // Attempt to convert the input 
            auto conv_status = google::protobuf::util::JsonStringToMessage(input_line, &request);
            if(!conv_status.ok()) {
                std::string error_message(conv_status.ToString());
                std::cerr << label << "(" << line_count << "): " << error_message << std::endl;
                ++error_count;
                if(!ignore_json_errors) 
                    break;
                oq.output(line_count, error_message);
                continue;
            }
        }
        if(active_calls == concurrent_calls || !have_input) {
            // Wait for completion
            slot = 0; ok = false;
            cq.Next((void **) &slot, &ok);
            --active_calls;
            if(ok && slot > 0 && slot <= concurrent_calls) {
                call_info<INT, OUTT> &cc = ciq[slot-1];
                std::ostringstream outs;
                if(show_headers) {
                    for(auto const &mde: cc.contextp->GetServerInitialMetadata()) 
                        outs << "#" << cc.id << "- " << std::string(mde.first.data(), mde.first.length()) << ": " << std::string(mde.second.data(), mde.second.length()) << "\n";
                    for(auto const &mde: cc.contextp->GetServerTrailingMetadata()) 
                        outs << "#" << cc.id << "= " << std::string(mde.first.data(), mde.first.length()) << ": " << std::string(mde.second.data(), mde.second.length()) << "\n";
                } else if(time_calls) {
                    for(auto const &mde: cc.contextp->GetServerTrailingMetadata()) if(std::string(mde.first.data(), mde.first.length()) == "times-bin")
                        outs << "#" << cc.id << "= " << std::string(mde.first.data(), mde.first.length()) << ": " << std::string(mde.second.data(), mde.second.length()) << "\n";
                }
                if(use_hq) {
                    hq.output(cc.id, outs.str());
                    outs.str("");
                }
                if(!cc.status.ok()) {
                    ++error_count;
                    std::cerr << label << "(" << cc.id << ")[" << cc.contextp->peer() << "]: error " << cc.status.error_code() << " " << cc.status.error_message() << "\n" << cc.status.error_details() << std::endl;
                    outs << "#" << cc.id << "~ [" << cc.contextp->peer() << "]  error " << cc.status.error_code() << " " << cc.status.error_message() << "\n";
                    if(!ignore_grpc_errors)
                        break;
                } else {
                    outs << message_to_json(cc.response, false) << "\n";
                }
                oq.output(cc.id, outs.str());
                // free the current slot
                cc.id = 0;
            }
        }
        if(have_input) {
            // get the next free slot
            ++active_calls;
            unsigned x = 0; while(x < concurrent_calls && ciq[x].id != 0) ++x;
            assert(x < concurrent_calls);
            call_info<INT, OUTT> &cc = ciq[x];
            // send the request
            cc.response.Clear();
            cc.contextp = std::unique_ptr<::grpc::ClientContext>(new ::grpc::ClientContext);
            if(set_async_calls) 
                cc.contextp->AddMetadata("overlapped-calls", async_calls? "1": "0");
            if(time_calls) 
                cc.contextp->AddMetadata("time-call", "1");
            for(auto const &xhe: headers)
                cc.contextp->AddMetadata(xhe.first, xhe.second);
            if(call_timeout > 0) {
                auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(call_timeout);
                cc.contextp->set_deadline(deadline);
            }
            cc.id = line_count;
            cc.request.CopyFrom(request);
            cc.carr = prepare_async(cc.contextp.get(), cc.request, &cq);
            cc.carr->StartCall();
            cc.carr->Finish(&cc.response, &cc.status, (void *) ((unsigned long)x+1));
        }
    }
    cq.Shutdown();
    while(cq.Next((void **) &slot, &ok));
    return error_count;
}
struct ansiesc_out {
    bool use_escapes;
    std::ostream &out;
    int c1_count = 0, c2_count = 0;
    char const *c1_begin =  "\x1b[38;5;4m", *c1_end = "\x1b[0m";
    char const *c2_begin = "\x1b[1m", *c2_end = "\x1b[0m";
    ansiesc_out(std::ostream &o, bool force_escapes=isatty(fileno(stderr)) && isatty(fileno(stdout))):out(o), use_escapes(force_escapes) {}
};
inline static ansiesc_out &operator <<(ansiesc_out &out, char const *s) {
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
inline static ansiesc_out &operator <<(ansiesc_out &out, V s) {
    out.out << s;
    return out;
}
static entry_info const *find_entry(std::string const &name) {
    if(name.empty()) return nullptr;
    entry_info const *p = nullptr, *pp = nullptr;
    std::vector<std::string> matches, prefix_matches;
    for(auto const &rme: entry_table) {
        std::string mn = rme.entry_full_name;
        if(mn == name || (mn.length() > name.length() + 1 && mn.substr(mn.length()-name.length()-1) == std::string(".")+name)) {
            matches.push_back(mn); 
            p = &rme;
        }
        std::string smn = mn.find_last_of('.') == std::string::npos? mn: mn.substr(mn.find_last_of('.')+1);
        if(smn.substr(0, name.length()) == name) {
            prefix_matches.push_back(mn);
            pp = &rme; 
        }
    }
    switch(matches.size()) {
        case 0:
            switch(prefix_matches.size()) {
                case 0: 
                    std::cerr << "Unknown method \"" << name << "\"\n";
                    break;
                case 1:
                    p = pp;
                    break;
                default:
                    matches = prefix_matches;
                    p = nullptr;
            }
            break;
        case 1: 
            break;
        default:
            p = nullptr;
            break;
    }
    if(p == nullptr && matches.size()) {
        std::cerr << "Ambiguous method name \"" << name << "\", matches ";
        for(unsigned u = 0; u+2 < matches.size(); ++u) std::cerr << "\"" << name << "\", ";
        std::cerr << "\"" << matches[matches.size()-2] << "\" and \"" << matches[matches.size()-1] << "\"\n";
    }
    return p;
}
static ansiesc_out &print_banner(ansiesc_out &out) {
    out << "{{NAME}} gRPC client\n" 
           "{{MAIN_FILE_SHORT}} ({{MAIN_FILE_TS}})\n" 
           "{{FLOWC_NAME}} {{FLOWC_VERSION}} ({{FLOWC_BUILD}})\n"
           "grpc " << grpc::Version() 
#ifdef GOOGLE_PROTOBUF_VERSION
        << ", protobuf " << int(GOOGLE_PROTOBUF_VERSION / 1000000) << "." << int((GOOGLE_PROTOBUF_VERSION % 1000000) / 1000) << "." << GOOGLE_PROTOBUF_VERSION % 1000
#endif
        << ", c-ares " << ares_version(nullptr) << "\n"
#if defined(__clang__)          
           "clang++ " << __clang_version__ << " (" << __cplusplus <<  ")\n"
#elif defined(__GNUC__) 
           "g++ " << __VERSION__ << " (" << __cplusplus << ")\n"
#else
#endif
    ;
    return out;
}
static struct option long_opts[] {
    { "async-calls",          no_argument, nullptr,         'a' },
    { "entries-proto",        no_argument, nullptr,         'E' },
    { "header",               required_argument, nullptr,   'H' },
    { "help",                 no_argument, nullptr,         'h' },
    { "ignore-grpc-errors",   no_argument, nullptr,         'g' },
    { "ignore-json-errors",   no_argument, nullptr,         'j' },
    { "input-schema",         required_argument, nullptr,   'I' },
    { "nodes-proto",          no_argument, nullptr,         'A' },
    { "output-schema",        required_argument, nullptr,   'O' },
    { "proto",                required_argument, nullptr,   'P' },
    { "show-headers",         no_argument, nullptr,         's' },
    { "streams",              required_argument, nullptr,   'n' },
    { "time-calls",           no_argument, nullptr,         't' },
    { "timeout",              required_argument, nullptr,   'T' },
    { "version",              no_argument, nullptr,         'v' },
    { nullptr,0,nullptr,0 }
};
static bool parse_command_line(int &argc, char **&argv, ansiesc_out &aout) {
    int ec = 0, ch;
    while((ch = getopt_long(argc, argv, "hvgjsta:n:T:H:", long_opts, nullptr)) != -1) {
        switch (ch) {
            case 'a': async_calls = stringtobool(optarg, async_calls); set_async_calls = true;break;
            case 'h': show_help = true; break;
            case 'g': ignore_grpc_errors = true; break;
            case 'j': ignore_json_errors = true; break;
            case 'v': show_version = true; break;
            case 's': show_headers = true; break;
            case 't': time_calls = true; break;
            case 'H': add_header(optarg); break;
            case 'n': 
                      concurrent_calls = atoi(optarg); 
                      if(concurrent_calls <= 0) {
                          aout << argv[0] << ": invalid number of streams -- '" << optarg << "'\n";
                          ++ec; 
                      }
                      break;
            case 'A': case 'E': 
                      show.push_back(std::make_pair(ch, std::string())); break;
            case 'I': case 'O': case 'P':
                      show.push_back(std::make_pair(ch, std::string(optarg))); break;
            case 'T': 
                      call_timeout = std::atoi(optarg); 
                      if(call_timeout < 0) {
                          aout << argv[0] << ": invalid timeout value -- '" << optarg << "'\n";
                          ++ec;
                      }
                      break;
            default:
                return false;
        }
    }
    argv[optind-1] = argv[0];
    argv+=optind-1;
    argc-=optind-1;
    return ec == 0;
}
int main(int argc, char *argv[]) {
    ansiesc_out aout(std::cout);
    show_help = argc == 1;
    if(!parse_command_line(argc, argv, aout) || show_help || show_version || (show.size() == 0 && (argc < 3 || argc > 6)) || show.size() != 0 && argc > 1) {
        unsigned ec = 0;
        char const *argv0 = strrchr(argv[0], '/')? strrchr(argv[0], '/')+1: argv[0];
        if(show_help || show_version) print_banner(aout);
        if(show_version) return 0;
        if(show_help) {
            aout << "\n"
                "USAGE\n" 
                "\t" << argv0 << " [OPTIONS] PORT|ENDPOINT [SERVICE.]RPC [JSONL-INPUT-FILE] [OUTPUT-FILE] [HEADERS-FILE]\n"
                "\t" << argv0 << " --input-schema|--output-schema|--proto [SERVICE.]RPC\n"
                "\t" << argv0 << " --entries-proto|--nodes-proto\n"
                "\n"
                "\tInput and output files default to <stdin> and <stdout> respectively. The headers file defaults to the output file.\n"
                "\n"
                "OPTIONS\n"
                "\t`--async-calls`, `-a` TRUE/FALSE\n\t\tOverride the asynchronous calls setting in the aggregator.\n\n"
                "\t`--entries-proto`\n\t\tOutput a proto file with the definition for all the entries. See also `--nodes-proto` and `--proto`.\n\n"
                "\t`--help`, `-h`\n\t\tDisplay this help and exit.\n\n"
                "\t`--header`, `-H` NAME=VALUE\n\t\tAdd this header to every request.\n\n"
                "\t`--ignore-grpc-errors`, `-g`\n\t\tKeep going when grpc errors are encountered.\n\n"
                "\t`--ignore-json-errors`, `-j`\n\t\tKeep going even if input 'JSON' fails conversion to 'protobuf.'\n\n"
                "\t`--input-schema` [SERVICE.]RPC\n\t\tShow the 'JSON' schema for the input for this 'RPC'.\n\n"
                "\t`--nodes-proto`\n\t\tOutput a proto file with the definition for all the entries and mnodes. See also `--entries-proto` and `--proto`.\n\n"
                "\t`--ouput-schema` [SERVICE.]RPC\n\t\tShow the 'JSON' schema for the output for 'RPC'.\n\n"
                "\t`---proto` [SERVICE.]RPC\n\t\tOutput a proto file with the definition for this 'RPC'.\n\t\tSee also `--entries-proto` and `--nodes-proto`.\n\n"
                "\t`--show-headers`, `-s`\n\t\tDisplay headers returned with the reply. By default headers are shown only if a headers file is given.\n\t\tIf this option is set and no header file is given, the headers will be displayed before each output.\n\n"
                "\t`--streams`, `-n` INTEGER\n\t\tNumber of concurrent calls to make through the connection. The default is '1'.\n\n"
                "\t`--time-calls`, `-t`\n\t\tRetrieve timing information for each call. The timing information is displayed in the 'times-bin' output header.\n\t\tSee `--show-headers` for more information.\n\n"
                "\t`--timeout`, `-T`  MILLISECONDS\n\t\tLimit each call to the given amount of time. The default is '3600000' (one hour). Set to '0' to disable timeouts.\n\n"
                "\t`--version`, `-v`\n\t\tDisplay version information.\n\n"
                "ENTRIES\n"
                "\tgRPC services implemented in the aggregator\n";
            for(auto const &mid: entry_table) if(mid.entry) {
                ++ec;
                aout << "\t\t'" << mid.entry_full_name << "'\n";
            }
            if(ec != entry_table.size()) {
                aout << "\nNODES\n\tgRPC services referenced by nodes\n";
                for(auto const &mid: entry_table) if(!mid.entry)
                    aout << "\t\t'" << mid.entry_full_name << "'\n";
            }
            aout << "\n";
        } else {
            aout << "Use `--help` to find the list of all valid options\n";
        }
        return show_help? 1: 0;
    }

    if(show.size() > 0) {
        for(auto const &se: show) {
            auto pi = find_entry(se.second);
            switch(se.first) {
                case 'I':
                    if(pi != nullptr) 
                        std::cout << pi->input_schema << "\n";
                    break;
                case 'O':
                    if(pi != nullptr) 
                        std::cout << pi->output_schema << "\n";
                    break;
                case 'P':
                    if(pi != nullptr) 
                        std::cout << pi->proto << "\n";
                    break;
                case 'E':
                    std::cout << {{ENTRIES_PROTO/c}} << "\n";
                    break;
                case 'A':
                    std::cout << {{ALL_NODES_PROTO/c}} << "\n";
                    break;
                default:
                    break;
            }
        }
        return 0;
    }

    std::string endpoint(strspn(argv[1], "0123456789") == strlen(argv[1])? std::string("localhost:")+argv[1]: std::string(argv[1]));
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

    std::shared_ptr<grpc::Channel> channel(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));

    std::istream *in = &std::cin;
    std::ifstream infs;
    std::string input_label("<stdin>");
    std::ostream *out = &std::cout;
    std::ofstream outfs;
    std::string output_label("<stdout>");

    if(argc >= 4 && strcmp("-", argv[3]) != 0) {
        infs.open(argv[3]);
        if(!infs.is_open()) {
            std::cerr << "Could not read file: " << argv[3] << "\n";
            return 1;
        }
        in = &infs;
        input_label = argv[3];
    }
    if(argc >= 5 && strcmp("-", argv[4]) != 0) {
        outfs.open(argv[4]);
        if(!outfs.is_open()) {
            std::cerr << "Could not write file: " << argv[4] << "\n";
            return 1;
        }
        out = &outfs;
        output_label = argv[4];
    }
    std::ostream *heads = out;
    std::ofstream headfs;
    std::string heads_label = output_label;
    if(argc >= 6 && strcmp(argv[4], argv[5]) != 0) {
        if(strcmp("-", argv[5]) == 0) {
            heads = &std::cout;
            heads_label = "<stdout>";
        } else {
            headfs.open(argv[5]);
            if(!headfs.is_open()) {
                std::cerr << "Could not write file: " << argv[5] << "\n";
                return 1;
            }
            heads = &headfs;
            heads_label = argv[5];
        }
    }

    output_queue oq(*out, output_label);
    output_queue hq(*heads, heads_label);
    int rc = 0;
    switch(eip->id) {
{I:MDP_FULL_NAME{        case {{MDP_FULL_NAME/id/upper}}: {
            // rpc {{MDP_FULL_NAME}}
            std::unique_ptr<{{MDP_SERVICE_NAME}}::Stub> client_stub({{MDP_SERVICE_NAME}}::NewStub(channel));
            auto prep_lambda  = [&client_stub](grpc::ClientContext *ctx, {{MDP_INPUT_TYPE}} &request, grpc::CompletionQueue *cq) -> std::unique_ptr<grpc::ClientAsyncResponseReader<{{MDP_OUTPUT_TYPE}}>> { 
                return client_stub->PrepareAsync{{MDP_NAME}}(ctx, request, cq);
            };
            rc = process_file<{{MDP_INPUT_TYPE}}, {{MDP_OUTPUT_TYPE}}, decltype(prep_lambda)>(concurrent_calls, input_label, *in, oq, hq, prep_lambda);
                    
        } break;
}I}
        default:
            break;
    }
    return rc;
}

