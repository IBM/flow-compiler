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
#include "flow-compiler.H"
#include "helpo.H"
#include "stru1.H"
#include "varsub.H"
#include "grpc-helpers.H"

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>


/** 
 * Set this to false to turn off ANSI coloring 
 */
bool use_ansi_escapes = true;

static std::string output_directory;
static 
std::string output_filename(std::string const &filename) {
    if(output_directory.empty())
            return filename;
    if(*output_directory.rbegin() == '/') 
        return output_directory+filename;
    return output_directory+"/"+filename;
}

void FErrorPrinter::AddMessage(std::string const &type, std::string const &color, std::string const &filename, int line, int column, std::string const &message) {
    if(use_ansi_escapes)
        *outs << ANSI_BOLD;
    *outs << filename;
    if(line >= 0) *outs << "(" << line+1;
    if(line >= 0 && column >= 0) *outs << ":" << column+1;
    if(line >= 0) *outs << ")";
    *outs << ": " << color << type << ": ";
    if(use_ansi_escapes)
        *outs << ANSI_RESET;
    stru1::emphasize(*outs, message, ansi_escape(ANSI_BOLD,ANSI_GREEN));
    *outs << "\n";
}
void FErrorPrinter::AddError(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("error", ansi_escape(ANSI_RED), filename, line, column, message);
}
void FErrorPrinter::AddWarning(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("warning", ansi_escape(ANSI_MAGENTA), filename, line, column, message);
}
void FErrorPrinter::AddNote(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("note", ansi_escape(ANSI_BLUE), filename, line, column, message);
}
void ErrorPrinter::AddError(int line, int column, std::string const & message) {
    fperr.AddError(filename, line, column, message);
}
::google::protobuf::io::ZeroCopyOutputStream *GeneratorOD::Open(std::string const &filename) {
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    std::string fn = output_filename(filename);
    int fd = open(fn.c_str(), O_CREAT|O_RDWR|O_TRUNC, mode);
    return new ::google::protobuf::io::FileOutputStream(fd);
}
void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
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
    bool ok = true;
    char buf[16384];
    size_t size;
    int sd = open(source.c_str(), O_RDONLY, 0);
    int dd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    while((size = read(sd, buf, sizeof(buf))) > 0) 
        ok = write(dd, buf, size) == size && ok;
    close(sd); close(dd);
    struct stat a_file_stat;
    struct timeval tv[2];
    stat(source.c_str(), &a_file_stat);
    tv[0].tv_sec = a_file_stat.st_atime; tv[0].tv_usec = 0;
    tv[1].tv_sec = a_file_stat.st_mtime; tv[1].tv_usec = 0;
    utimes(dest.c_str(), &tv[0]);
    return ok? 0: 1;
}

using namespace varsub;

char const *get_version();
char const *get_build_id();
#define FLOWC_NAME "flowc"

flow_compiler::flow_compiler(): pcerr(std::cerr), importer(&source_tree, &pcerr), trace_on(false), verbose(false), input_dp(nullptr) {
    input_label = "input";
    rest_node_label = "rest";
    rest_image_port = 8081;
    gui_image_port = 8082;
    custom_gui_image_port = 8083;

    base_port = 53135;                  // the lowest it can be is 49152
    default_node_timeout = 180000;      // 3 minutes
    default_entry_timeout = 600000;     // 10 minutes
    default_maxcc = 16;                  
    default_repository = "/";

    set(global_vars, "FLOWC_BUILD", get_build_id());
    set(global_vars, "FLOWC_VERSION", get_version());
    set(global_vars, "FLOWC_NAME", FLOWC_NAME);
    set(global_vars, "REST_IMAGE_PORT", std::to_string(rest_image_port));
    set(global_vars, "GUI_IMAGE_PORT", std::to_string(gui_image_port));
    set(global_vars, "CUSTOM_GUI_IMAGE_PORT", std::to_string(custom_gui_image_port));
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
    return 0;
}

