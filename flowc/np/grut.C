#include <iostream>

#include <grpcpp/grpcpp.h>
#if defined(STACK_TRACE) && STACK_TRACE
#include <execinfo.h>
#endif

#include "ansi-escapes.H"
#include "filu.H"
#include "gru.H"
#include "helpo.H"
#include "stru1.H"
#include "flowcomp.H"

extern std::string get_grut_help();

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

int main(int argc, char **argv) {
    signal(SIGABRT, handler);
    signal(SIGSEGV, handler);
    helpo::opts opts;
    int main_argc = argc;
    if(opts.parse(get_grut_help(), argc, argv) != 0 || opts.have("version") || opts.have("help") || argc < 2) {
        std::cerr << (opts.optb("color", isatty(fileno(stderr)) && isatty(fileno(stdout)))? ansi::on: ansi::off);
        if(opts.have("version")) {
            std::cout << "grpc " << grpc::Version() << "\n";
#if defined(__clang__)          
            std::cout << "clang++ " << __clang_version__ << " (" << __cplusplus << ")\n";
#elif defined(__GNUC__) 
            std::cout << "g++ " << __VERSION__ << " (" << __cplusplus << ")\n";
#else   
#endif
            return 0;
        } else if(opts.have("help") || main_argc == 1) {
            std::cout << ansi::emphasize(ansi::emphasize(get_grut_help(), ANSI_BOLD, "-", " \r\n\t =,;/", true, true), ANSI_BLUE) << "\n";
            return opts.have("help")? 0: 1;
        } else {
            if(argc < 2) 
                std::cout << "Invalid number of arguments\n";
            std::cout << "Use --help to see the command line usage and all available options\n\n";
            return 1;
        }
    }
    // Make sure we use appropriate coloring for errors
    std::cerr << (opts.optb("color", isatty(fileno(stderr)) && isatty(fileno(stdout)))? ansi::on: ansi::off);
    fc::flow_compiler comp;
    // Add all the paths to proto files
    for(auto path: opts["proto-path"]) {
        std::cerr << "-I " << path << "\n";
        comp.add_to_proto_path(path);
    }
    // Compile the input files
    for(int i = 1; i < argc; ++i) {
        std::cerr << argv[i] << ":\n";
        comp.compile_proto(argv[i], true);
    }
    // Look for the entries
    for(auto entry: opts["entry"]) {
        std::cerr << "-e " << entry << "\n";
        auto md = comp.find_method(entry);
        if(md == nullptr) {
            std::cerr << "NOT FOUND!\n";
        } else {
            std::cout << md->DebugString() << "\n";
        }
    }
    std::vector<std::string> deps;
    comp.get_dependencies(deps, opts["entry"]);

    for(auto s: deps) {
        std::cerr << s << "\n";
    }
    std::vector<std::string> srcs;
    comp.get_proto_text(srcs, deps);
    for(int i = 0, e = srcs.size(); i < e; ++i) {
        std::string ofn = filu::path_join(opts.opt("output-directory", "."), stru1::sfmt() << "p" << (i+1) << ".proto");
        filu::write_file(ofn, srcs[i]);
    }

    return 0;
}
