#include "box.hpp"
#include "logger.hpp"
#include <vector>
#include <fstream>
#include <boost/program_options.hpp>
#ifdef __unix__
#include <signal.h>
#include <string.h>
#endif
namespace po = boost::program_options;

BoxCreators box_creators __attribute__((init_priority(101)));
CottonLogger* logger;
std::string program_name;

#ifdef __unix__
void sig_handler(int sig) {
    logger->error(255, "Received signal " + std::to_string(sig) + " (" + strsignal(sig) + ")");
    logger->write();
    exit(0);
}
#endif


#define TEST_FEATURE(feature) if (features & Sandbox::feature) std::get<2>(res.back()).emplace_back(#feature);
std::vector<std::tuple<std::string, int, std::vector<std::string>>> list_boxes(const std::string& box_root) {
    std::vector<std::tuple<std::string, int, std::vector<std::string>>> res;
    for (const auto& creator: box_creators) {
        std::unique_ptr<Sandbox> s(creator.second(box_root));
        s->set_error_handler(logger->get_error_function());
        s->set_warning_handler(logger->get_warning_function());
        if (!s->is_available()) continue;
        res.emplace_back(creator.first, s->get_penality(), std::vector<std::string>{});
        Sandbox::feature_mask_t features = s->get_features();
        TEST_FEATURE(memory_limit);
        TEST_FEATURE(time_limit);
        TEST_FEATURE(wall_time_limit);
        TEST_FEATURE(process_limit);
        TEST_FEATURE(process_limit_full);
        TEST_FEATURE(disk_limit);
        TEST_FEATURE(disk_limit_full);
        TEST_FEATURE(folder_mount);
        TEST_FEATURE(memory_usage);
        TEST_FEATURE(running_time);
        TEST_FEATURE(wall_time);
        TEST_FEATURE(clearable);
        TEST_FEATURE(process_isolation);
        TEST_FEATURE(io_redirection);
        TEST_FEATURE(network_isolation);
        TEST_FEATURE(return_code);
        TEST_FEATURE(signal);
    }
    return res;
}
#undef TEST_FEATURE

size_t parse_time_limit(const std::string& s) {
    return std::stod(s)*1000000UL;
}

size_t parse_space_limit(const std::string& s) {
    return std::stoi(s)*1024UL;
}

std::unique_ptr<Sandbox> load_box(const std::string& box_root, const std::string& box_id) {
    try {
        std::ifstream fin(Sandbox::box_base_path(box_root, std::stoi(box_id)) + "boxinfo");
        boost::archive::text_iarchive ia{fin};
        Sandbox* s;
        ia >> s;
        s->set_error_handler(logger->get_error_function());
        s->set_warning_handler(logger->get_warning_function());
        return std::unique_ptr<Sandbox>(s);
    } catch (std::exception& e) {
        logger->error(3, std::string("Error loading the sandbox: ") + e.what());
        return nullptr;
    }
}

void save_box(const std::string& box_root, std::unique_ptr<Sandbox>& s) {
    try {
        std::ofstream fout(Sandbox::box_base_path(box_root, s->get_id()) + "boxinfo");
        boost::archive::text_oarchive oa{fout};
        oa << s.get();
    } catch (std::exception& e) {
        logger->error(3, std::string("Error saving the sandbox: ") + e.what());
    }
}


size_t create_box(const std::string& box_root, const std::string& box_type) {
    if (!box_creators.count(box_type)) {
        logger->error(2, "The given box type does not exist!");
        return 0;
    }
    std::unique_ptr<Sandbox> s(box_creators[box_type](box_root));
    s->set_error_handler(logger->get_error_function());
    s->set_warning_handler(logger->get_warning_function());
    if (!s->is_available()) {
        logger->error(2, "The given box type is not available!");
        return 0;
    }
    s->create_box();
    save_box(box_root, s);
    return s->get_id();
}

bool check_box(const std::string& box_root, const std::string& box_id) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    return s->check();
}

std::string get_root(const std::string& box_root, const std::string& box_id) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return "";
    return s->get_root();
}

bool set_limit(const std::string& box_root, const std::string& box_id, const std::string& what, const std::string& value) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    try {
        if (what == "memory-limit") {
            bool ret = s->set_memory_limit(parse_space_limit(value));
            save_box(box_root, s);
            return ret;
        } else if (what == "time-limit") {
            bool ret = s->set_time_limit(parse_time_limit(value));
            save_box(box_root, s);
            return ret;
        } else if (what == "wall-time-limit") {
            bool ret = s->set_wall_time_limit(parse_time_limit(value));
            save_box(box_root, s);
            return ret;
        } else if (what == "process-limit") {
            bool ret = s->set_process_limit(std::stoi(value));
            save_box(box_root, s);
            return ret;
        } else if (what == "disk-limit") {
            bool ret = s->set_disk_limit(parse_space_limit(value));
            save_box(box_root, s);
            return ret;
        } else {
            logger->error(2, "Invalid limit type given!");
            return false;
        }
    } catch (std::exception& e) {
            logger->error(2, "Invalid limit value given!");
            return false;
    }
}