int flow_compiler::process(std::string const &input_filename, std::string const &orchestrator_name, std::set<std::string> const &targets, helpo::opts const &opts) {
    int error_count = 0;
    set(global_vars, "INPUT_FILE", input_filename);
    /****************************************************************
     * Add all the import directories to the search path - check if they are valid
     */ 
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
    {
        std::string real_input_filename;
        source_tree.VirtualFileToDiskFile(main_file, &real_input_filename);
        if(realpath(output_filename(main_file)) != realpath(real_input_filename)) 
            cp_p(real_input_filename, output_filename(main_file));
        
        struct stat a_file_stat;
        stat(real_input_filename.c_str(), &a_file_stat);
        char buffer[2048];
        buffer[std::strftime(buffer, sizeof(buffer)-1, "%a, %e %b %Y %T %z", std::localtime(&a_file_stat.st_mtime))] = '\0';
        set(global_vars, "MAIN_FILE_TS", buffer);
    }
   
    set(global_vars, "PROTO_FILES_PATH", path_join(std::getenv("PWD"), output_filename(".")));
    set(global_vars, "NAME", orchestrator_name);
    set(global_vars, "NAME_ID", to_lower(to_identifier(orchestrator_name)));
    set(global_vars, "NAME_OPT", to_option(orchestrator_name));
    set(global_vars, "NAME_UPPER", to_upper(to_option(orchestrator_name)));
    set(global_vars, "MAIN_DESCRIPTION", c_escape(""));
    set(global_vars, "NAME_UPPERID", to_upper(to_identifier(orchestrator_name)));
    /****************************************************************
     * file names
     */
    std::string server_bin = orchestrator_name + "-server";
    std::string server_source = output_filename(server_bin + ".C");
    std::string client_bin = orchestrator_name + "-client";
    std::string client_source = output_filename(client_bin + ".C");
    std::string orchestrator_makefile = orchestrator_name + ".mak";
    std::string orchestrator_logproto = orchestrator_name + "-flow-logger.proto";
    std::string orchestrator_dockerfile = orchestrator_name + ".Dockerfile";

    error_count += parse();
    //if(opts.have("print-ast")) 
    //    print_ast(std::cout);
    if(error_count == 0)
        error_count += compile(targets);
    if(opts.have("print-graph")) {
        std::string entry(opts.opt("print-graph", ""));
        int en = 0;
        for(auto ei: names) if(ei.second.first == "entry") {
            if(ei.first == entry || ends_with(ei.first, std::string(".") + entry)) {
                en = ei.second.second;
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
    if(opts.have("print-ast")) 
        print_ast(std::cout);
    if(opts.have("print-pseudocode"))
        dump_code(std::cout);

    if(error_count == 0 && package_name.empty())
        package_name = orchestrator_name;

    package_name = to_identifier(package_name);

    if(token_comment.size() > 0 && token_comment[0].first == 1) 
        set(global_vars, "MAIN_DESCRIPTION", c_escape(token_comment[0].second));

    /*******************************************************************
     * Override some of the vars set at compile time
     */
    base_port = opts.opti("base-port", base_port);
    rest_image_port = opts.opti("rest-port", rest_image_port);
    gui_image_port = opts.opti("rest-port", gui_image_port);
    custom_gui_image_port = opts.opti("rest-port", custom_gui_image_port);

    set(global_vars, "MAIN_PORT", std::to_string(base_port));
    set(global_vars, "REST_NODE_PORT", std::to_string(base_port-1));
    set(global_vars, "GUI_NODE_PORT", std::to_string(base_port-2));
    set(global_vars, "CUSTOM_GUI_NODE_PORT", std::to_string(base_port-3));

    default_node_timeout = opts.opti("default-client-timeout", default_node_timeout);
    //default_entry_timeout = opts.opti("default-server-timeout", default_entry_timeout);
    default_maxcc = opts.opti("default-client-calls", default_maxcc);

    rest_image = opts.opt("rest-image", rest_image);
    if(!rest_image.empty() && rest_image[0] == '/') rest_image = path_join(default_repository, rest_image.substr(1));
    
   
    orchestrator_tag = opts.opt("image-tag", "1");
    orchestrator_image = opts.opt("image", to_lower(orchestrator_name)+":"+orchestrator_tag);

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
     * All files are compiled at this pont so,
     * set all as many of values that control code generation as possible
     */
    bool generate_kubernetes = contains(targets, "kubernetes");
    bool generate_docker_compose = contains(targets, "docker-compose");
    bool generate_config_files = generate_kubernetes || generate_docker_compose;
    bool build_bin = opts.have("build-client") || opts.have("build-server");
    bool build_any = opts.have("build-image") || build_bin;

    set(global_vars, "REST_NODE_IMAGE", rest_image);
    // Whether REST is added or not depends on the value of the rest_image field -- for now
    bool add_rest_node = generate_config_files && !rest_image.empty();
    if(generate_config_files) {
        for(auto const &nc: names) if(nc.second.first == "container") 
            referenced_nodes.emplace(nc.second.second, node_info(nc.second.second, nc.first));
    }
    /********************************************************************
     * Generate C files for proto and grpc
     */
    if(error_count == 0 && (contains(targets, "protobuf-files") || build_bin)) {
        std::string fn = output_filename(orchestrator_logproto);
        {   // Write out the logger interface
            std::ofstream logpf(fn.c_str());
            if(!logpf.is_open()) {
                ++error_count;
                pcerr.AddError(orchestrator_logproto, -1, 0, "failed to write logger proto file");
            } else {
                extern char const *template_flow_logger_proto;
                render_varsub(logpf, template_flow_logger_proto, global_vars);
            }
        }
        // Add the destination directory to the proto path
        // since imports are compiled by now 
        add_to_proto_path(output_filename(".")); 
        // Compile the logger protobuf
        error_count += compile_proto(orchestrator_logproto);
        error_count += genc_protobuf(); 
    }

    if(error_count == 0 && (contains(targets, "grpc-files") || build_bin))
        error_count += genc_grpc(); 

    // Prepare include statements with all the grpc and pb generated headers
    clear(global_vars, "PB_GENERATED_C");
    clear(global_vars, "PB_GENERATED_H");
    clear(global_vars, "GRPC_GENERATED_C");
    clear(global_vars, "GRPC_GENERATED_H");
    clear(global_vars, "PROTO_FILE");

    for(auto fdp: fdps) {
        std::string file(fdp->name());
        std::string basefn = remove_suffix(file, ".proto");
        append(global_vars, "PROTO_FILE", file);
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
        if(realpath(output_filename(file)) != realpath(filename)) 
            cp_p(filename, output_filename(file));
    }
    // Set a value to trigger node generation

    clear(global_vars, "HAVE_NODES");
    if(referenced_nodes.size() > 0) 
        set(global_vars, "HAVE_NODES", ""); 
    // Make sure the rest node name doesn't collide with any other node
    for(unsigned i = 1; i < 100; ++i) {
        // FIXME: need to ckeck against referenced_nodes
        if(!contains(names, rest_node_label))
            break;
        rest_node_label = sfmt() << "rest_" << i;
    }
    // Use the this var to conditionally generate rest node in templates
    if(add_rest_node) {
        set(global_vars, "REST_NODE_NAME", rest_node_label);
        // FIXME: this needs to be resurected
        //image_port[rest_node_label] = rest_image_port;
    } else  {
        clear(global_vars, "REST_NODE_NAME");
    }

    // Grab all the image names, image ports and volume names 
    if(generate_config_files) {
        // Make a first pass to collect the declared ports and groups
        for(auto &rn: referenced_nodes) {
            int blck = rn.first;
            auto &ni = rn.second;
            std::string const &nn = rn.second.name;

            int value = 0, pv = 0;
            error_count += get_block_value(value, blck, "port", false, {FTK_INTEGER});
            if(value > 0) {
                pv = (int) get_integer(value);
                if(pv <= 0) {
                    pcerr.AddWarning(main_file, at(value), sfmt() << "invalid \"port\" value: \"" << pv << "\" for node \"" << nn << "\", a default value will be assigned");
                    pv = 0;
                }
            }
            ni.port = pv;

            error_count += get_block_value(value, blck, "pod", false, {FTK_STRING, FTK_INTEGER});
            std::string pod_name;
            if(!opts.have("single-pod") && value > 0) pod_name = get_value(value);
            ni.group = pod_name;
        }
        // Allocate port values for the nodes that have not declared one
        while(true) {
            int nn = 0; 
            for(auto np: referenced_nodes) if(np.second.port == 0) {
                nn = np.first;
                break;
            }
            if(nn == 0) break;
            bool free = false;
            for(int p = base_port + 1; !free && p < 65534; ++p) {
                free = true;
                for(auto np: referenced_nodes) if(np.second.port == p) {
                    free = false;
                    break;
                }
                if(free) referenced_nodes[nn].port = p;
            }
            if(!free) {
                ++error_count;
                pcerr.AddError(main_file, -1, 0, sfmt() << "unable to allocate port for node \"" << name(nn) << "\"");
            }
        }
        
        for(auto &rn: referenced_nodes) {
            int blck = rn.first;
            auto &ni = rn.second;

            std::string const &nn = ni.name;
            append(global_vars, "NODE_NAME", nn);
            int value = 0, pv = 0;

            std::string pod_name = ni.group;
            append(global_vars, "NODE_GROUP", pod_name);
            append(group_vars[pod_name], "G_NODE_GROUP", pod_name);
            append(group_vars[pod_name], "G_NODE_GROUP_UPPER", to_upper(to_underscore(pod_name)));
            append(group_vars[pod_name], "G_NODE_NAME", nn);
            append(group_vars[pod_name], "G_NODE_OPTION", to_option(nn));
            append(group_vars[pod_name], "G_NODE_SERVICE", sfmt() << to_option(orchestrator_name) << "-" << pod_name);

            pv = ni.port;
            append(global_vars, "IMAGE_PORT", std::to_string(pv));
            append(group_vars[pod_name], "G_IMAGE_PORT", std::to_string(pv));

            // For kubernetes check that ports in the same pod don't clash
            if(generate_kubernetes) for(auto const &np: referenced_nodes) if(np.second.port == pv && np.first != blck && np.second.group == pod_name) {
                ++error_count;
                pcerr.AddError(main_file, at(blck), sfmt() << "port value \"" << pv << "\" for \"" << nn << "\" already used by \"" << np.first << "\"");
                // Only generate one port conflict message
                break;
            }
            value = 0;
            bool external_node = false;
            error_count += get_block_value(value, blck, "image", false, {FTK_STRING});
            if(value <= 0 || get_string(value).empty()) {
                error_count += get_block_value(value, blck, "endpoint", false, {FTK_STRING});
                if(value <= 0 || get_string(value).empty()) {
                    ++error_count;
                    pcerr.AddError(main_file, at(blck), sfmt() << "node \"" << nn << "\" must have either an image or an endpoint defined");
                } else {
                    ni.external_endpoint = get_string(value);
                    external_node = true;
                }
            } else {
                ni.image_name = get_string(value);
                if(ni.image_name[0] == '/') 
                    ni.image_name = path_join(default_repository, ni.image_name.substr(1));
            }
            append(global_vars, "NODE_IMAGE", ni.image_name);
            append(group_vars[pod_name], "G_NODE_IMAGE", ni.image_name);
            append(global_vars, "NODE_ENDPOINT", ni.external_endpoint);
            append(group_vars[pod_name], "G_NODE_ENDPOINT", ni.external_endpoint);
            if(external_node) {
                append(global_vars, "EXTERN_NODE", "#");
                append(group_vars[pod_name], "G_EXTERN_NODE", "#");
            } else {
                append(global_vars, "EXTERN_NODE", "");
                append(group_vars[pod_name], "G_EXTERN_NODE", "");
            }

            // Volume mounts
            int old_value = 0;
            ni.mounts.clear();
            if(!external_node) for(int p = 0, v = find_in_blck(blck, "mount", &p); v != 0; v = find_in_blck(blck, "mount", &p)) {
                if(at(v).type != FTK_lblk) {
                    error_count += 1;
                    pcerr.AddError(main_file, at(v), "mount must a labeled name/value pair block");
                    continue;
                }
                std::string mount_name = get_id(at(v).children[0]);
                v = at(v).children[1];

                auto ebp = block_store.find(v);
                assert(ebp != block_store.end());
                bool read_write = false;       
                int access_value = 0;
                error_count += get_block_value(access_value, v, "access", false, {FTK_STRING});
                if(access_value > 0) {
                    auto acc = to_lower(get_string(access_value));
                    bool ro = acc == "ro" || acc == "read-only"  || acc == "readonly";
                    bool rw = acc == "rw" || acc == "read-write" || acc == "readwrite";
                    if(ro == rw) {
                        ++error_count;
                        pcerr.AddError(main_file, at(access_value), sfmt() << "access for \"" << mount_name << "\" in \"" << nn << "\" must be either read-only or read-write");
                        continue;
                    }
                    read_write = !ro;
                }

                auto &minf = mounts.emplace(v, mount_info(v, rn.first, mount_name, !read_write)).first->second;
                ni.mounts.push_back(v);

                int s = 0;
                error_count += get_block_value(s, v, "cos", false, {FTK_STRING});
                if(s > 0) 
                    minf.cos = get_string(s);
                error_count += get_block_value(s, v, "artifactory", false, {FTK_STRING});
                if(s > 0) 
                    minf.artifactory = get_string(s);
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
                    group_volumes[pod_name].insert(mount_name);
                    //ni.mounts.push_back(std::make_tuple(mount_name, path, read_write));
                    minf.paths.push_back(path);
                }
                old_value = v;
            }
            old_value = 0;
            ni.environment.clear();
            if(!external_node) for(int p = 0, v = find_in_blck(blck, "environment", &p); v != 0; v = find_in_blck(blck, "environment", &p)) {
                if(at(v).type != FTK_blck) {
                    error_count += 1;
                    pcerr.AddError(main_file, at(v), "environment must a name/value pair block");
                    continue;
                }
                // Grab all environment values
                auto ebp = block_store.find(v);
                assert(ebp != block_store.end());
                for(auto const &nv: ebp->second) {
                    if(at(nv.second).type != FTK_STRING && at(nv.second).type != FTK_FLOAT && at(nv.second).type != FTK_INTEGER) {
                        error_count += 1;
                        pcerr.AddError(main_file, at(nv.second), "expected value for environment variable");
                        continue;
                    }
                    ni.environment[nv.first] = get_value(nv.second); 
                }
                old_value = v;
            }
        }

        // Avoid pod name collision
        set(global_vars, "MAIN_POD", "main");
        for(int i = 1; i < 100 && contains(group_vars, get(global_vars, "MAIN_POD")); ++i) {
            set(global_vars, "MAIN_POD", sfmt() << "main" << i);
        }
        
        for(auto const &gv: group_vars) {
            clear(group_vars[gv.first], "G_HAVE_NODES");
            // TODO: check here if all nodes in the group are external
            if(group_vars[gv.first]["G_NODE_NAME"].size() > 0) 
                set(group_vars[gv.first], "G_HAVE_NODES", "");
            clear(group_vars[gv.first], "G_HAVE_VOLUMES");
            if(group_volumes[gv.first].size() > 0)
                set(group_vars[gv.first], "G_HAVE_VOLUMES", "");
        }
        // TODO: check here if all nodes are external
        set(global_vars, "HAVE_NODES", ""); 

        if(mounts.size() > 0) 
            set(global_vars, "HAVE_VOLUMES", "");
        else 
            clear(global_vars, "HAVE_VOLUMES");
    }
    if(add_rest_node || mounts.size() > 0) 
        set(global_vars, "HAVE_VOLUMES_OR_REST", "");
    else 
        clear(global_vars, "HAVE_VOLUMES_OR_REST");

    bool have_artifactory = false;
    bool have_cos = false;
    for(auto const &m: mounts) {
        have_artifactory = have_artifactory || !m.second.artifactory.empty();
        have_cos = have_cos || !m.second.cos.empty();
        if(have_artifactory && have_cos) break;
    }
    if(have_artifactory)
        set(global_vars, "HAVE_ARTIFACTORY", "");
    else
        clear(global_vars, "HAVE_ARTIFACTORY");
    if(have_cos)
        set(global_vars, "HAVE_COS", "");
    else
        clear(global_vars, "HAVE_COS");

    if(have_cos || have_artifactory)
        set(global_vars, "HAVE_REMOTE_RESOURCE", "");
    else
        clear(global_vars, "HAVE_REMOTE_RESOURCE");

    bool generate_artifactory_get = generate_docker_compose && have_artifactory;
    bool generate_cos_get = generate_docker_compose && have_cos;

    // Make sure the rest volume name doesn't collide with any other volume name
    std::string rest_volume_name = "proto-files";
    std::set<std::string> volumes;
    for(auto np: mounts) volumes.insert(np.second.name);
    for(unsigned i = 1; i < 100; ++i) {
        if(!contains(volumes, rest_volume_name))
            break;
        rest_volume_name = sfmt() << "proto-files-" << i;
    }
    set(global_vars, "REST_VOLUME_NAME", rest_volume_name);
    std::string htdocs_volume_name = "htdocs";
    for(unsigned i = 1; i < 100; ++i) {
        if(!contains(volumes, htdocs_volume_name))
            break;
        htdocs_volume_name = sfmt() << "htdocs-files-" << i;
    }
    set(global_vars, "HTDOCS_VOLUME_NAME", htdocs_volume_name);

    // Check/generate the entry list for the rest gateway
    // We only do this if we have rest and generated config files
    if(add_rest_node) {
        // use a set to avoid duplicates
        std::set<std::string> rest_entries(all(global_vars, "REST_ENTRY").begin(), all(global_vars, "REST_ENTRY").end()); 
        for(auto ep: names) if(ep.second.first == "entry") {
            std::string method = ep.first;
            auto mdp = check_method(method, 0);
            int blck = ep.second.second, pv = 0;
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
        for(auto const &re: rest_entries) 
            append(global_vars, "ENTRY_UPPERID", to_upper(to_underscore(re)));
    }
    //std::cerr << "********* global: \n" << global_vars << "\n";

    //std::cerr << "----- before server: " << error_count << "\n";
    if(error_count == 0 && (contains(targets, "server") || opts.have("build-server"))) {
        std::ofstream outf(server_source.c_str());
        if(!outf.is_open()) {
            ++error_count;
            pcerr.AddError(server_source, -1, 0, "failed to write server source file");
        } else {
            error_count += gc_server(outf);
        }
    }
    //std::cerr << "----- before client: " << error_count << "\n";
    if(error_count == 0 && (contains(targets, "client") || opts.have("build-client"))) {
        std::ofstream outf(client_source.c_str());
        if(!outf.is_open()) {
            ++error_count;
            pcerr.AddError(client_source, -1, 0, "failed to write client source file");
        } else {
            error_count += genc_client(outf);
        }
    }
    //std::cerr << "----- before makefile: " << error_count << "\n";
    if(error_count == 0 && (contains(targets, "makefile") || build_any)) {
        std::string fn = output_filename(orchestrator_makefile);
        std::ofstream makf(fn.c_str());
        if(!makf.is_open()) {
            ++error_count;
            pcerr.AddError(orchestrator_makefile, -1, 0, "failed to write make file");
        } else {
            extern char const *template_Makefile;
            render_varsub(makf, template_Makefile, global_vars);
        }
    }
    //std::cerr << "----- before dockerfile: " << error_count << "\n";
    if(error_count == 0 && (contains(targets, "dockerfile") || opts.have("build-image"))) {
        std::string fn = output_filename(orchestrator_dockerfile);
        std::ofstream outf(fn.c_str());
        if(!outf.is_open()) {
            ++error_count;
            pcerr.AddError(orchestrator_dockerfile, -1, 0, "failed to write docker file");
        } else {
            extern char const *template_runtime_Dockerfile;
            render_varsub(outf, template_runtime_Dockerfile, global_vars);
            extern char const *template_Dockerfile;
            render_varsub(outf, template_Dockerfile, global_vars);
        }
    }
    //std::cerr << "----- before build image: " << error_count << "\n";
    if(error_count == 0 && opts.have("build-image")) {
        std::string makec = sfmt() << "cd " << output_filename(".") << " && make -f " << orchestrator_makefile 
            << " DOCKERFILE=" << orchestrator_dockerfile << " image";
        if(system(makec.c_str()) != 0) {
            pcerr.AddError(main_file, -1, 0, "failed to build docker image");
            ++error_count;
        }
    }
    //std::cerr << "----- before build bins: " << error_count << "\n";
    if(error_count == 0 && build_bin) {
        std::string makec = sfmt() << "cd " << output_filename(".")  << " && make -f " << orchestrator_makefile << " ";
        if(opts.have("build-server") && opts.have("build-client")) 
            makec += "-j2 all";
        else if(opts.have("build-server"))
            makec += "server";
        else 
            makec += "client";
        if(system(makec.c_str()) != 0) {
            pcerr.AddError(main_file, -1, 0, "failed to build");
            ++error_count;
        }
    }
    //std::cerr << "----- before compose: " << error_count << "\n";
    if(error_count == 0 && generate_docker_compose) {
        std::ostringstream yaml;
        error_count += genc_composer(yaml);

        std::string outputfn = output_filename(orchestrator_name + "-dc.sh");
        if(error_count == 0) {
            std::ofstream outs(outputfn.c_str());
            if(!outs.is_open()) {
                ++error_count;
                pcerr.AddError(outputfn, -1, 0, "failed to write Docker Composer driver");
            } else {
                error_count += genc_composer_driver(outs, yaml.str());
            }
        }
        if(error_count == 0) chmodx(outputfn);
    }
    if(error_count == 0 && generate_artifactory_get) {
        std::string outputfn = output_filename(orchestrator_name + "-ag.sh");
        if(error_count == 0) {
            std::ofstream outs(outputfn.c_str());
            if(!outs.is_open()) {
                ++error_count;
                pcerr.AddError(outputfn, -1, 0, "failed to write Artifactory puller");
            } else {
                extern char const *artiget_sh;
                render_varsub(outs, artiget_sh, global_vars);
            }
        }
        if(error_count == 0) chmodx(outputfn);
    }
    if(error_count == 0 && generate_cos_get) {
        std::string outputfn = output_filename(orchestrator_name + "-cg.sh");
        if(error_count == 0) {
            std::ofstream outs(outputfn.c_str());
            if(!outs.is_open()) {
                ++error_count;
                pcerr.AddError(outputfn, -1, 0, "failed to write COS puller");
            } else {
                extern char const *cosget_sh;
                render_varsub(outs, cosget_sh, global_vars);
            }
        }
        if(error_count == 0) chmodx(outputfn);
    }
    if(error_count == 0 && (generate_kubernetes || generate_docker_compose)) {
        for(auto const &entry: all(global_vars, "REST_ENTRY")) {
            std::string ente(entry);
            auto mdpe = check_method(ente, 0);
            int n = 0;
            for(auto ep: names) if(ep.second.first == "entry") {
                std::string entn(ep.first);
                if(mdpe == check_method(entn, 0)) {
                    n = ep.second.second; 
                    break;
                }
            }
            assert(n != 0);
            std::string outputfn = output_filename(entry + ".dot");
            {
                std::ofstream outf(outputfn.c_str());
                if(!outf.is_open()) {
                    ++error_count;
                    pcerr.AddError(outputfn, -1, 0, "failed to write DOT file");
                } else {
                    print_graph(outf, n);
                }
            }
            std::string svg_filename = output_filename(entry + ".svg");
            std::string dotc(sfmt() << "dot -Tsvg " << outputfn << " -o " << svg_filename);
            if(system(dotc.c_str()) != 0)  {
                pcerr.AddWarning(outputfn, -1, 0, "failed to gerenate graph svg, is dot available?");
                append(global_vars, "ENTRY_SVG_YAMLSTR", c_escape(""));
            } else {
                std::ifstream svgf(svg_filename.c_str());
                std::stringstream buf;
                buf << svgf.rdbuf();
                append(global_vars, "ENTRY_SVG_YAMLSTR", c_escape(buf.str()));
            }

        }
    }
    //std::cerr << "----- before kubernetes: " << error_count << "\n";
    if(error_count == 0 && generate_kubernetes) {
        std::ostringstream yaml;
        error_count += genc_kube(yaml);

        //std::cerr << "----- before kubernetes driver: " << error_count << "\n";
        std::string outputfn = output_filename(orchestrator_name + "-kube.sh");
        if(error_count == 0) {
            std::ofstream outf(outputfn.c_str());
            if(!outf.is_open()) {
                ++error_count;
                pcerr.AddError(outputfn, -1, 0, "failed to write Kubernetes driver");
            } else {
                error_count += genc_kube_driver(outf, yaml.str());
            }
        }
        if(error_count == 0) chmodx(outputfn);
    }
    //std::cerr << "----- before return: " << error_count << "\n";
    if(error_count == 0) {
        //pcerr.AddNote(main_file, -1, 0, sfmt() << "build successful");
    } else {
        pcerr.AddNote(main_file, -1, 0, sfmt() << error_count << " error(s) during compilation");
    }
    return error_count;
}

extern char const *template_help, *template_syntax;
static std::map<std::string, std::vector<std::string>> all_targets = {
    {"dockerfile",        {"makefile"}},
    {"client",            {"protobuf-files", "grpc-files", "makefile", "dockerfile" }},
    {"server",            {"protobuf-files", "grpc-files", "makefile", "dockerfile" }},
    {"docker-compose",    {}},
    {"kubernetes",        {}},
    {"protobuf-files",    {}},
    {"grpc-files",        {"protobuf-files"}},
    {"makefile",          {}},
};


int main(int argc, char *argv[]) {
    signal(SIGSEGV, handler);
    helpo::opts opts;
    if(opts.parse(template_help, argc, argv) != 0 || opts.have("help") || argc != 2) {
        use_ansi_escapes = opts.optb("color", use_ansi_escapes && isatty(fileno(stdout)) && isatty(fileno(stderr)));
        if(opts.have("help-syntax")) {
            std::cout << emphasize(template_syntax, ansi_escape(ANSI_BOLD, ANSI_GREEN), ansi_escape(ANSI_BOLD, ANSI_MAGENTA)) << "\n";
            return 0;
        } else if(opts.have("version")) {
            std::cout << FLOWC_NAME << " " << get_version() << " (" << get_build_id() << ")\n";
            std::cout << "grpc " << grpc::Version() << "\n";
            return 0;
        } else {
            emphasize(std::cout, emphasize(template_help, ansi_escape(ANSI_BLUE)), ansi_escape(ANSI_BOLD), "-", " \r\n\t =,;/", true, true) << "\n";
            return opts.have("help")? 0: 1;
        }
    }
    // Make sure we use appropriate coloring for errors
    use_ansi_escapes = opts.optb("color", use_ansi_escapes && isatty(fileno(stderr)) && isatty(fileno(stdout)));

    std::set<std::string> targets;
    for(auto kt: all_targets) 
        if(opts.have(kt.first)) {
            targets.insert(kt.first);
            targets.insert(kt.second.begin(), kt.second.end());
        }

    flow_compiler gfc;
    gfc.trace_on = opts.have("trace");

    std::string orchestrator_name(opts.opt("name", basename(argv[1], ".flow")));
    output_directory = opts.opt("output-directory", "");

    if(orchestrator_name == argv[1]) {
        std::cerr << "The orchestrator name is the same as the input file, use --name to change\n";
        return 1;
    }
    //std::cerr << "The orchestrator name is " << orchestrator_name <<"\n";
    int rc = gfc.process(argv[1], orchestrator_name, targets, opts);
    return rc;
}