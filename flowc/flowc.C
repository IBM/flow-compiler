#include <algorithm>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <ftw.h>
#include <iostream>
#include <map>
#include <set>
#include <signal.h>
#include <sstream>
#include <unistd.h>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "ansi-escapes.H"
#include "cot.H"
#include "filu.H"
#include "flow-compiler.H"
#include "grpc-helpers.H"
#include "helpo.H"
#include "stru1.H"
#include "vex.H"

#if defined(STACK_TRACE) && STACK_TRACE
#include <execinfo.h>
#endif

using namespace stru1;
/** 
 * Set this to false to turn off ANSI coloring 
 */
bool ansi::use_escapes = true;
bool flow_compiler::debug_enable = false;

static std::string install_directory;
static std::string output_directory;
std::string flow_compiler::output_filename(std::string const &filename) {
    if(output_directory.empty())
            return filename;
    if(*output_directory.rbegin() == '/') 
        return output_directory+filename;
    return output_directory+"/"+filename;
}
void FErrorPrinter::AddMessage(std::string const &type, std::string const &color, std::string const &filename, int line, int column, std::string const &message) {
    if(ansi::use_escapes)
        *outs << ANSI_BOLD;
    *outs << filename;
    if(line >= 0) *outs << "(" << line+1;
    if(line >= 0 && column >= 0) *outs << ":" << column+1;
    if(line >= 0) *outs << ")";
    *outs << ": " << color << type << ": ";
    if(ansi::use_escapes)
        *outs << ANSI_RESET;
    ansi::emphasize(*outs, message, ansi::escape(ANSI_BOLD,ANSI_GREEN));
    *outs << "\n";
}
void FErrorPrinter::AddError(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("error", ansi::escape(ANSI_RED), filename, line, column, message);
}
void FErrorPrinter::AddWarning(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("warning", ansi::escape(ANSI_MAGENTA), filename, line, column, message);
}
void FErrorPrinter::AddNote(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("note", ansi::escape(ANSI_BLUE), filename, line, column, message);
}
void ErrorPrinter::AddError(int line, int column, std::string const & message) {
    fperr.AddError(filename, line, column, message);
}
void handler(int sig) {
    void *array[10];
    size_t size;

#if defined(STACK_TRACE) && STACK_TRACE 
    // get void*'s for all entries on the stack
    size = backtrace(array, 10);
#endif
    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
#if defined(STACK_TRACE) && STACK_TRACE 
    backtrace_symbols_fd(array, size, STDERR_FILENO);
#endif
    exit(1);
}


char const *get_version();
char const *get_build_id();
char const *get_default_runtime();
std::set<std::string> available_runtimes();
#define FLOWC_NAME "flowc"

