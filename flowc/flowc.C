#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <grpcpp/grpcpp.h>
#include "ansi-escapes.H"
#include "flow-compiler.H"
#include "helpo.H"
#include "stru1.H"
#include "cot.H"
#include "vex.H"
#include "grpc-helpers.H"

#include <stdio.h>
#if defined(STACK_TRACE) && STACK_TRACE
#include <execinfo.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <ftw.h>
#include <errno.h>

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

bool is_dir(char const *fn) {
	struct stat sb;
    if(stat(fn, &sb) == -1) 
		return false;	
    return (sb.st_mode & S_IFMT) == S_IFDIR;
}

void chmodx(std::string const &fn) {
	struct stat sb;
    char const *fpath = fn.c_str();
    if(stat(fpath, &sb) == -1) 
		return;	

	mode_t chm = sb.st_mode;
    if(chm & S_IRUSR) chm = chm | S_IXUSR;
    if(chm & S_IRGRP) chm = chm | S_IXGRP;
    if(chm & S_IROTH) chm = chm | S_IXOTH;
    if(chm != sb.st_mode)
        chmod(fpath, chm);
}
std::string realpath(std::string const &f) {
    if(f.empty()) return "";
    char actualpath[PATH_MAX+1];
    auto r = realpath(f.c_str(), actualpath);
    return (r == nullptr) ? "" : r;
}
int cp_p(std::string const &source, std::string const &dest) {
    struct stat a_file_stat, b_file_stat;
    if(stat(source.c_str(), &a_file_stat) != 0 || S_ISDIR(a_file_stat.st_mode)) 
        return 1;
    if(stat(dest.c_str(), &b_file_stat) == 0 && a_file_stat.st_dev == b_file_stat.st_dev && a_file_stat.st_ino == b_file_stat.st_ino)
        return 0;
    bool ok = true;
    char buf[16384];
    size_t size;
    int sd = open(source.c_str(), O_RDONLY, 0);
    int dd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    while((size = read(sd, buf, sizeof(buf))) > 0) 
        ok = write(dd, buf, size) == size && ok;
    close(sd); close(dd);
    struct timeval tv[2];
    tv[0].tv_sec = a_file_stat.st_atime; tv[0].tv_usec = 0;
    tv[1].tv_sec = a_file_stat.st_mtime; tv[1].tv_usec = 0;
    utimes(dest.c_str(), &tv[0]);
    return ok? 0: 1;
}
int write_file(std::string const &fn, std::string const &source_fn, char const *default_content) {
    if(!source_fn.empty()) 
        return cp_p(source_fn, fn);
    std::ofstream outf(fn.c_str());
    if(!outf.is_open()) 
        return 1;
    return outf.write(default_content, strlen(default_content))? 0: 1;
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
/**
 * Add the directory to the proto path
 * Return 0 on success or 1 if can't access directory
 */
int flow_compiler::add_to_proto_path(std::string const &directory) {
    struct stat sb;
    if(stat(directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) 
        return 1;
    source_tree.MapPath("", directory);
    grpccc += "-I"; grpccc += directory; grpccc += " ";
    protocc += "-I"; protocc += directory; protocc += " ";
    return 0;
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
                if(realpath(output_filename(std::string("docs/")+main_file)) != realpath(real_input_filename)) 
                    cp_p(real_input_filename, output_filename(std::string("docs/")+main_file));
            }
        }
        
        struct stat a_file_stat;
        stat(real_input_filename.c_str(), &a_file_stat);
        char buffer[2048];
        buffer[std::strftime(buffer, sizeof(buffer)-1, "%a, %e %b %Y %T %z", std::localtime(&a_file_stat.st_mtime))] = '\0';
        set(global_vars, "MAIN_FILE_TS", buffer);

        std::string xtra_h = path_join(dirname(real_input_filename), orchestrator_name+"-xtra.H");
        if(cot::contains(targets, "server") && stat(xtra_h.c_str(), &a_file_stat) >= 0 && ((a_file_stat.st_mode & S_IFMT) == S_IFREG || (a_file_stat.st_mode & S_IFMT) == S_IFLNK)) {
            cp_p(xtra_h, output_filename(orchestrator_name+"-xtra.H"));
            append(global_vars, "SERVER_XTRA_H", orchestrator_name+"-xtra.H"); 
        }
    }
    if(opts.have("htdocs")) {
        std::string htdocs_path(path_join(gwd(), opts.opt("htdocs")));
        set(global_vars, "HTDOCS_PATH", htdocs_path);
    }

    set(global_vars, "PROTO_FILES_PATH", path_join(gwd(), output_filename(".")));
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
    if(error_count == 0)
        error_count += compile(targets);

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
    std::map<std::string, int> mns;
    for(int n: *this) if(at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) 
        for(int m: subtree(n)) if(at(m).type == FTK_MOUNT)  if(name.has(m)) {
            if(cot::contains(mns, name(m))) {
                pcerr.AddWarning(main_file, at(m), sfmt() << "reuse of volume mount name \"" << name(m) << "\"");
            }
            mns[name(m)] = m;
        }
    for(int n: *this) if(at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) 
        for(int m: subtree(n)) if(at(m).type == FTK_MOUNT) {
            std::string von;
            if(!name.has(m)) do {
                von = "vo";
                get_unna(von, m);
                name.update(m, von);
            } while(cot::contains(mns, von));
            mns[name(m)] = m;
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
    clear(global_vars, "HAVE_DEFN");
    for(int i: *this) if(at(i).type == FTK_DEFINE) {
        auto &defn = at(i);
        append(global_vars, "DEFN", get_id(defn.children[0]));
        append(global_vars, "DEFV", get_value(defn.children[1]));
        append(global_vars, "DEFD", description(defn.children[0]));
        append(global_vars, "DEFT", at(defn.children[1]).type == FTK_STRING? "STRING": (at(defn.children[1]).type == FTK_INTEGER? "INTEGER": "FLOAT"));
        set(global_vars, "HAVE_DEFN", "");
    }

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
     * All files are compiled at this point so,
     * set as many of values that control code generation as possible
     */
    if(cot::contains(targets, "driver")) {
        /*
        for(int n: *this) if(at(n).type == FTK_CONTAINER) {
            referenced_nodes.emplace(name(n), node_info(name(n), n));
        }
        */
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
        append(global_vars, "PROTO_FULL_PATH", realpath(filename));

        // Copy all the proto files into docs
        if(cot::contains(targets, "docs")) {
            if(realpath(output_filename(std::string("docs/")+file)) != realpath(filename)) 
                cp_p(filename, output_filename(std::string("docs/")+file));
        }
    }

    // Generate lists for all client node + containers with:
    // name, group, port, image, endpoint, runtime, extern-node 
    std::map<std::string, std::set<int>> groups;
    std::map<std::string, std::map<int, int>> ports;
    for(int n: *this) if(method_descriptor(n) != nullptr && at(n).type == FTK_NODE || at(n).type == FTK_CONTAINER) {
        std::string group_name;
        error_count += get_block_s(group_name, n, "group", {FTK_STRING, FTK_INTEGER}, "");
        append(global_vars, "XCLI_NODE_NAME", name(n));
        append(global_vars, "NODE_NAME", name(n));
        append(global_vars, "NODE_GROUP", group_name);
        groups[group_name].insert(n);
        int pv = 0;
        error_count += get_block_i(pv, n, "port", 0);
        if(pv != 0 && cot::contains(targets, "driver")) {
            
            if(cot::contains(ports[group_name], pv)) {
                pcerr.AddWarning(main_file, at(n), sfmt() << "port value \"" << pv << "\" for \"" << name(n) << "\" already used by \"" << name(ports[group_name][pv]) << "\" in the same group");
                pcerr.AddNote(main_file, at(get_first_value_node(get_ne_block_node(ports[group_name][pv]), "port")), "first used here");
            } else {
                ports[group_name][pv] = n; 
            }
                
        }
        
        int value = 0;
        bool external_node = false;
        /*
        std::string endpoint, image_name;
        error_count += get_block_value(value, n, "image", false, {FTK_STRING});
        if(value <= 0 || get_string(value).empty()) {
            error_count += get_block_value(value, n, "endpoint", false, {FTK_STRING});
            if(value <= 0 || get_string(value).empty()) {
                ++error_count;
                pcerr.AddError(main_file, at(n), sfmt() << "node \"" << name(n) << "\" must have either an image or an endpoint defined");
            } else {
                endpoint = get_string(value);
                external_node = true;
            }
        } else {
            image_name = get_string(value);
            if(image_name[0] == '/') 
                image_name = path_join(default_repository, image_name.substr(1));
        }

        append(global_vars, "NODE_IMAGE", image_name);
        append(group_vars[group_name], "G_NODE_IMAGE", image_name);
        append(global_vars, "NODE_ENDPOINT", endpoint);
        append(group_vars[group_name], "G_NODE_ENDPOINT", endpoint);
        if(external_node) {
            append(global_vars, "EXTERN_NODE", "#");
            append(group_vars[group_name], "G_EXTERN_NODE", "#");
        } else {
            append(global_vars, "EXTERN_NODE", "");
            append(group_vars[group_name], "G_EXTERN_NODE", "");
        }
*/
        // Volume mounts
        int old_value = 0;
        //ni.mounts.clear();
        int blck = get_ne_block_node(n);
        /*
        if(!external_node) for(int p = 0, v = find_in_blck(blck, "mount", &p); v != 0; v = find_in_blck(blck, "mount", &p)) {
            if(at(v).type != FTK_MOUNT) {
                error_count += 1;
                pcerr.AddError(main_file, at(v), "mount must be a labeled name/value pair block");
                continue;
            }
            std::string mount_name = get_id(at(v).children[0]);
            v = at(v).children[1];

            bool read_write = false;       
            int access_value = 0;
            error_count += get_block_value(access_value, v, "access", false, {FTK_STRING});
            if(access_value > 0) {
                auto acc = to_lower(get_string(access_value));
                bool ro = acc == "ro" || acc == "read-only"  || acc == "readonly";
                bool rw = acc == "rw" || acc == "read-write" || acc == "readwrite";
                if(ro == rw) {
                    ++error_count;
                    pcerr.AddError(main_file, at(access_value), sfmt() << "access for \"" << mount_name << "\" in \"" << name(n) << "\" must be either read-only or read-write");
                    continue;
                }
                read_write = !ro;
            }

            auto &minf = mounts.emplace(v, mount_info(v, n, mount_name, !read_write)).first->second;
            //ni.mounts.push_back(v);

            int s = 0;
            error_count += get_block_value(s, v, "url", false, {FTK_STRING});
            if(s > 0) 
                minf.cos = get_string(s);

            error_count += get_block_value(s, v, "cos", false, {FTK_STRING});
            if(s > 0) 
                if(minf.cos.empty()) minf.cos = get_string(s);

            error_count += get_block_value(s, v, "artifactory", false, {FTK_STRING});
            if(s > 0) 
                if(minf.cos.empty()) minf.cos = get_string(s);
            error_count += get_block_value(s, v, "remote", false, {FTK_STRING});
            if(s > 0) 
                if(minf.cos.empty()) minf.cos = get_string(s);

            error_count += get_block_value(s, v, "secret", false, {FTK_STRING});
            if(s > 0) 
                minf.secret = get_string(s);
            error_count += get_block_value(s, v, "pvc", false, {FTK_STRING});
            if(s > 0) 
                minf.pvc = get_string(s);
            error_count += get_block_value(s, v, "local", false, {FTK_STRING});
            if(s > 0) 
                minf.local = get_string(s);
            
            std::vector<int> paths; 
            error_count += get_block_value(paths, v, "path", true, {FTK_STRING});
            for(auto v: paths) {
                std::string path = get_string(v);
                if(path.empty()) 
                    pcerr.AddWarning(main_file, at(v), "empty \"path\" value");
                group_volumes[group_name].insert(mount_name);
                //ni.mounts.push_back(std::make_tuple(mount_name, path, read_write));
                minf.paths.push_back(path);
            }
            old_value = v;
        }
        */
        old_value = 0;
        //ni.environment.clear();
        if(!external_node) {
            std::map<std::string, int> env;
            error_count += get_nv_block(env, blck, "environment", {FTK_STRING, FTK_FLOAT, FTK_INTEGER});
            /*
            for(auto a: env)
                ni.environment[a.first] = get_value(a.second);
                */
        }
    }

    // Make sure port values are allocated when needed
    /*
    for(int n: get_all_referenced_nodes()) if(method_descriptor(n) != nullptr) {
        int block = get_ne_block_node(n);
        std::string group_name;
        error_count += get_block_s(group_name, block, "group", {FTK_STRING, FTK_INTEGER}, "");
        int pv = 0;
        error_count += get_block_i(pv, block, "port", 0);
        if(pv == 0) {
            pv = base_port+2; 
            while(cot::contains(ports[group_name], pv)) ++pv;
        }
        append(global_vars, "IMAGE_PORT", std::to_string(pv));
        append(group_vars[group_name], "G_IMAGE_PORT", std::to_string(pv));
        ports[group_name].insert(pv);
    }
    */
    // Avoid group name collision
    /*
    for(auto const &gv: group_vars) {
        clear(group_vars[gv.first], "G_HAVE_NODES");
        if(group_vars[gv.first]["G_NODE_NAME"].size() > 0) 
            set(group_vars[gv.first], "G_HAVE_NODES", "");
        clear(group_vars[gv.first], "G_HAVE_VOLUMES");
        if(group_volumes[gv.first].size() > 0)
            set(group_vars[gv.first], "G_HAVE_VOLUMES", "");
    }
    */
    if(mns.size() > 0) 
        set(global_vars, "HAVE_VOLUMES", "");
    else 
        clear(global_vars, "HAVE_VOLUMES");

    bool have_cos = false;
    /*
    for(auto const &m: mounts) {
        have_cos = have_cos || !m.second.cos.empty();
        if(have_cos) break;
    }
    */
    if(have_cos)
        set(global_vars, "HAVE_COS", "");
    else
        clear(global_vars, "HAVE_COS");
    // Make sure the rest volume name doesn't collide with any other volume name
    std::set<std::string> volumes;
    std::string htdocs_volume_name = "htdocs";
    for(unsigned i = 1; i < 100; ++i) {
        if(!cot::contains(volumes, htdocs_volume_name))
            break;
        htdocs_volume_name = sfmt() << "htdocs-files-" << i;
    }
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

    if(error_count == 0 && cot::contains(targets, "server")) 
        error_count += genc_server_source(server_source);
    
    if(error_count == 0 && cot::contains(targets, "client")) 
        error_count += genc_client_source(client_source);

    //std::cerr << "----- before ssl certificates: " << error_count << "\n";
    if(error_count == 0 && cot::contains(targets, "ssl-certificates")) {
        std::string o_default_certificate = opts.opt("default-certificate", "");
        std::string o_grpc_certificate = opts.opt("grpc-certificate", o_default_certificate);
        std::string o_rest_certificate = opts.opt("rest-certificate", o_default_certificate);
        extern char const *template_server_pem;

        if(o_grpc_certificate == o_rest_certificate) {
            if(write_file(output_filename(default_certificate), o_grpc_certificate, template_server_pem)) {
                ++error_count;
                pcerr.AddError(output_filename(default_certificate), -1, 0, "failed to write ssl certificate file");
            }
            append(global_vars, "GRPC_CERTIFICATE", default_certificate);
            append(global_vars, "REST_CERTIFICATE", default_certificate);
        } else {
            if(write_file(output_filename(grpc_certificate), o_grpc_certificate, template_server_pem)) {
                ++error_count;
                pcerr.AddError(output_filename(grpc_certificate), -1, 0, "failed to write grpc ssl certificate file");
            }
            if(write_file(output_filename(rest_certificate), o_rest_certificate, template_server_pem)) {
                ++error_count;
                pcerr.AddError(output_filename(rest_certificate), -1, 0, "failed to write rest ssl certificate file");
            }
            append(global_vars, "GRPC_CERTIFICATE", grpc_certificate);
            append(global_vars, "REST_CERTIFICATE", rest_certificate);
        }
    }
    // Add volume information FIXME
    /* 
    for(auto const &mip: mounts) {
        std::string const &vn = mip.second.name;

        append(global_vars, "VOLUME_NAME", vn);
        append(global_vars, "VOLUME_LOCAL", mip.second.local);
        append(global_vars, "VOLUME_COS", mip.second.cos);
        append(global_vars, "VOLUME_SECRET", mip.second.secret);
        append(global_vars, "VOLUME_PVC", mip.second.pvc);
        append(global_vars, "VOLUME_ISRO", mip.second.read_only? "1": "0");

        auto cp = comments.find(vn);
        if(cp != comments.end()) {
            append(global_vars, "VOLUME_COMMENT", join(cp->second, " "));
        } else {
            append(global_vars, "VOLUME_COMMENT", "");
        }
    }
    */
    if(error_count == 0 && cot::contains(targets, "makefile")) 
        error_count += genc_makefile(orchestrator_makefile);

    if(error_count == 0 && cot::contains(targets, "dockerfile"))
        error_count += genc_dockerfile(orchestrator_name);
    
    //std::cerr << "----- before build image: " << error_count << "\n";
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
    //std::cerr << "----- before build bins: " << error_count << "\n";
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
        error_count += genc_python_client(output_filename(client_bin + ".py"));
           
    //std::cerr << "----- before driver: " << error_count << "\n";
    if(error_count == 0 && cot::contains(targets, "driver")) {
        std::map<std::string, std::vector<std::string>> local_vars;
        std::ostringstream buff;
        error_count += genc_composer(buff, local_vars);
        set(local_vars, "DOCKER_COMPOSE_YAML",  buff.str());
        extern char const *rr_keys_sh;
        set(local_vars, "RR_KEYS_SH", rr_keys_sh);
        extern char const *rr_get_sh;
        buff.str("");
        vex::expand(buff, rr_get_sh, vex::make_smap(local_vars));
        set(local_vars, "RR_GET_SH",  buff.str());
        std::ostringstream yaml;
        error_count += genc_kube(yaml);
        set(local_vars, "KUBERNETES_YAML", yaml.str());

        std::string outputfn = output_filename(orchestrator_name + "-dd.sh");
        if(error_count == 0) {
            std::ofstream outs(outputfn.c_str());
            if(!outs.is_open()) {
                ++error_count;
                pcerr.AddError(outputfn, -1, 0, "failed to write deployment driver");
            } else {
                error_count += genc_deployment_driver(outs, local_vars);
            }
        }
        if(error_count == 0) chmodx(outputfn);
    }
    if(error_count == 0 && cot::contains(targets, "graph-files")) 
        error_count += genc_graph(cot::contains(targets, "svg-files"));
    if(error_count == 0 && cot::contains(targets, "www-files") && !orchestrator_no_rest) 
        error_count += genc_www();
    
    if(error_count != 0) 
        pcerr.AddNote(main_file, -1, 0, sfmt() << error_count << " error(s) during compilation");
   
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_graph(bool gen_svgs) {
    int error_count = 0;
    DEBUG_ENTER;

    for(auto const &entry_name: all(global_vars, "REST_ENTRY")) {
        // Copy the entry name into a writable string because check_entry might overwrite it
        std::string check_entry(entry_name);
        auto mdpe = check_method(check_entry, 0);
        // Find the node corresponding to this entry
        int node = 0;
        for(int n: *this) if(at(n).type == FTK_ENTRY && method_descriptor(n) == mdpe) {
            node = n;
            break;
        }
        // There must always be a node...
        assert(node != 0);
        std::string entry(mdpe->name());
        std::string outputfn = output_filename(std::string("docs/") + entry + ".dot");
        {
            OFSTREAM_SE(outf, outputfn);
            if(error_count == 0)
                print_graph(outf, node);
        }
        if(gen_svgs) {
            std::string svg_filename = output_filename(std::string("docs/") + entry + ".svg");
            std::string dotc(sfmt() << "dot -Tsvg " << outputfn << " -o " << svg_filename);
            if(system(dotc.c_str()) != 0)  {
                pcerr.AddWarning(outputfn, -1, 0, "failed to generate graph svg, is dot available?");
                append(global_vars, "ENTRY_SVG_YAMLSTR", c_escape(""));
            } else {
                std::ifstream svgf(svg_filename.c_str());
                std::stringstream buf;
                buf << svgf.rdbuf();
                append(global_vars, "ENTRY_SVG_YAMLSTR", c_escape(buf.str()));
            }
        }
    }
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
    {"server",            {"grpc-files", "makefile", "svg-files", "dockerfile", "www-files", "ssl-certificates"}},
    {"svg-files",         {"graph-files"}},
    {"grpc-files",        {"protobuf-files"}},
    {"build-client",      {"client"}},
    {"python-client",     {}},
    {"build-server",      {"server"}},
    {"build-image",       {"server", "client", "dockerfile"}},
    {"makefile",          {}},
    {"driver",            {}},
    {"ssl-certificates",  {}},
    {"protobuf-files",    {}},
    {"www-files",         {"docs"}},
    {"graph-files",       {}},
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
        } else {
            ansi::emphasize(std::cout, ansi::emphasize(template_help, ansi::escape(ANSI_BLUE)), ansi::escape(ANSI_BOLD), "-", " \r\n\t =,;/", true, true) << "\n";
            return opts.have("help")? 0: 1;
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
        if(is_dir(path)) {
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
