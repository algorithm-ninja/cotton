#include "box.hpp"
#include "logger.hpp"
#include <vector>
#include <fstream>
#include "util.hpp"
#include <option.hpp>
#include <positional.hpp>
#include <command.hpp>
#ifdef COTTON_UNIX
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#endif
#ifdef COTTON_WINDOWS
#include <windows.h>
#include <tchar.h>
#endif

BoxCreators* box_creators;
CottonLogger* logger;

#ifdef COTTON_UNIX
void sig_handler(int sig) {
    logger->error(255, "Received signal " + std::to_string(sig) + " (" + strsignal(sig) + ")");
    logger->write();
    _Exit(0);
}
#endif

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
        Sandbox* ptr = s.get();
        oa << ptr;
    } catch (std::exception& e) {
        logger->error(3, std::string("Error saving the sandbox: ") + e.what());
    }
}

namespace program_options {

template<>
time_limit_t from_char_ptr(const char* ptr) {
    return time_limit_t(from_char_ptr<double>(ptr));
}

template<>
space_limit_t from_char_ptr(const char* ptr) {
    return space_limit_t(from_char_ptr<double>(ptr));
}

DEFINE_OPTION(help, "print this message", 'h');
DEFINE_OPTION(box_root, "specify a different folder to put sandboxes in", 'r');
DEFINE_OPTION(json, "force JSON output", 'j');
DEFINE_OPTION(box_id, "id of the sandbox", 'b');
DEFINE_OPTION(box_type, "type of the sandbox to be created");
DEFINE_OPTION(value, "value to set");
DEFINE_OPTION(stream, "the stream to operate on");
DEFINE_OPTION(external_path, "path on the system");
DEFINE_OPTION(internal_path, "path in the sandbox");
DEFINE_OPTION(rw, "read-write");
DEFINE_OPTION(exec, "executable to run");
DEFINE_OPTION(arg, "arguments for the executable");

DEFINE_COMMAND(list, "list available implementations");
DEFINE_COMMAND(create, "create a sandbox",
    positional<_box_type, const char*, 1>());
DEFINE_COMMAND(check, "check if a sandbox is consistent");
DEFINE_COMMAND(get_root, "gets the root path of the sandbox");
DEFINE_COMMAND(cpu_limit, "gets or sets the cpu time limit",
    positional<_value, time_limit_t, 0, 1>());
DEFINE_COMMAND(wall_limit, "gets or sets the wall clock limit",
    positional<_value, time_limit_t, 0, 1>());
DEFINE_COMMAND(memory_limit, "gets or sets the memory limit",
    positional<_value, space_limit_t, 0, 1>());
DEFINE_COMMAND(disk_limit, "gets or sets the disk limit",
    positional<_value, space_limit_t, 0, 1>());
DEFINE_COMMAND(process_limit, "gets or sets the process limit",
    positional<_value, int, 0, 1>());
DEFINE_COMMAND(redirect, "gets or sets i/o redirections",
    positional<_stream, const char*, 1>(),
    positional<_value, const char*, 0, 1>());
DEFINE_COMMAND(mount, "enables paths in the sandbox, or gets information on a path",
    option<_rw, void>(),
    positional<_internal_path, const char*, 1>(),
    positional<_external_path, const char*, 0, 1>());
DEFINE_COMMAND(umount, "disables paths in the sandbox",
    positional<_internal_path, const char*, 1>());
DEFINE_COMMAND(run, "run program in the sandbox",
    positional<_exec, const char*, 1>(),
    positional<_arg, const char*, 0, 1000>());
DEFINE_COMMAND(running_time, "get last command's cpu time");
DEFINE_COMMAND(wall_time, "get last command's wall time");
DEFINE_COMMAND(memory_usage, "get last command's memory usage");
DEFINE_COMMAND(status, "get last command's exit reason");
DEFINE_COMMAND(return_code, "get last command's return code");
DEFINE_COMMAND(signal, "get last command's killing signal");
DEFINE_COMMAND(clear, "resets the sandbox to a clean state");
DEFINE_COMMAND(destroy, "deletes the sandbox");

DEFINE_COMMAND(cotton, "Cotton sandbox",
    option<_help, void>(),
    option<_box_root, const char*>("/tmp"),
    option<_json, void>(),
    option<_box_id, const char*>(),
    &list_command,
    &create_command,
    &check_command,
    &get_root_command,
    &cpu_limit_command,
    &wall_limit_command,
    &memory_limit_command,
    &disk_limit_command,
    &process_limit_command,
    &redirect_command,
    &mount_command,
    &umount_command,
    &run_command,
    &running_time_command,
    &wall_time_command,
    &memory_usage_command,
    &status_command,
    &return_code_command,
    &signal_command,
    &clear_command,
    &destroy_command);

template<>
void command_callback(const decltype(cotton_command)& cc) {
    if (cc.has_option<_help>()) {
        cc.print_help(std::cerr);
        exit(0);
    }
    if (cc.has_option<_json>()) {
        delete logger;
        logger = new CottonJSONLogger;
    }
}

#define TEST_FEATURE(feature) if (features & Sandbox::feature) std::get<2>(res.back()).emplace_back(#feature);
template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(list_command)& lc) {
    std::vector<std::tuple<std::string, int, std::vector<std::string>>> res;
    for (const auto& creator: *box_creators) {
        std::unique_ptr<Sandbox> s(creator.second(cc.get_option<_box_root>()));
        s->set_error_handler(logger->get_error_function());
        s->set_warning_handler(logger->get_warning_function());
        if (!s->is_available()) continue;
        res.emplace_back(creator.first, s->get_overhead(), std::vector<std::string>{});
        Sandbox::feature_mask_t features = s->get_features();
        TEST_FEATURE(memory_limit);
        TEST_FEATURE(cpu_limit);
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
    logger->result(res);
}
#undef TEST_FEATURE

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(create_command)& crc) {
    if (!box_creators->count(crc.get_positional<_box_type>()[0])) {
        logger->error(2, "The given box type does not exist!");
        return;
    }
    auto box_creator = (*box_creators)[crc.get_positional<_box_type>()[0]];
    std::unique_ptr<Sandbox> s(box_creator(cc.get_option<_box_root>()));
    s->set_error_handler(logger->get_error_function());
    s->set_warning_handler(logger->get_warning_function());
    if (!s->is_available()) {
        logger->error(2, "The given box type is not available!");
        return;
    }
    s->create_box();
    save_box(cc.get_option<_box_root>(), s);
    logger->result(s->get_id());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(check_command)& chc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? false : s->check());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(get_root_command)& grc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? "" : s->get_root());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(memory_limit_command)& lc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    if (lc.count_positional<_value>() > 0) {
        const auto& val = lc.get_positional<_value>()[0];
        logger->result(s.get() == nullptr ? false : s->set_memory_limit(val));
        save_box(cc.get_option<_box_root>(), s);
    } else {
        logger->result(s.get() == nullptr ? 0 : s->get_memory_limit());
    }
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(cpu_limit_command)& lc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    if (lc.count_positional<_value>() > 0) {
        const auto& val = lc.get_positional<_value>()[0];
        logger->result(s.get() == nullptr ? false : s->set_time_limit(val));
        save_box(cc.get_option<_box_root>(), s);
    } else {
        logger->result(s.get() == nullptr ? 0 : s->get_time_limit());
    }
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(wall_limit_command)& lc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    if (lc.count_positional<_value>() > 0) {
        const auto& val = lc.get_positional<_value>()[0];
        logger->result(s.get() == nullptr ? false : s->set_wall_time_limit(val));
        save_box(cc.get_option<_box_root>(), s);
    } else {
        logger->result(s.get() == nullptr ? 0 : s->get_wall_time_limit());
    }
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(process_limit_command)& lc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    if (lc.count_positional<_value>() > 0) {
        const auto& val = lc.get_positional<_value>()[0];
        logger->result(s.get() == nullptr ? false : s->set_process_limit(val));
        save_box(cc.get_option<_box_root>(), s);
    } else {
        logger->result(s.get() == nullptr ? 0 : s->get_process_limit());
    }
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(disk_limit_command)& lc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    if (lc.count_positional<_value>() > 0) {
        const auto& val = lc.get_positional<_value>()[0];
        logger->result(s.get() == nullptr ? false : s->set_disk_limit(val));
        save_box(cc.get_option<_box_root>(), s);
    } else {
        logger->result(s.get() == nullptr ? 0 : s->get_disk_limit());
    }
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(redirect_command)& rc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    std::string stream = rc.get_positional<_stream>()[0];
    if (rc.count_positional<_value>() > 0) {
        if (s.get() == nullptr) {
            logger->result(false);
            return;
        }
        std::string val = rc.get_positional<_value>()[0];
        if (val == "-") val = "";
        if (stream == "stdin") {
            logger->result(s->redirect_stdin(val));
        } else if (stream == "stdout") {
            logger->result(s->redirect_stdout(val));
        } else if (stream == "stderr") {
            logger->result(s->redirect_stderr(val));
        } else {
            logger->error(2, "Invalid redirect type given");
            logger->result(false);
        }
        save_box(cc.get_option<_box_root>(), s);
    } else {
        if (s.get() == nullptr) {
            logger->result("");
            return;
        }
        if (stream == "stdin") {
            logger->result(s->get_stdin());
        } else if (stream == "stdout") {
            logger->result(s->get_stdout());
        } else if (stream == "stderr") {
            logger->result(s->get_stderr());
        } else {
            logger->error(2, "Invalid redirect type given");
            logger->result(false);
        }
    }
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(mount_command)& mc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    std::string inner_path = mc.get_positional<_internal_path>()[0];
    if (mc.count_positional<_external_path>() > 0) {
        std::string val = mc.get_positional<_external_path>()[0];
        logger->result(s.get() == nullptr ? false : s->mount(inner_path, val, mc.has_option<_rw>()));
        save_box(cc.get_option<_box_root>(), s);
    } else {
        logger->result(s.get() == nullptr ? "" : s->mount(inner_path));
    }
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(umount_command)& uc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    std::string inner_path = uc.get_positional<_internal_path>()[0];
    logger->result(s.get() == nullptr ? false : s->umount(inner_path));
    save_box(cc.get_option<_box_root>(), s);
}