size_t get_limit(const std::string& box_root, const std::string& box_id, const std::string& what) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return 0;
    if (what == "memory-limit") {
        return s->get_memory_limit();
    } else if (what == "time-limit") {
        return s->get_time_limit();
    } else if (what == "wall-time-limit") {
        return s->get_wall_time_limit();
    } else if (what == "process-limit") {
        return s->get_process_limit();
    } else if (what == "disk-limit") {
        return s->get_disk_limit();
    } else {
        logger->error(2, "Invalid limit type given!");
        return 0;
    }
}

std::string get_redirect(const std::string& box_root, const std::string& box_id, const std::string& what) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return "";
    if (what == "stdout") {
        return s->get_stdout();
    } else if (what == "stderr") {
        return s->get_stderr();
    } else if (what == "stdin") {
        return s->get_stdin();
    } else {
        logger->error(2, "Invalid redirect type given!");
        return "";
    }
}

bool set_redirect(const std::string& box_root, const std::string& box_id, const std::string& what, const std::string& value = "") {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    if (what == "stdout") {
        bool ret = s->redirect_stdout(value);
        save_box(box_root, s);
        return ret;
    } else if (what == "stderr") {
        bool ret = s->redirect_stderr(value);
        save_box(box_root, s);
        return ret;
    } else if (what == "stdin") {
        bool ret = s->redirect_stdin(value);
        save_box(box_root, s);
        return ret;
    } else {
        logger->error(2, "Invalid redirect type given!");
        return false;
    }
}

std::vector<std::pair<std::string, std::string>> mount_info(const std::string& box_root, const std::string& box_id) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return {};
    return s->mount();
}

std::string mount_info(const std::string& box_root, const std::string& box_id, const std::string& box_path) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return "";
    return s->mount(box_path);
}

bool do_mount(const std::string& box_root, const std::string& box_id, const std::string& box_path, const std::string& orig_path, bool rw = false) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    bool res = s->mount(box_path, orig_path, rw);
    save_box(box_root, s);
    return res;
}

bool do_umount(const std::string& box_root, const std::string& box_id, const std::string& box_path) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    bool res = s->umount(box_path);
    save_box(box_root, s);
    return res;
}

bool run(const std::string& box_root, const std::string& box_id, const std::string& exec_path, const std::vector<std::string>& args) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    bool res = s->run(exec_path, args);
    save_box(box_root, s);
    return res;
}

size_t get_stat(const std::string& box_root, const std::string& box_id, const std::string& what) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return 0;
    if (what == "memory-usage") {
        return s->get_memory_usage();
    } else if (what == "running-time") {
        return s->get_running_time();
    } else if (what == "wall-time") {
        return s->get_wall_time();
    } else if (what == "return-code") {
        return s->get_return_code();
    } else if (what == "signal") {
        return s->get_signal();
    } else {
        logger->error(2, "Invalid statistic type given!");
        return 0;
    }
}

bool clear_box(const std::string& box_root, const std::string& box_id) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    bool res = s->clear();
    save_box(box_root, s);
    return res;
}

bool delete_box(const std::string& box_root, const std::string& box_id) {
    auto s = load_box(box_root, box_id);
    if (s.get() == nullptr) return false;
    return s->delete_box();
}

