#include <algorithm>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <set>

#include <grpc++/grpc++.h>

#include "ansi-escapes.H"
#include "filu.H"
#include "flow-comp.H"
#include "flow-templates.H"
#include "helpo.H"
#include "stru.H"

#if defined(STACK_TRACE) && STACK_TRACE
#include <execinfo.h>
#endif

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

#define FLOWC_NAME "flowc"

void show_builtin_help(std::ostream &);

int main(int argc, char *argv[]) {
    signal(SIGABRT, handler);
    signal(SIGSEGV, handler);
    helpo::opts opts;
    int main_argc = argc;
    if(opts.parse(templates::flowc_help(), argc, argv) != 0 || opts.have("version") || opts.have("help") || opts.have("help-syntax") || argc != 2) {
        std::cerr << (opts.optb("color", isatty(fileno(stderr)) && isatty(fileno(stdout)))? ansi::on: ansi::off);
        if(opts.have("help-syntax")) {
            /*
            std::cout << ansi::emphasize2(templates::syntax(), ANSI_BOLD+ANSI_GREEN, ANSI_BOLD+ANSI_MAGENTA) << "\n\n";
            std::ostringstream out;
            show_builtin_help(out);
            std::cout << "Built in functions:\n\n";
            std::cout << ansi::emphasize2(out.str(), ANSI_BOLD+ANSI_GREEN, ANSI_BOLD+ANSI_MAGENTA) << "\n";
            */
            return 0;
        } else if(opts.have("version")) {
            std::cout << FLOWC_NAME << " " << fc::compiler::get_version() << " (" << fc::compiler::get_build_id() << ")\n";
            std::cout << "grpc " << grpc::Version() << "\n";
#if defined(__clang__)          
            std::cout << "clang++ " << __clang_version__ << " (" << __cplusplus << ")\n";
#elif defined(__GNUC__) 
            std::cout << "g++ " << __VERSION__ << " (" << __cplusplus << ")\n";
#else
#endif
            if(fc::compiler::available_runtimes().size() == 1) {
                std::cout << "runtime: " << fc::compiler::get_default_runtime() << "\n";
            } else {
                std::cout << "default runtime: " << fc::compiler::get_default_runtime() << "\n";
                std::cout << "available runtimes: " << stru::join(fc::compiler::available_runtimes(), ", ") << "\n";
            }
            return 0;
        } else if(opts.have("help") || main_argc == 1) {
            std::cout << ansi::emphasize(ansi::emphasize(templates::flowc_help(), ANSI_BLUE), ANSI_BOLD, "-", " \r\n\t =,;/", true, true) << "\n";
            return opts.have("help")? 0: 1;
        } else {
            if(argc != 2) 
                std::cout << "Invalid number of arguments\n";
            std::cout << "Use --help to see the command line usage and all available options\n\n";
            return 1;
        }
    }
    // Make sure we use appropriate coloring for errors
    std::cerr << (opts.optb("color", isatty(fileno(stderr)) && isatty(fileno(stdout)))? ansi::on: ansi::off);
    // Debug flag
    bool debug_on = opts.optb("debug", false);
    fc::compiler comp(&std::cerr);
    bool trace_on = opts.have("trace");

    // Compile the input file(s)
    for(int c = 1; c < argc; ++c) {
        std::cerr << argv[c] << ":\n";
        int rc = comp.compile(argv[c], debug_on, trace_on);
    }
    // Print the AST if asked to
    if(opts.optb("print-ast"))
        comp.print_ast(std::cout, 0);

    return comp.error_count;
}