template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(run_command)& rc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    std::string exec = rc.get_positional<_exec>()[0];
    auto args = rc.get_positional<_arg>();
    std::vector<std::string> s_args;
    for (const auto str: args) s_args.emplace_back(str);
    logger->result(s.get() == nullptr ? false : s->run(exec, s_args));
    save_box(cc.get_option<_box_root>(), s);
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(memory_usage_command)& muc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? space_limit_t(0) : s->get_memory_usage());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(running_time_command)& rtc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? time_limit_t(0) : s->get_running_time());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(wall_time_command)& rtc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? time_limit_t(0) : s->get_wall_time());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(status_command)& rtc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? "" : s->get_status());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(return_code_command)& rtc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? 0 : s->get_return_code());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(signal_command)& rtc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? 0 : s->get_signal());
}

template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(clear_command)& rtc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? false : s->clear());
    save_box(cc.get_option<_box_root>(), s);
}


template<>
void command_callback(const decltype(cotton_command)& cc, const decltype(destroy_command)& rtc) {
    if (!cc.has_option<_box_id>()) {
        logger->error(2, "You need to specify a box id!");
        return;
    }
    auto s = load_box(cc.get_option<_box_root>(), cc.get_option<_box_id>());
    logger->result(s.get() == nullptr ? false : s->delete_box());
}

} // namespace program_options

int main(int argc, const char** argv) {
#ifdef COTTON_UNIX
    // Immediately drop privileges if the program is setuid, do nothing otherwise.
    setreuid(geteuid(), getuid());

    if (!isatty(fileno(stdout))) logger = new CottonJSONLogger;
    else logger = new CottonTTYLogger;

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
#else
    logger = new CottonTTYLogger;
#endif
    try {
        program_options::cotton_command.parse(argc, argv);
    } catch (std::exception& e) {
        logger->error(255, std::string("Unandled exception! ") + e.what());
    }
    logger->write();
}