std::vector<std::string> parse_subcommand_options(
    po::options_description& global_opts,
    const std::vector<std::string>& opts,
    po::variables_map& vm,
    const std::string& command_name,
    const std::vector<std::pair<const char*, const char*>>& flags,
    const std::vector<const char*>& required_args,
    const std::vector<const char*>& optional_args,
    const char* trailing_args = nullptr) {
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

    std::vector<std::string> trailing_vals;
    if (trailing_args != nullptr) {
        hidden_args.add_options()(trailing_args, po::value<std::vector<std::string>>(&trailing_vals)->composing(), "");
        pos.add(trailing_args, -1);
    }

    po::options_description command_opts;
    command_opts.add(subcommand_descr).add(hidden_args);

    try {
        po::parsed_options parsed = po::command_line_parser(opts)
            .options(command_opts)
            .positional(pos)
            .allow_unregistered()
            .run();

        po::store(parsed, vm);
        vm.notify();
        trailing_vals = po::collect_unrecognized(parsed.options, po::include_positional);
        for (unsigned i=0; i<required_args.size(); i++)
            trailing_vals.erase(trailing_vals.begin());
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
        if (trailing_args != nullptr)
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
    return trailing_vals;
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
        std::cerr << " list         list available implementations" << std::endl;
        std::cerr << " check        check if a sandbox is consistent" << std::endl;
        std::cerr << " set-limit    set some limit for the program execution" << std::endl;
        std::cerr << " get-limit    read a current limit" << std::endl;
        std::cerr << " set-redirect set io redirection" << std::endl;
        std::cerr << " get-redirect get io redirection" << std::endl;
        std::cerr << " mount        make paths readable to the process" << std::endl;
        std::cerr << " umount       disable paths" << std::endl;
        std::cerr << " run          execute command" << std::endl;
        std::cerr << " get          get information on the last execution" << std::endl;
        std::cerr << " clear        cleanup the sandbox" << std::endl;
        std::cerr << " delete       delete the sandbox" << std::endl;
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
                logger->result(list_boxes(box_root));
            } else if (cmd == "create") {
                parse_subcommand_options(global, opts, vm, "create", {}, {"box-type"}, {});
                logger->result(create_box(box_root, vm["box-type"].as<std::string>()));
            } else if (cmd == "check") {
                parse_subcommand_options(global, opts, vm, "check", {}, {"box-id"}, {});
                logger->result(check_box(box_root, vm["box-id"].as<std::string>()));
            } else if (cmd == "get-root") {
                parse_subcommand_options(global, opts, vm, "get-root", {}, {"box-id"}, {});
                logger->result(get_root(box_root, vm["box-id"].as<std::string>()));
            } else if (cmd == "set-limit") {
                parse_subcommand_options(global, opts, vm, "set-limit", {}, {"box-id", "what", "value"}, {});
                logger->result(set_limit(box_root, vm["box-id"].as<std::string>(), vm["what"].as<std::string>(), vm["value"].as<std::string>()));
            } else if (cmd == "get-limit") {
                parse_subcommand_options(global, opts, vm, "get-limit", {}, {"box-id", "what"}, {});
                logger->result(get_limit(box_root, vm["box-id"].as<std::string>(), vm["what"].as<std::string>()));
            } else if (cmd == "get-redirect") {
                parse_subcommand_options(global, opts, vm, "get-redirect", {}, {"box-id", "what"}, {});
                logger->result(get_redirect(box_root, vm["box-id"].as<std::string>(), vm["what"].as<std::string>()));
            } else if (cmd == "set-redirect") {
                parse_subcommand_options(global, opts, vm, "set-redirect", {}, {"box-id", "what"}, {"value"});
                if (vm.count("value") == 0)
                    logger->result(set_redirect(box_root, vm["box-id"].as<std::string>(), vm["what"].as<std::string>()));
                else
                    logger->result(set_redirect(box_root, vm["box-id"].as<std::string>(), vm["what"].as<std::string>(), vm["value"].as<std::string>()));
            } else if (cmd == "mount") {
                parse_subcommand_options(global, opts, vm, "mount", {{"rw", "read-write"}}, {"box-id"}, {"box_path", "orig_path"});
                if (vm.count("box_path") == 0)
                    logger->result(mount_info(box_root, vm["box-id"].as<std::string>()));
                else if (vm.count("orig_path") == 0)
                    logger->result(mount_info(box_root, vm["box-id"].as<std::string>(), vm["box_path"].as<std::string>()));
                else
                    logger->result(do_mount(box_root, vm["box-id"].as<std::string>(), vm["box_path"].as<std::string>(), vm["orig_path"].as<std::string>(), vm.count("rw")));
            } else if (cmd == "umount") {
                parse_subcommand_options(global, opts, vm, "umount", {}, {"box-id", "box_path"}, {});
                logger->result(do_umount(box_root, vm["box-id"].as<std::string>(), vm["box_path"].as<std::string>()));
            } else if (cmd == "run") {
                auto args = parse_subcommand_options(global, opts, vm, "run", {}, {"box-id", "exec_path"}, {}, "arg");
                logger->result(run(box_root, vm["box-id"].as<std::string>(), vm["exec_path"].as<std::string>(), args));
            } else if (cmd == "get") {
                parse_subcommand_options(global, opts, vm, "get", {}, {"box-id", "statistic"}, {});
                logger->result(get_stat(box_root, vm["box-id"].as<std::string>(), vm["statistic"].as<std::string>()));
            } else if (cmd == "clear") {
                parse_subcommand_options(global, opts, vm, "clear", {}, {"box-id"}, {});
                logger->result(clear_box(box_root, vm["box-id"].as<std::string>()));
            } else if (cmd == "delete") {
                parse_subcommand_options(global, opts, vm, "delete", {}, {"box-id"}, {});
                logger->result(delete_box(box_root, vm["box-id"].as<std::string>()));
            } else
                logger->error(1, "Unknown command!");
        } catch (std::exception& e) {
            logger->error(255, std::string("Unhandled exception! ") + e.what());
        }
    }
    logger->write();
}