flow_compiler::flow_compiler(): pcerr(std::cerr), importer(&source_tree, &pcerr), trace_on(false), verbose(false), input_dp(nullptr) {
    input_label = "input";
    rest_port = -1;
    base_port = 53135;                  // the lowest it can be is 49152
    default_node_timeout = 180000;      // 3 minutes
    default_entry_timeout = 600000;     // 10 minutes
    default_maxcc = 16;                  
    default_repository = "/";
    runtime = get_default_runtime();

    set(global_vars, "FLOWC_BUILD", get_build_id());
    set(global_vars, "FLOWC_VERSION", get_version());
    set(global_vars, "BASE_IMAGE", runtime);
    set(global_vars, "FLOWC_NAME", FLOWC_NAME);
}
int flow_compiler::get_unna(std::string &name, int hint) {
    std::string nroot(name);
    int c = 0;
    if(cot::contains(glunna, name)) {
        name = sfmt() << nroot << hint;
        while(cot::contains(glunna, name) && c < 1999) 
            name = sfmt() << nroot << ++c; 
    }
    glunna[name] = 0;
    return c < 1999? 0: 1;
}
int flow_compiler::process(std::string const &input_filename, std::string const &orchestrator_name, std::set<std::string> const &targets, helpo::opts const &opts) {
    int error_count = 0;
    DEBUG_ENTER;
    main_name = orchestrator_name;
    set(global_vars, "INPUT_FILE", input_filename);
    set(global_vars, "MAIN_FILE_SHORT", stru1::basename(input_filename));
    set(global_vars, "MAIN_SCALE", "1");
    /****************************************************************
     * Add all the import directories to the search path - check if they are valid
     */ 
    protocc = sfmt() << "protoc --cpp_out=" << output_filename(".") << " ";
    grpccc = sfmt() << "protoc --grpc_out=" << output_filename(".") << " --plugin=protoc-gen-grpc=" << search_path("grpc_cpp_plugin");
    { 
        struct stat sb;
        if(stat(input_filename.c_str(), &sb) != 0 || S_ISDIR(sb.st_mode) || (sb.st_mode & S_IREAD) == 0) {
            ++error_count;
            pcerr.AddError(input_filename, -1, 0, "can't find or access file");
        }
        std::string dirname = output_filename(".");
        if(stat(dirname.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode) || (sb.st_mode & S_IWRITE) == 0) {
            ++error_count;
            pcerr.AddError(dirname, -1, 0, "can't find or write to directory");
        }
        main_file = basename(input_filename, "", &dirname);
        set(global_vars, "MAIN_FILE", main_file);
        source_tree.MapPath("", dirname);
        if(dirname.empty()) dirname = ".";
        grpccc += " -I"; grpccc += dirname; grpccc += " ";
        protocc += "-I"; protocc += dirname; protocc += " ";
    }
    
    for(auto const &path: opts["proto-path"]) 
        if(add_to_proto_path(path) != 0) {
            ++error_count;
            pcerr.AddError(path, -1, 0, "can't find or access directory");
        }

    input_label = opts.opt("input-label", input_label);
    if(error_count > 0) return error_count;
   
    /****************************************************************
     */
    clear(global_vars, "SERVER_XTRA_H");
    clear(global_vars, "SERVER_XTRA_C");
    {
        std::string real_input_filename;
        source_tree.VirtualFileToDiskFile(main_file, &real_input_filename);

        if(cot::contains(targets, "docs") || cot::contains(targets, "graph-files")) {
            std::string docs_directory = output_filename("docs");
            mkdir(docs_directory.c_str(), 0777);
            if(cot::contains(targets, "docs")) {
                // Copy the flow file into docs
                if(filu::realpath(output_filename(std::string("docs/")+main_file)) != filu::realpath(real_input_filename)) 
                    filu::cp_p(real_input_filename, output_filename(std::string("docs/")+main_file));
            }
        }
        struct stat a_file_stat;
        stat(real_input_filename.c_str(), &a_file_stat);
        char buffer[2048];
        buffer[std::strftime(buffer, sizeof(buffer)-1, "%a, %e %b %Y %T %z", std::localtime(&a_file_stat.st_mtime))] = '\0';
        set(global_vars, "MAIN_FILE_TS", buffer);

        std::string xtra_h = path_join(dirname(real_input_filename), orchestrator_name+"-xtra.H");
        if(cot::contains(targets, "server") && stat(xtra_h.c_str(), &a_file_stat) >= 0 && ((a_file_stat.st_mode & S_IFMT) == S_IFREG || (a_file_stat.st_mode & S_IFMT) == S_IFLNK)) {
            filu::cp_p(xtra_h, output_filename(orchestrator_name+"-xtra.H"));
            append(global_vars, "SERVER_XTRA_H", orchestrator_name+"-xtra.H"); 
        }
    }
    if(opts.have("htdocs")) {
        std::string htdocs_path(path_join(filu::gwd(), opts.opt("htdocs")));
        set(global_vars, "HTDOCS_PATH", htdocs_path);
    }
    set(global_vars, "PROTO_FILES_PATH", path_join(filu::gwd(), output_filename(".")));
    set(global_vars, "NAME", orchestrator_name);
    /****************************************************************
     * file names
     */
    std::string server_bin = orchestrator_name + "-server";
    std::string server_source = output_filename(server_bin + ".C");
    std::string client_bin = orchestrator_name + "-client";
    std::string client_source = output_filename(client_bin + ".C");
    std::string orchestrator_makefile = orchestrator_name + ".mak";
    std::string default_certificate = orchestrator_name + ".pem";
    std::string grpc_certificate = orchestrator_name + "-grpc.pem";
    std::string rest_certificate = orchestrator_name + "-rest.pem";

    error_count += parse();
    int imports = 0;
    if(opts.have("import")) for(auto filename: opts["import"]) {
        int ec = compile_proto(filename, ++imports);
        error_count += ec;
    }

    if(error_count == 0)
        error_count += compile(targets, opts.optb("ignore-imports", false));

    if(opts.have("print-graph")) {
        std::string entry(opts.opt("print-graph", ""));
        int en = 0;
        for(int n: *this) if(at(n).type == FTK_ENTRY) { 
            if(name(n) == entry || ends_with(name(n), std::string(".") + entry)) {
                en = n;
                break;
            }
        }
        if(en == 0) {
            ++error_count;
            pcerr.AddError(main_file, -1, 0, sfmt() << "entry \"" << entry << "\" was not found");
        } else {
            print_graph(std::cout, en);
        }
    }
    // Set the name attribute for all volume mounts
    std::map<std::string, int> mounts;
    for(int n: *this) if(at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) 
        for(int m: subtree(n)) if(at(m).type == FTK_MOUNT)  if(name.has(m)) {
            if(cot::contains(mounts, name(m))) {
                pcerr.AddWarning(main_file, at(m), sfmt() << "reuse of volume mount name \"" << name(m) << "\"");
                pcerr.AddNote(main_file, at(mounts[name(m)]), "first used here");
            }
            mounts[name(m)] = m;
        }
    for(int n: *this) if(at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) 
        for(int m: subtree(n)) if(at(m).type == FTK_MOUNT) {
            std::string von;
            if(!name.has(m)) do {
                von = "vo";
                get_unna(von, m);
                name.update(m, von);
            } while(cot::contains(mounts, von));
            mounts[name(m)] = m;
        }
    // Set the group attribute
    for(int n: *this) if(at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) {
        std::string group_name;
        error_count += get_block_s(group_name, n, "group", {FTK_STRING, FTK_INTEGER}, "");
        if(!group_name.empty()) {
            group.put(n, group_name);
            if(!cot::contains(glunna, group_name))
                glunna[group_name] = n;
        }
    }
    main_group_name = "main"; 
    MASSERT(get_unna(main_group_name) == 0) << "Failed to generate name " << main_group_name << "\n";
    set(global_vars, "MAIN_POD", main_group_name);
    set(global_vars, "MAIN_GROUP", main_group_name);
    for(int n: *this) if(at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) {
        if(!group.has(n))
            group.put(n, main_group_name);
    }
    if(opts.have("print-ast"))  
        print_ast(std::cout);
    if(opts.have("print-pseudocode"))
        print_pseudocode(std::cout);

    if(token_comment.size() > 0 && token_comment[0].first == 1) {
        main_description = token_comment[0].second;
        if(!main_description.empty())
            set(global_vars, "MAIN_DESCRIPTION", main_description);
    }
    /*******************************************************************
     * Set global level defines
     */
    error_count += set_def_vars(global_vars);
    auto referenced_nodes = get_all_referenced_nodes();

    /*******************************************************************
     * Override some of the vars set at compile time
     */
    base_port = opts.opti("base-port", base_port);
    rest_port = opts.opti("rest-port", rest_port);
    
    runtime = opts.opt("runtime", runtime);
    if(!cot::contains(available_runtimes(), runtime)) {
        std::cerr << "unknown runtime: " << runtime << ", available: " << stru1::join(available_runtimes(), ", ") << "\n";
        return ++error_count;
    }
    set(global_vars, "BASE_IMAGE", runtime);

    if(base_port == rest_port) {
        ++error_count;
        pcerr.AddError(main_file, -1, 0, sfmt() << "gRPC port and REST port are set to the same value: \"" << base_port << "\"");
    }

    if(rest_port < 0) rest_port = base_port -1;
    set(global_vars, "MAIN_PORT", std::to_string(base_port));
    set(global_vars, "REST_PORT", std::to_string(rest_port));

    default_node_timeout = get_time_value(opts.opt("default-client-timeout", std::to_string(default_node_timeout)+"ms"));
    default_entry_timeout = get_time_value(opts.opt("default-entry-timeout", std::to_string(default_entry_timeout)+"ms"));

    default_maxcc = opts.opti("default-client-calls", default_maxcc);

    orchestrator_tag = opts.opt("image-tag", "v1");
    orchestrator_image = opts.opt("image", to_lower(orchestrator_name)+":"+orchestrator_tag);
    orchestrator_debug_image = opts.optb("debug-image", false);
    orchestrator_no_rest = !opts.optb("rest-api", true);

    set(global_vars, "DEBUG_IMAGE", orchestrator_debug_image? "yes":"no");
    set(global_vars, "REST_API", orchestrator_no_rest? "no":"yes");
    set(global_vars, "IMAGE_TAG", orchestrator_tag);

    if(!orchestrator_image.empty()) {
        if(orchestrator_image.find_first_of('/') == std::string::npos) 
            orchestrator_pull_image = path_join(default_repository, orchestrator_image);
        orchestrator_image = basename(orchestrator_pull_image, "", &push_repository);
        if(*orchestrator_pull_image.begin() == '/') orchestrator_pull_image = orchestrator_pull_image.substr(1);
        if(push_repository == "/") push_repository.clear();
    }
    set(global_vars, "IMAGE", orchestrator_image);
    set(global_vars, "PULL_IMAGE", orchestrator_pull_image);
        
    push_repository = opts.opt("push-repository", push_repository);
    if(!push_repository.empty()) {
        if(*push_repository.rbegin() != '/') push_repository += '/';
        set(global_vars, "PUSH_REPO", push_repository);
    }

    for(auto const &s: opts["image-pull-secret"]) 
        image_pull_secrets.insert(s);
    
    clear(global_vars, "HAVE_IMAGE_PULL_SECRETS");
    for(auto const &s: image_pull_secrets) {
        append(global_vars, "IMAGE_PULL_SECRET", s);
        set(global_vars, "HAVE_IMAGE_PULL_SECRETS", "");
    }

    /********************************************************************
     * Generate C files for proto and grpc
     */
    if(error_count == 0 && cot::contains(targets, "protobuf-files")) {
        // Add the destination directory to the proto path
        // since imports are compiled by now 
        add_to_proto_path(output_filename(".")); 
        error_count += genc_protobuf(); 
    }

    if(error_count == 0 && cot::contains(targets, "grpc-files"))
        error_count += genc_grpc(); 

    // Prepare include statements with all the grpc and pb generated headers
    clear(global_vars, "PB_GENERATED_C");
    clear(global_vars, "PB_GENERATED_H");
    clear(global_vars, "GRPC_GENERATED_C");
    clear(global_vars, "GRPC_GENERATED_H");
    clear(global_vars, "PROTO_FILE");
    clear(global_vars, "PROTO_FILE_DESCRIPTION");

    for(auto fdp: fdps) {
        std::string file(fdp->name());
        std::vector<std::string> mns;
        for(int s = 0, sdc = fdp->service_count(); s < sdc; ++s) {
            auto const *sdp = fdp->service(s);
            for(int m = 0, mdc = sdp->method_count(); m < mdc; ++m) 
                mns.push_back(sdp->method(m)->full_name());
        }
        std::string basefn = remove_suffix(file, ".proto");
        append(global_vars, "PROTO_FILE", file);
        append(global_vars, "PROTO_FILE_DESCRIPTION", join(mns, ", ", " and "));
        append(global_vars, "PB_GENERATED_C", basefn+".pb.cc");
        append(global_vars, "PB_GENERATED_H", basefn+".pb.h");
        if(fdp->service_count() > 0) {
            append(global_vars, "GRPC_GENERATED_C", basefn+".grpc.pb.cc");
            append(global_vars, "GRPC_GENERATED_H", basefn+".grpc.pb.h");
        }
        std::string filename;
        source_tree.VirtualFileToDiskFile(file, &filename);
        std::ifstream protof(filename.c_str());
        std::stringstream buf;
        buf << protof.rdbuf();
        append(global_vars, "PROTO_FILE_YAMLSTR", c_escape(buf.str()));
        append(global_vars, "PROTO_FULL_PATH", filu::realpath(filename));

        // Copy all the proto files into docs
        if(cot::contains(targets, "docs")) {
            if(filu::realpath(output_filename(std::string("docs/")+file)) != filu::realpath(filename)) 
                filu::cp_p(filename, output_filename(std::string("docs/")+file));
        }
    }
    // Generate lists for all client node + containers with:
    // name, group, port, image, endpoint, runtime, extern-node 
    std::map<std::string, std::set<int>> groups;
    std::map<std::string, std::map<int, int>> ports;
    clear(global_vars, "HAVE_COS");
    clear(global_vars, "HAVE_VOLUMES");
    bool have_cos = false, have_volumes = false;
    for(int n: *this) if(method_descriptor(n) != nullptr && at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) {
        append(global_vars, "XCLI_NODE_NAME", name(n));
        append(global_vars, "NODE_NAME", name(n));
        append(global_vars, "NODE_GROUP", group(n));
        groups[group(n)].insert(n);
        int pv = 0;
        error_count += get_block_i(pv, n, "port", 0);
        if(pv != 0 && cot::contains(targets, "driver")) {
            
            if(cot::contains(ports[group(n)], pv)) {
                pcerr.AddWarning(main_file, at(n), sfmt() << "port value \"" << pv << "\" for \"" << name(n) << "\" already used by \"" << name(ports[group(n)][pv]) << "\" in the same group");
                pcerr.AddNote(main_file, at(get_first_value_node(get_ne_block_node(ports[group(n)][pv]), "port")), "first used here");
            } else {
                ports[group(n)][pv] = n;
            }
        }

        std::map<std::string, std::string> minfo;
        for(int m: subtree(n)) if(at(m).type == FTK_MOUNT) {
            minfo.clear();
            error_count += get_mount_info(minfo, m);
            have_volumes = true;
            append(global_vars, "VOLUME_NAME", minfo["name"]);
            append(global_vars, "VOLUME_LOCAL", minfo["local"]);
            append(global_vars, "VOLUME_COS", minfo["url"]);
            if(!minfo["url"].empty())
                have_cos = true;
            append(global_vars, "VOLUME_SECRET", minfo["secret"]);
            append(global_vars, "VOLUME_PVC", minfo["pvc"]);
            append(global_vars, "VOLUME_ISRO", minfo["access"] == "ro"? "1": "0");
            append(global_vars, "VOLUME_ACCESS", minfo["access"]);
            if(description.has(get_previous_sibling(n))) {
                append(global_vars, "VOLUME_COMMENT", description(get_previous_sibling(n)));
            } else {
                append(global_vars, "VOLUME_COMMENT", "");
            }
        }
        
        int value = 0;
        bool external_node = false;
    }
    if(have_volumes) 
        set(global_vars, "HAVE_VOLUMES", "");
    if(have_cos)
        set(global_vars, "HAVE_COS", "");

    // Make sure the rest volume name doesn't collide with any other volume name
    std::string htdocs_volume_name = "htdocs";
    get_unna(htdocs_volume_name);
    set(global_vars, "HTDOCS_VOLUME_NAME", htdocs_volume_name);

    // Check/generate the entry list for the rest gateway
    // Do this if we have rest and generated config files
    if(cot::contains(targets, "server")) {
        // use a set to avoid duplicates
        std::set<std::string> rest_entries(all(global_vars, "REST_ENTRY").begin(), all(global_vars, "REST_ENTRY").end()); 
        for(int n: *this) if(at(n).type == FTK_ENTRY) {
            std::string method = name(n);
            auto mdp = check_method(method, 0);
            int blck = get_ne_block_node(n), pv = 0;
            for(int p = 0, v = find_in_blck(blck, "path", &p); v != 0; v = find_in_blck(blck, "path", &p)) {
                if(at(v).type != FTK_STRING && at(v).type != FTK_STRING) {
                    error_count += 1;
                    pcerr.AddError(main_file, at(v), "path must be a string");
                    continue;
                }
                rest_entries.insert(sfmt() << mdp->full_name() << "=" << get_string(v));
                pv = v;
            }
            if(pv == 0) 
                rest_entries.insert(mdp->full_name());
        }
        set_all(global_vars, "REST_ENTRY", rest_entries.begin(), rest_entries.end());
    }
    // Generic entry and client node info. 
    // Note that client here is not necessarily an instantiated node.
    error_count += set_entry_vars(global_vars);
    error_count += set_cli_node_vars(global_vars);


    if(error_count == 0 && opts.have("print-proto")) for(auto pt: opts["print-proto"]) {
        if(pt == ".") {
            vex::expand(std::cout, "// {{NAME}} -- entries and nodes\n{{ALL_NODES_PROTO}}\n", vex::make_smap(global_vars));
        } else if(pt == "-") {
            vex::expand(std::cout, "// {{NAME}} -- entries\n{{ENTRIES_PROTO}}\n", vex::make_smap(global_vars));
        } else {
            // look for a matching entry name
            int en = 0;
            for(int n: *this) if(at(n).type == FTK_ENTRY) { 
                if(name(n) == pt || ends_with(name(n), std::string(".") + pt)) {
                    en = n;
                    break;
                }
            }
            if(en != 0) {
                std::cout << "// " << pt << "\n";
                std::cout << gen_proto(method_descriptor(en)) << "\n";
                continue;
            } 
            std::vector<int> ven;
            std::set<MethodDescriptor const *> ms;
            for(int n: *this) if(at(n).type == FTK_NODE) { 
                if((type(n) == pt || name(n) == pt) && method_descriptor(n) != nullptr) {
                    ven.push_back(n);
                    ms.insert(method_descriptor(n));
                }
            }
            if(ven.size() == 0) {
                ++error_count;
                pcerr.AddError(main_file, -1, 0, sfmt() << "\"" << pt << "\" does not match any entry or node");
                break;
            } else if(ms.size() > 1) {
                ++error_count;
                pcerr.AddError(main_file, -1, 0, sfmt() << "\"" << pt << "\" matches more than one node");
                std::vector<std::string> mids; 
                for(int n: ven) mids.push_back(name(n));
                pcerr.AddNote(main_file, -1, 0, sfmt() << "matches " << stru1::join(mids, ", ", " and ")); 
                break;
            } 
            std::vector<std::string> mids; 
            for(int n: ven) mids.push_back(name(n));
            std::cout << "// " << stru1::join(mids, ", ", " and ") << "\n"; 
            std::cout << gen_proto(*ms.begin()) << "\n";
        }
    }

    if(error_count == 0 && cot::contains(targets, "server")) 
        error_count += genc_cc_server(server_source);
    
    if(error_count == 0 && cot::contains(targets, "client")) 
        error_count += genc_cc_client(client_source);

    if(error_count == 0 && cot::contains(targets, "ssl-certificates")) {
        std::string o_default_certificate = opts.opt("default-certificate", "");
        std::string o_grpc_certificate = opts.opt("grpc-certificate", o_default_certificate);
        std::string o_rest_certificate = opts.opt("rest-certificate", o_default_certificate);
        extern char const *template_server_pem;

        if(o_grpc_certificate == o_rest_certificate) {
            if(filu::write_file(output_filename(default_certificate), o_grpc_certificate, template_server_pem)) {
                ++error_count;
                pcerr.AddError(output_filename(default_certificate), -1, 0, "failed to write ssl certificate file");
            }
            append(global_vars, "GRPC_CERTIFICATE", default_certificate);
            append(global_vars, "REST_CERTIFICATE", default_certificate);
        } else {
            if(filu::write_file(output_filename(grpc_certificate), o_grpc_certificate, template_server_pem)) {
                ++error_count;
                pcerr.AddError(output_filename(grpc_certificate), -1, 0, "failed to write grpc ssl certificate file");
            }
            if(filu::write_file(output_filename(rest_certificate), o_rest_certificate, template_server_pem)) {
                ++error_count;
                pcerr.AddError(output_filename(rest_certificate), -1, 0, "failed to write rest ssl certificate file");
            }
            append(global_vars, "GRPC_CERTIFICATE", grpc_certificate);
            append(global_vars, "REST_CERTIFICATE", rest_certificate);
        }
    }
    if(error_count == 0 && cot::contains(targets, "makefile")) 
        error_count += genc_makefile(orchestrator_makefile);

    if(error_count == 0 && cot::contains(targets, "dockerfile"))
        error_count += genc_dockerfile(orchestrator_name);
    
    if(error_count == 0 && cot::contains(targets, "build-image")) {
        std::string makec = sfmt() << "cd " << output_filename(".") << " && make -f " << orchestrator_makefile 
            << (orchestrator_debug_image? " DBG=yes": "") 
            << (orchestrator_no_rest? " REST=no": "") 
            << " image";
        if(system(makec.c_str()) != 0) {
            pcerr.AddError(main_file, -1, 0, "failed to build docker image");
            ++error_count;
        }
    }
    if(error_count == 0 && (cot::contains(targets, "build-server") || cot::contains(targets, "build-client"))) {
        std::string makec = sfmt() << "cd " << output_filename(".")  << " && make -f " << orchestrator_makefile 
            << (orchestrator_debug_image? " DBG=yes": "") 
            << (orchestrator_no_rest? " REST=no": "") 
            << " ";
        if(cot::contains(targets, "build-server") && cot::contains(targets, "build-client")) 
            makec += "-j2 all";
        else if(cot::contains(targets, "build-server"))
            makec += "server";
        else 
            makec += "client";
        if(system(makec.c_str()) != 0) {
            pcerr.AddError(main_file, -1, 0, "failed to build");
            ++error_count;
        }
    }
    if(error_count == 0 && cot::contains(targets, "python-client")) 
        error_count += genc_py_client(output_filename(client_bin + ".py"));
    if(error_count == 0 && cot::contains(targets, "driver")) 
        error_count += genc_deployment_driver(output_filename(orchestrator_name + "-dd.sh"));
    if(error_count == 0 && cot::contains(targets, "graph-files")) 
        error_count += genc_graph(cot::contains(targets, "svg-files"));
    if(error_count == 0 && cot::contains(targets, "www-files") && !orchestrator_no_rest) 
        error_count += genc_www();
    
    if(error_count != 0) 
        pcerr.AddNote(main_file, -1, 0, sfmt() << error_count << " error(s) during compilation");
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_www() {
    int error_count = 0;
    DEBUG_ENTER;
    // Make sure output directory exists
    std::string www_directory = output_filename("www");
    mkdir(www_directory.c_str(), 0777);

    std::string outputfn = output_filename("www/index.html");
    OFSTREAM_SE(outf, outputfn);

    if(DEBUG_GENC) {
        outputfn += ".json";
        OFSTREAM_SE(outj, outputfn);
        stru1::to_json(outj, global_vars);
    }
    if(error_count == 0) {
        extern char const *template_index_html;
        vex::expand(outf, template_index_html, vex::make_smap(global_vars));
    }
    for(int n: get_all_referenced_nodes()) if(method_descriptor(n) != nullptr) {
        decltype(global_vars) local_vars;
        set_cli_active_node_vars(local_vars, n);
        std::string outputfn = output_filename("www/"+stru1::to_identifier(stru1::to_option(name(n)))+"-index.html");
        OFSTREAM_SE(outf, outputfn);
        if(DEBUG_GENC) {
            outputfn += ".json";
            OFSTREAM_SE(outj, outputfn);
            stru1::to_json(outj, local_vars);
        }
        if(error_count == 0) {
            extern char const *template_index_html;
            vex::expand(outf, template_index_html, vex::make_cmap(vex::make_smap(local_vars), vex::make_smap(global_vars)));
        }
    }
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_makefile(std::string const &makefile_name) {
    int error_count = 0;
    DEBUG_ENTER;
    std::string fn = output_filename(makefile_name);
    OFSTREAM_SE(makf, fn);
    if(error_count == 0) {
        extern char const *template_Makefile;
        vex::expand(makf, template_Makefile, vex::make_smap(global_vars));
    }
    // Create a link to this makefile if Makefile isn't in the way
    std::string mp = output_filename("Makefile");
    std::string od = output_filename(".");
    int output_fd = open(od.c_str(), O_DIRECTORY);
    // Ignore the error
    if(0 != symlinkat(makefile_name.c_str(), output_fd, "Makefile")) {
    }
    close(output_fd);
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_dockerfile(std::string const &orchestrator_name) {
    int error_count = 0;
    DEBUG_ENTER;
    auto global_smap = vex::make_smap(global_vars);
    std::string fn = output_filename(orchestrator_name+".Dockerfile");
    OFSTREAM_SE(outf, fn);
    if(error_count == 0) {
        extern std::map<std::string, char const *> template_runtime_Dockerfile;
        char const *c_template_runtime_Dockerfile = template_runtime_Dockerfile.find(runtime)->second;
        vex::expand(outf, c_template_runtime_Dockerfile, global_smap);
        extern char const *template_Dockerfile;
        vex::expand(outf, template_Dockerfile, global_smap);
    }
    std::string fn2 = output_filename(orchestrator_name+".slim.Dockerfile");
    OFSTREAM_SE(outf2, fn2);
    if(error_count == 0) {
        extern char const *template_slim_Dockerfile;
        vex::expand(outf2, template_slim_Dockerfile, global_smap);
    }
    DEBUG_LEAVE;
    return error_count;
}
extern char const *template_help, *template_syntax;
static std::map<std::string, std::vector<std::string>> all_targets = {
    {"dockerfile",        {"makefile"}},
    {"client",            {"grpc-files", "makefile", "dockerfile"}},
    {"server",            {"grpc-files", "makefile", "dockerfile", "www-files", "ssl-certificates"}},
    {"grpc-files",        {"protobuf-files"}},
    {"build-client",      {"client"}},
    {"python-client",     {}},
    {"build-server",      {"server"}},
    {"build-image",       {"server", "client", "docs"}},
    {"makefile",          {}},
    {"driver",            {}},
    {"ssl-certificates",  {}},
    {"protobuf-files",    {}},
    {"www-files",         {}},
};
static std::set<std::string> can_use_tempdir = {
    "build-image" /*, "build-server", "build_client"*/
};
static int remove_callback(const char *pathname, const struct stat *, int, struct FTW *) {
    return remove(pathname);
} 
void show_builtin_help(std::ostream &);
int main(int argc, char *argv[]) {
    signal(SIGABRT, handler);
    signal(SIGSEGV, handler);
    helpo::opts opts;
    int main_argc = argc;
    if(opts.parse(template_help, argc, argv) != 0 || opts.have("version") || opts.have("help") || opts.have("help-syntax") || argc != 2) {
        ansi::use_escapes = opts.optb("color", ansi::use_escapes && isatty(fileno(stdout)) && isatty(fileno(stderr)));
        if(opts.have("help-syntax")) {
            std::cout << ansi::emphasize(template_syntax, ansi::escape(ANSI_BOLD, ANSI_GREEN), ansi::escape(ANSI_BOLD, ANSI_MAGENTA)) << "\n\n";
            std::ostringstream out;
            show_builtin_help(out);
            std::cout << "Built in functions:\n\n";
            std::cout << ansi::emphasize(out.str(), ansi::escape(ANSI_BOLD, ANSI_GREEN), ansi::escape(ANSI_BOLD, ANSI_MAGENTA)) << "\n";
            return 0;
        } else if(opts.have("version")) {
            std::cout << FLOWC_NAME << " " << get_version() << " (" << get_build_id() << ")\n";
            std::cout << "grpc " << grpc::Version() << "\n";
#if defined(__clang__)          
            std::cout << "clang++ " << __clang_version__ << " (" << __cplusplus << ")\n";
#elif defined(__GNUC__) 
            std::cout << "g++ " << __VERSION__ << " (" << __cplusplus << ")\n";
#else
#endif
            if(available_runtimes().size() == 1) {
                std::cout << "runtime: " << get_default_runtime() << "\n";
            } else {
                std::cout << "default runtime: " << get_default_runtime() << "\n";
                std::cout << "available runtimes: " << stru1::join(available_runtimes(), ", ") << "\n";
            }
            return 0;
        } else if(opts.have("help") || main_argc == 1) {
            ansi::emphasize(std::cout, ansi::emphasize(template_help, ansi::escape(ANSI_BLUE)), ansi::escape(ANSI_BOLD), "-", " \r\n\t =,;/", true, true) << "\n";
            return opts.have("help")? 0: 1;
        } else {
            if(argc != 2) 
                std::cout << "Invalid number of arguments\n";
            std::cout << "Use --help to see the command line usage and all available options\n\n";
            return 1;
        }
    }
    // Make sure we use appropriate coloring for errors
    ansi::use_escapes = opts.optb("color", ansi::use_escapes && isatty(fileno(stderr)) && isatty(fileno(stdout)));
    // Debug flag
    flow_compiler::debug_enable = opts.optb("debug", flow_compiler::debug_enable);
    std::set<std::string> targets;
    bool use_tempdir = true;
    for(auto kt: all_targets) 
        if(opts.have(kt.first)) {
            targets.insert(kt.first);
            use_tempdir = use_tempdir && cot::contains(can_use_tempdir, kt.first);
            targets.insert(kt.second.begin(), kt.second.end());
        }
    // Add all the dependent targets to the target list
    while(true) {
        auto stargets = targets.size();
        std::set<std::string> tmp(targets.begin(), targets.end());
        for(auto const &t: tmp) {
            if(cot::contains(targets, t)) {
                auto dt = all_targets.find(t);
                if(dt != all_targets.end())
                    targets.insert(dt->second.begin(), dt->second.end());
            }
        }
        if(stargets == targets.size()) 
            break;
    }
    flow_compiler gfc;
    gfc.trace_on = opts.have("trace");

    std::string orchestrator_name(opts.opt("name", basename(argv[1], ".flow")));
    output_directory = opts.opt("output-directory", ".");
    install_directory = output_directory;

    if(orchestrator_name == argv[1]) {
        std::cerr << "The orchestrator name is the same as the input file, use --name to change\n";
        return 1;
    }
    use_tempdir = use_tempdir && targets.size() > 0;
    if(use_tempdir) {
        char path[4096];
        char const *tmpdir = getenv("TMPDIR");
        if(tmpdir != nullptr) {
            strcpy(path, tmpdir);
        } else {
            strcpy(path, "/tmp");
        }
        if(filu::is_dir(path)) {
            unsigned dirlen = strlen(path);
            if(path[0] && path[dirlen-1] != '/') {
                path[dirlen] = '/'; path[++dirlen] = '\0';
            }
        } else {
            path[0] = '\0';
        }
        strcat(path, "flowcXXXXXX");
        char const *tmp_dirname = mkdtemp(path);
        if(tmp_dirname == nullptr) {
            perror("error: ");
            return 1;
        }
        output_directory = tmp_dirname;
    }
    int rc = gfc.process(argv[1], orchestrator_name, targets, opts);
    if(use_tempdir && nftw(output_directory.c_str(), remove_callback, FOPEN_MAX, FTW_DEPTH | FTW_MOUNT | FTW_PHYS) == -1) {
        std::cerr << output_directory;
        perror(": ");
        return 1;
    }
    return rc;
}
