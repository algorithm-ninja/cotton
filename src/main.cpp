#include "box.hpp"
#include <vector>
#include <boost/program_options.hpp>
#ifdef __unix__
#include <setjmp.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#endif
namespace po = boost::program_options;

BoxCreators box_creators;
std::vector<std::pair<int, std::string>> errors;
std::vector<std::pair<int, std::string>> warnings;
std::string result = "null";
bool json_output = false;

#ifdef __unix__
static const char* const error_color = "\033[31;1m";
static const char* const warning_color = "\033[31;3m";
static const char* const reset_color = "\033[;m";
#else
static const char* const error_color = "";
static const char* const warning_color = "";
static const char* const reset_color = "";
#endif


#ifdef __unix__
jmp_buf exit_buf;
void sig_handler(int sig) {
    errors.emplace_back(255, "Received signal " + std::to_string(sig));
    longjmp(exit_buf, 1);
}
#endif

int main(int argc, char** argv) {
    po::options_description global("Generic options");
    global.add_options()
        ("help,h", "produce help message")
        ("box-root,b", po::value<std::string>(), "specify a different folder to put sandboxes in")
        ("quiet,q", "less output for tty mode")
        ("json,j", "force JSON output");

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("command", po::value<std::string>(), "command to execute")
        ("command-args", po::value<std::vector<std::string> >(), "arguments for command");

    po::positional_options_description pos;
    pos.add("command", 1).
        add("command-args", -1);

    po::variables_map vm;

    po::options_description generic_opts("Generic options");
    generic_opts.add(global).add(hidden);
    po::parsed_options parsed = po::command_line_parser(argc, argv).
        options(generic_opts).
        positional(pos).
        allow_unregistered().
        run();

    po::store(parsed, vm);
    if (vm.count("help") == 1) {
        std::cerr << "Usage: " << argv[0] << " [options] command [command-args...]" << std::endl;
        std::cerr << std::endl;
        std::cerr << global << std::endl;
        std::cerr << "Possible commands are:" << std::endl;
        std::cerr << " list        list available implementations" << std::endl;
        std::cerr << " check       check if a sandbox is consistent" << std::endl;
        std::cerr << " set-limit   set some limit for the program execution" << std::endl;
        std::cerr << " get-limit   read a current limit" << std::endl;
        std::cerr << " mount       make paths readable to the process" << std::endl;
        std::cerr << " umount      disable paths" << std::endl;
        std::cerr << " run         execute command" << std::endl;
        std::cerr << " get         get information on the last execution" << std::endl;
        std::cerr << " clear       cleanup the sandbox" << std::endl;
        std::cerr << " delete      delete the sandbox" << std::endl;
        return 1;
    }

#ifdef __unix__
    std::string box_root = "/tmp";
#else
    std::string box_root = "who knows";
#endif
    if (vm.count("box-root") == 1) box_root = vm["box-root"].as<std::string>();

    if (vm.count("json")) json_output = true;
#ifdef __unix__
    if (!isatty(fileno(stdout))) json_output = true;
#endif


#ifdef __unix__
    signal(SIGHUP, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGABRT, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGPIPE, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGBUS, sig_handler);
#endif
    if (!vm.count("command")) {
        errors.emplace_back(1, "No command given!");
#ifdef __unix__
    } else if (!setjmp(exit_buf)) {
#else
    } else {
#endif
        try {
            std::string cmd = vm["command"].as<std::string>();
            if (cmd == "list") {
            
            } else if (cmd == "create") {
            
            } else if (cmd == "check") {
            
            } else if (cmd == "get-root") {
            
            } else if (cmd == "set-limit") {
            
            } else if (cmd == "get-limit") {
            
            } else if (cmd == "get-redirect") {
            
            } else if (cmd == "set-redirect") {
            
            } else if (cmd == "mount") {
            
            } else if (cmd == "umount") {
            
            } else if (cmd == "run") {
            
            } else if (cmd == "get") {
            
            } else if (cmd == "clear") {
            
            } else if (cmd == "delete") {
            
            } else
                errors.emplace_back(2, "Unknown command!");
        } catch (std::exception& e) {
            errors.emplace_back(255, std::string("Unhandled exception! ") + e.what());
        }
    }
    if (json_output) {
        std::cout << "{\"result\": " << result;
        std::cout << ", \"errors\": [";
        for (unsigned i=0; i<errors.size(); i++) {
            std::cout << "{\"code\": " << errors[i].first;
            std::cout << ", \"message\": \"" << errors[i].second << "\"}";
            if (i+1 != errors.size()) std::cout << ", ";
        }
        std::cout << "], \"warnings\": [";
        for (unsigned i=0; i<warnings.size(); i++) {
            std::cout << "{\"code\": " << warnings[i].first;
            std::cout << ", \"message\": \"" << warnings[i].second << "\"}";
            if (i+1 != warnings.size()) std::cout << ", ";
        }
        std::cout << "]}" << std::endl;
    } else {
        if (result != "null") std::cout << result << std::endl;
        for (auto x: errors) {
            std::cerr << error_color << "Error " << x.first;
            std::cerr << reset_color << ": " << x.second << std::endl;
        }
        for (auto x: warnings) {
            std::cerr << warning_color << "Warning " << x.first;
            std::cerr << reset_color << ": " << x.second << std::endl;
        }
    }
}
