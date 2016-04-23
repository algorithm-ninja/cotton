#include "box.hpp"
#include "logger.hpp"
#include <vector>
#include <boost/program_options.hpp>
#ifdef __unix__
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#endif
namespace po = boost::program_options;

BoxCreators box_creators;
CottonLogger* logger;
std::string program_name;

#ifdef __unix__
void sig_handler(int sig) {
    logger->error(255, "Received signal " + std::to_string(sig));
    logger->write();
    exit(0);
}
#endif


void parse_subcommand_options(
    po::options_description& global_opts,
    const std::vector<std::string>& opts,
    po::variables_map& vm,
    const std::string& command_name,
    const std::vector<std::pair<const char*, const char*>>& flags,
    const std::vector<const char*>& required_args,
    const std::vector<const char*>& optional_args,
    const char* trailing_args = NULL) {
    po::options_description subcommand_descr(command_name + " options");
    subcommand_descr.add_options()("help,h", "produce help message");
    for (const auto& flag: flags)
        subcommand_descr.add_options()(flag.first, flag.second);

    po::options_description hidden_args("hidden arguments");
    po::positional_options_description pos;
    for (const auto& arg: required_args) {
        hidden_args.add_options()(arg, po::value<std::string>()->required(), "");
        pos.add(arg, 1);
    }
    for (const auto& arg: optional_args) {
        hidden_args.add_options()(arg, po::value<std::string>(), "");
        pos.add(arg, 1);
    }
    if (trailing_args != NULL) {
        hidden_args.add_options()(trailing_args, "");
        pos.add(trailing_args, -1);
    }

    po::options_description command_opts;
    command_opts.add(subcommand_descr).add(hidden_args);

    try {
        po::parsed_options parsed = po::command_line_parser(opts)
            .options(command_opts)
            .positional(pos)
            .run();

        po::store(parsed, vm);
        vm.notify();
    } catch (std::exception& e) {
        logger->error(1, e.what());
        logger->write();
        exit(2);
    }
    if (vm.count("help")) {
        std::cerr << "Usage: " << program_name << " [global options] ";
        std::cerr << command_name << " [options]";
        for (auto arg: required_args) std::cerr << " " << arg;
        for (auto arg: optional_args) std::cerr << " [" << arg;
        if (trailing_args != NULL)
            std::cerr << " [" << trailing_args << " ... ]";
        for (unsigned i=0; i<optional_args.size(); i++) std::cerr << "]"; 
        std::cerr << std::endl << std::endl;
        std::cerr << global_opts << std::endl;
        std::cerr << subcommand_descr;
        exit(1);
    }
    for (auto arg: required_args) {
        if (vm.count(arg) == 0) {
            logger->error(1, std::string("Missing required argument ") + arg);
            logger->write();
            exit(2);
        }
    }
}

int main(int argc, char** argv) {
    program_name = argv[0];
#ifdef __unix__
    if (!isatty(fileno(stdout))) logger = new CottonJSONLogger;
    else logger = new CottonTTYLogger;
#else
    logger = new CottonTTYLogger;
#endif
    po::options_description global("Generic options");
    global.add_options()
        ("help,h", "produce help message")
        ("box-root,b", po::value<std::string>(), "specify a different folder to put sandboxes in")
        ("json,j", "force JSON output");

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("command", po::value<std::string>(), "command to execute")
        ("command-args", po::value<std::vector<std::string>>(), "arguments for command");

    po::positional_options_description pos;
    pos.add("command", 1).
        add("command-args", -1);

    po::variables_map vm;

    std::vector<std::string> opts;
    po::options_description generic_opts("Generic options");
    generic_opts.add(global).add(hidden);
    try {
        po::parsed_options parsed = po::command_line_parser(argc, argv)
            .options(generic_opts)
            .positional(pos)
            .allow_unregistered()
            .run();
        po::store(parsed, vm);
        vm.notify();
        if (vm.count("command")) {
            opts = po::collect_unrecognized(parsed.options, po::include_positional);
            opts.erase(opts.begin());
        }
    } catch (std::exception& e) {
        logger->error(1, e.what());
        logger->write();
        return 2;
    }

    if (vm.count("help") == 1 && vm.count("command") == 0) {
        std::cerr << "Usage: " << program_name << " [options] command [command-args...]" << std::endl;
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

    if (vm.count("json") && logger->isttylogger()) {
        delete logger;
        logger = new CottonJSONLogger;
    }
#ifdef __unix__
    std::string box_root = "/tmp";
#else
    std::string box_root = "who knows";
#endif
    if (vm.count("box-root") == 1) box_root = vm["box-root"].as<std::string>();

    if (!vm.count("command")) {
        logger->error(1, "No command given!");
    } else {
        try {
            std::string cmd = vm["command"].as<std::string>();
            if (cmd == "list") {
                parse_subcommand_options(global, opts, vm, "list", {}, {}, {});
            } else if (cmd == "create") {
                parse_subcommand_options(global, opts, vm, "create", {}, {"box-type"}, {});
            } else if (cmd == "check") {
                parse_subcommand_options(global, opts, vm, "check", {}, {"box-id"}, {});
            } else if (cmd == "get-root") {
                parse_subcommand_options(global, opts, vm, "get-root", {}, {"box-id"}, {});
            } else if (cmd == "set-limit") {
                parse_subcommand_options(global, opts, vm, "set-limit", {}, {"box-id", "what", "value"}, {});
            } else if (cmd == "get-limit") {
                parse_subcommand_options(global, opts, vm, "get-limit", {}, {"box-id", "what"}, {});
            } else if (cmd == "get-redirect") {
                parse_subcommand_options(global, opts, vm, "get-redirect", {}, {"box-id", "what"}, {});
            } else if (cmd == "set-redirect") {
                parse_subcommand_options(global, opts, vm, "set-redirect", {}, {"box-id", "what"}, {"value"});
            } else if (cmd == "mount") {
                parse_subcommand_options(global, opts, vm, "mount", {{"rw", "read-write"}}, {"box-id"}, {"box_path", "orig_path"});
            } else if (cmd == "umount") {
                parse_subcommand_options(global, opts, vm, "umount", {}, {"box-id", "box_path"}, {});
            } else if (cmd == "run") {
                parse_subcommand_options(global, opts, vm, "run", {}, {"box-id", "exec_path"}, {}, "arg");
            } else if (cmd == "get") {
                parse_subcommand_options(global, opts, vm, "get", {}, {"box-id", "statistic"}, {});
            } else if (cmd == "clear") {
                parse_subcommand_options(global, opts, vm, "clear", {}, {"box-id"}, {});
            } else if (cmd == "delete") {
                parse_subcommand_options(global, opts, vm, "delete", {}, {"box-id"}, {});
            } else
                logger->error(254, "Unknown command!");
        } catch (std::exception& e) {
            logger->error(255, std::string("Unhandled exception! ") + e.what());
        }
    }
    logger->write();
}
