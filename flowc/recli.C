/**
 * REST client *** curl wrapper for concurrent REST requests
 *
 * c++ -O3 -std=c++11 -o recli recli.C $(pkg-config --libs  libcurl)
 */
 
#include <algorithm>
#include <chrono>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <curl/curl.h>
#include <unistd.h>
#include <getopt.h>

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

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::string *ps = (std::string *) userdata;
    ps->append(ptr, size*nmemb);
    return nmemb;
}

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    std::string *ps = (std::string *) userdata;
    ps->append(buffer, size*nitems);
    return nitems;
}

bool verbose_curl = false;
bool reuse_connections = true;
bool flush_outputs = false;
int poll_timeout_ms = 1000;
bool show_help = false;
int connections = 1;
std::vector<std::string> request_headers;
std::vector<std::tuple<std::string, std::string, unsigned, bool>> save_headers;
int call_timeout_seconds = 3600*24*100;
std::string call_id_label;  


struct call_data {
    std::chrono::system_clock::time_point deadline;
    unsigned long line_number;
    std::string start_time;
    std::string headers;
    std::string data;
    std::string request;
    char error_buffer[CURL_ERROR_SIZE];
    long response_code;
    struct curl_slist *slist1;
    CURL *ehd;
    bool free; 

    call_data(): response_code(0), slist1(nullptr), ehd(nullptr), free(true) {
        error_buffer[0] = 0;
    }
    ~call_data() {
        if(ehd != nullptr) curl_easy_cleanup(ehd);
        ehd = nullptr;
        if(slist1 != nullptr) curl_slist_free_all(slist1);
        slist1 = nullptr;
    }

    CURL *setup(std::string const &url, std::vector<std::string> const &request_headers, unsigned long line_n, std::string const &line) {
        request = line;
        line_number = line_n;
        if(ehd == nullptr) ehd = curl_easy_init();

        curl_easy_setopt(ehd, CURLOPT_BUFFERSIZE, 102400L);
        curl_easy_setopt(ehd, CURLOPT_URL, url.c_str());
        curl_easy_setopt(ehd, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(ehd, CURLOPT_USERAGENT, "florecli/0.11.5");
        curl_easy_setopt(ehd, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(ehd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
        curl_easy_setopt(ehd, CURLOPT_VERBOSE, verbose_curl? 1L: 0L);
        curl_easy_setopt(ehd, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(ehd, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(ehd, CURLOPT_HEADERFUNCTION, header_callback);

        slist1 = nullptr;
        for(auto const &h: request_headers) 
            slist1 = curl_slist_append(slist1, h.c_str());
        std::string call_id = std::string("x-flow-call-id: ") + call_id_label + std::to_string(line_n);
        slist1 = curl_slist_append(slist1, call_id.c_str());
        curl_easy_setopt(ehd, CURLOPT_HTTPHEADER, slist1);

        curl_easy_setopt(ehd, CURLOPT_POSTFIELDS, request.c_str());
        curl_easy_setopt(ehd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)request.length());

        curl_easy_setopt(ehd, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(ehd, CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(ehd, CURLOPT_HEADERDATA, &headers);
        data.clear();
        error_buffer[0] = '\0';
        headers.clear();
        start_time = get_system_time();
        deadline = std::chrono::system_clock::now() + std::chrono::seconds(call_timeout_seconds);
        free = false;
        return ehd;
    }
    void finish() {
        curl_easy_getinfo(ehd, CURLINFO_RESPONSE_CODE, &response_code);
        headers += "\r";
    }
    void clear(bool abort = false) {
        if(!reuse_connections || abort) {
            curl_easy_cleanup(ehd);
            ehd = nullptr;
        }
        curl_slist_free_all(slist1);
        slist1 = nullptr;
        free = true;
    }
    bool has_timed_out(std::chrono::system_clock::time_point now) const {
        return !free && deadline < now;
    }
};

static std::string::size_type ci_find(std::string const &str, std::string const &substr, std::string::size_type pos = 0) {
    if(pos == std::string::npos || pos > str.length() || str.length() < substr.length()) 
        return std::string::npos;
    std::string cstr(str); 
    std::transform(cstr.begin(), cstr.end(), cstr.begin(), ::toupper);
    std::string csubstr(substr);
    std::transform(csubstr.begin(), csubstr.end(), csubstr.begin(), ::toupper);
    return cstr.find(csubstr, pos);
}

struct write_q {
    unsigned long lines_written;
    std::vector<std::ostream *> const &outputs;
    std::vector<std::pair<unsigned long, std::vector<std::string>>> buffer;

    write_q(std::vector<std::ostream *> const &ov): lines_written(0), outputs(ov) {
    }

    void write_through(std::vector<std::string> const &data) {
        for(unsigned d = 0, e = save_headers.size(); d < e; ++d) {
            auto const &w = save_headers[d];
            std::ostream &out = *outputs[std::get<2>(w)];
            if(!std::get<3>(w)) out << "\t";
            if(d < data.size()) out << data[d];
        }
        for(unsigned o = 0, e = outputs.size(); o < e; ++o) if(outputs[0] != nullptr) {
            if(flush_outputs) 
                *outputs[o] << std::endl;
            else 
                *outputs[o] << "\n";
        }
    }
    unsigned long write() {
        if(buffer.size() < 0) return lines_written;
        std::sort(buffer.begin(), buffer.end());
        unsigned long trim_size = 0;
        for(unsigned i = 0, e = buffer.size(); i < e && lines_written + 1 == buffer[i].first; ++i) {
            auto const &ds = buffer[i];
            write_through(ds.second);
            ++trim_size; ++lines_written;
        }
        if(trim_size > 0) buffer.erase(buffer.begin(), buffer.begin() + trim_size);
        return lines_written;
    }
    unsigned long add(unsigned long line_number, std::vector<std::string> const &data) {
        buffer.emplace_back(std::make_pair(line_number, data));
        return write();
    }
    unsigned long add(unsigned long line_number, std::string const &line) {
        return add(line_number, std::vector<std::string>(&line, &line +1));
    }
    unsigned long add(call_data const &cc) {
        std::vector<std::string> data(save_headers.size());
        for(unsigned i = 0, e = save_headers.size(); i < e; ++i) {
            auto const &s = save_headers[i];
            auto &d = data[i];
            if(std::get<0>(s) == "@O") {
                d = cc.data;
            } else if(std::get<0>(s) == "@T") {
                d = cc.start_time + " " + get_system_time();
            } else if(std::get<0>(s) == "@R") {
                d = cc.headers.substr(0, cc.headers.find_first_of('\r'));
            } else if(std::get<0>(s) == "@C") {
                d = std::to_string(cc.response_code);
            } else {
                std::string ss = std::string("\r\n") + std::get<0>(s) + ":"; 
                auto ssp = ci_find(cc.headers, ss);
                if(ssp != std::string::npos) {
                    d = cc.headers.substr(ssp + ss.length(), cc.headers.find_first_of('\r', ssp + ss.length()) - (ssp + ss.length()));
                    d = d.substr(d.find_first_not_of("\t "));
                    d = d.substr(0, d.find_last_not_of("\t \r\n")+1);
                }
            }
        }
        return add(cc.line_number, data);
    }
};

int do_curls(std::vector<std::ostream *> const &ous, std::istream &ins, std::string const &url) {
    std::string line;

    std::vector<call_data> ccv(connections);
    write_q queue(ous);
    int active_ccs = 0;
    bool have_input = true;
    unsigned long line_count = 0;


    CURLcode ret;
    CURLM *mhd = curl_multi_init();

    if(mhd == nullptr) {
        std::cerr << "Failed to initialize curl\n";
        return 1;
    }

    int still_running = 0;
    while(have_input || active_ccs > 0) {
       
        // keep queueing while there are free slots and there is input
        if(have_input) while(active_ccs < connections && (have_input = !!std::getline(ins, line))) {
            ++line_count;
            auto b = line.find_first_not_of("\t\r\a\b\v\f ");
            if(b == std::string::npos || line[b] == '#') {
                queue.add(line_count, line);
                continue;
            }
            // queue this request
            for(auto &cc: ccv) if(cc.free) {
                std::cerr << "[" << line_count << "]: sending " << line.length() << " bytes\n";
                curl_multi_add_handle(mhd, cc.setup(url, request_headers, line_count, line));
                ++active_ccs;
                break;
            }
        }
        while(active_ccs == connections || (active_ccs > 0 && !have_input)) {

            CURLMcode mc = curl_multi_perform(mhd, &still_running);
            int numfds;

            if(mc == CURLM_OK) {
                // wait for activity or timeout
                mc = curl_multi_wait(mhd, nullptr, 0, poll_timeout_ms, &numfds);
            }
            if(mc != CURLM_OK) {
                std::cerr << "curl-error(" << mc << ") multi failed\n"; 
                break;
            }
            
            if(numfds == 0) {
                auto now = std::chrono::system_clock::now();
                for(auto &cc: ccv) if(cc.has_timed_out(now)) {
                    std::cerr << "[" << cc.line_number << "]: exceeded " << call_timeout_seconds << " seconds\n";
                    curl_multi_remove_handle(mhd, cc.ehd);
                    queue.add(cc.line_number, std::string("# timed out after ") + std::to_string(call_timeout_seconds));
                    --active_ccs;
                    cc.clear(true);
                }
                usleep(100000); 
            }

            struct CURLMsg *m;
            do {
                int msgq = 0;
                m = curl_multi_info_read(mhd, &msgq);
                if(m != nullptr && m->msg == CURLMSG_DONE) {
                    CURL *e = m->easy_handle;
                    ret = m->data.result;
                    curl_multi_remove_handle(mhd, e);

                    for(auto &cc: ccv) if(!cc.free && cc.ehd == e)  {
                        cc.finish();
                        if(ret != CURLE_OK)
                            std::cerr << "curl-error(" << ret << "): " << cc.error_buffer << "\n";
                        queue.add(cc);
                        cc.clear();
                        --active_ccs;
                        break;
                    }
                }
            } while(m != nullptr);
        }
    }

    curl_multi_cleanup(mhd);
    mhd = nullptr;
    return (int)ret;
}

static struct option long_opts[] {
    { "help",                 no_argument, nullptr, 'h' },
    { "verbose",              no_argument, nullptr, 'v' },
    { "connections",          required_argument, nullptr, 'n' },
    { "header",               required_argument, nullptr, 'H' },
    { "label",                required_argument, nullptr, 'l' },
    { "response",             required_argument, nullptr, 'r' },
    { "response-code",        required_argument, nullptr, 'R' },
    { "save",                 required_argument, nullptr, 's' },
    { "time",                 required_argument, nullptr, 't' },
    { "time-calls",           required_argument, nullptr, 'c' },
    { "timeout",              required_argument, nullptr, 'T' },
    { nullptr,                0,                 nullptr,  0 }
};

static bool parse_command_line(int &argc, char **&argv) {
    int ch;
    char const *sp = nullptr;
    while((ch = getopt_long(argc, argv, "hvn:H:l:r:R:s:t:c:T:", long_opts, nullptr)) != -1) {
        switch (ch) {
            case 'h': show_help = true; break;
            case 'v': verbose_curl = true; break;
            case 'n': connections = atoi(optarg); break;
            case 'H': request_headers.push_back(optarg); break;
            case 'l': call_id_label = optarg; break;
            case 'r': 
                save_headers.push_back(std::make_tuple<std::string, std::string, unsigned, bool>("@R", optarg, 0, true));
                break;
            case 'R': 
                save_headers.push_back(std::make_tuple<std::string, std::string, unsigned, bool>("@C", optarg, 0, true));
                break;
            case 's': 
                sp = strchr(optarg, ':');
                save_headers.push_back(sp == nullptr? 
                        std::make_tuple<std::string, std::string, unsigned, bool>(optarg, "", 0, true): 
                        std::make_tuple<std::string, std::string, unsigned, bool>(std::string((const char *) optarg, sp), sp+1, 0, true)); 
                break;
            case 't': 
                save_headers.push_back(std::make_tuple<std::string, std::string, unsigned, bool>("@T", optarg, 0, true)); 
                break;
            case 'c': 
                request_headers.push_back("x-flow-time-call: 1"); 
                save_headers.push_back(std::make_tuple<std::string, std::string, unsigned, bool>("x-flow-call-times", optarg, 0, true)); 
                break;
            case 'T': call_timeout_seconds = std::atoi(optarg); break;
            default:
                return false;
        }
    }
    argv[optind-1] = argv[0];
    argv+=optind-1;
    argc-=optind-1;
    return true;
}

int main(int argc, char *argv[]) {
    if(!parse_command_line(argc, argv) || show_help || argc > 4 || argc < 2) {
        std::cerr << "REST client -- curl wrapper for concurrent REST requests.\n";
        std::cerr << curl_version() << "\n";
        std::cerr << "\n";
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] URL [INPUT-FILE [OUTPUT-FILE]]\n";
        std::cerr << "    or " << argv[0] << " --help\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "  -n, --connections INTEGER   Number of concurrent calls to make\n";
        std::cerr << "  -H, --header STRING         Add header to the request\n";
        std::cerr << "  -h, --help                  Display this help\n";
        std::cerr << "  -l, --label STRING          Send string along with the line number in call ids\n";
        std::cerr << "  -r, --response FILE         Save the HTTP response into FILE\n";
        std::cerr << "  -R, --response-code FILE    Save the HTTP response code into FILE\n";
        std::cerr << "  -s, --save HEADER:FILE      Save value of HEADER into FILE\n";
        std::cerr << "  -t, --time FILE             Save wall clock time information into FILE\n";
        std::cerr << "  -c, --time-calls FILE       Save detailed time information into FILE\n";
        std::cerr << "  -T, --timeout SECONDS       Limit each call to the given amount of time\n";
        std::cerr << "  -v, --verbose               Enable curl's verbose flag\n";
        std::cerr << "\n";
        return show_help? 1: 0;
    }

    save_headers.insert(save_headers.begin(), argc < 4? 
            std::make_tuple<std::string, std::string, unsigned, bool>("@O", "", 0, true):
            std::make_tuple<std::string, std::string, unsigned, bool>("@O", argv[3], 0, true));
    
    std::map<std::string, unsigned> output_filenames;
    bool first_stdout = true;
    for(auto &s: save_headers) 
        if(!std::get<1>(s).empty() && std::get<1>(s) != "-") {
            auto ofp = output_filenames.find(std::get<1>(s));
            if(ofp == output_filenames.end()) {
                std::get<2>(s) = output_filenames.size() + 1;
                output_filenames.emplace(std::get<1>(s), std::get<2>(s));
            } else {
                std::get<2>(s) = ofp->second;
                std::get<3>(s) = false;
            }
        } else {
            std::get<3>(s) = first_stdout;
            first_stdout = false;
        }
/*
    for(auto &s: save_headers) {
        std::cerr << std::get<0>(s) << ", " << std::get<1>(s) << ", " << std::get<2>(s) << ", " << std::get<3>(s) << "\n";
    }
*/
    std::vector<std::ofstream> ofsv(output_filenames.size());
    std::vector<std::ostream *> outv(output_filenames.size()+1);
    outv[0] = first_stdout? nullptr: &std::cout;
    for(auto const &fni: output_filenames) {
        ofsv[fni.second-1].open(fni.first.c_str());
        outv[fni.second] = &ofsv[fni.second-1];
    }

    std::ifstream infs;
    std::istream *ins = &std::cin;
    if(argc >= 3 && argv[2][0] != '\0' && strcmp(argv[2], "-") != 0) {
        infs.open(argv[2]);  
        ins = &infs;
    }

    request_headers.push_back("content-type: application/json");
    int rc = do_curls(outv, *ins, argv[1]);
    return rc;
}
