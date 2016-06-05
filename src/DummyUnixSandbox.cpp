#include "util.hpp"
#ifdef COTTON_UNIX
#include "DummyUnixSandbox.hpp"
#include <limits>
#include <chrono>
#include <thread>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>

DummyUnixSandbox::BoxLocker::BoxLocker(const DummyUnixSandbox* box, const std::string& lock): box(box) {
    lock_name = box->get_root() + "../" + lock;
    if (open(lock_name.c_str(), O_RDWR | O_CREAT | O_EXCL, DummyUnixSandbox::file_mode) == -1) {
        box->error(4, serror("Error acquiring lock " + lock_name));
        has_lock_ = false;
    } else {
        has_lock_ = true;
    }
}

DummyUnixSandbox::BoxLocker::~BoxLocker() {
    if (!has_lock_) return;
    if (unlink(lock_name.c_str()) == -1)
        box->warning(4, serror("Error removing lock " + lock_name));
}


bool DummyUnixSandbox::send_error(int error_id, int err) {
    int tmp[2] = {error_id, err};
    return 2*sizeof(int) == write(comm[1], (void*)tmp, 2*sizeof(int));
}

int DummyUnixSandbox::get_error(int& error_id, int& err) {
    union {int i[2]; char c[2*sizeof(int)];} tmp;
    unsigned buff_fill = 0;
    while (buff_fill < 2*sizeof(int)) {
        int nread = read(comm[0], tmp.c+buff_fill, 2*sizeof(int)-buff_fill);
        if (nread == -1 && errno == EINTR) continue;
        if (nread <= 0) return nread;
        buff_fill += nread;
    }
    error_id = tmp.i[0];
    err = tmp.i[1];
    return 1;
}

std::string DummyUnixSandbox::err_string(int error_id) const {
    switch (error_id) {
        case -1: return "Error setting stack limit";
        case -2: return "Error setting memory limit";
        case -3: return "Error setting time limit";
        case -4: return "Error setting process limit";
        case -5: return "Error setting disk limit";
        case 1: return "Cannot open stdin file";
        case 2: return "Cannot open stdout file";
        case 3: return "Cannot open stderr file";
        case 4: return "execv failed";
        case 5: return "chdir failed";
        default: return "Unknown error";
    }
}

bool DummyUnixSandbox::prepare_io_redirect(const std::string& file, std::string& redir, mode_t mode) {
    if (file == "") {
        redir = file;
        return true;
    }
    int fd = open((get_root() + file).c_str(), mode, file_mode);
    if (fd == -1) error(4, serror("Cannot open file " + file));
    else {
        close(fd);
        redir = file;
    }
    return fd != -1;
}

bool DummyUnixSandbox::setup_io_redirect(const std::string& file, int dest_fd, mode_t mode) {
    if (file == "") return true;
    int src_fd = open((get_root() + file).c_str(), mode);
    if (src_fd == -1) {
        send_error(1, errno);
        return false;
    }
    dup2(src_fd, dest_fd);
    return true;
}


[[noreturn]] void DummyUnixSandbox::box_inner(const std::string& command, const std::vector<std::string>& args) {
    // Set up IO redirection.
    if (!setup_io_redirect(stdin_, fileno(stdin), O_RDONLY)) exit(1);
    if (!setup_io_redirect(stdout_, fileno(stdout), O_RDWR)) exit(1);
    if (!setup_io_redirect(stderr_, fileno(stderr), O_RDWR)) exit(1);

    // Make the pipe close on the call to exec()
    fcntl(comm[1], F_SETFD, FD_CLOEXEC);

    // Build arguments for execve
    int trailing_slash_count = 0;
    while (command[trailing_slash_count] == '/') trailing_slash_count++;
    std::string executable = command.substr(trailing_slash_count);
    std::vector<char*> e_args;
    char* executable_c_str = new char[executable.size()+1];
    strcpy(executable_c_str, executable.c_str());
    e_args.push_back(executable_c_str);

    for (auto arg: args) {
        char* tmp = new char[arg.size()+1];
        strcpy(tmp, arg.c_str());
        e_args.push_back(tmp);
    }
    e_args.push_back(nullptr);

    // Change directory to box_root
    if (chdir(get_root().c_str()) != 0) {
        send_error(5, errno);
        exit(1);
    }

    // Set up limits
    struct rlimit rlim;
    rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_STACK, &rlim) == -1) send_error(-1, errno);
    // Apple and Linux disagree on the RLIMIT_AS unit
    if (mem_limit.rlimit_unit() != 0) {
        rlim.rlim_cur = rlim.rlim_max = mem_limit.rlimit_unit();
        if (setrlimit(RLIMIT_AS, &rlim) == -1) send_error(-2, errno);
    }
    if (time_limit.seconds() != 0) {
        rlim.rlim_cur = rlim.rlim_max = time_limit.seconds();
        if (setrlimit(RLIMIT_CPU, &rlim) == -1) send_error(-3, errno);
    }
    if (process_limit) {
        rlim.rlim_cur = rlim.rlim_max = process_limit;
        if (setrlimit(RLIMIT_NPROC, &rlim) == -1) send_error(-4, errno);
    }
    // Should be bytes both on Apple and Linux 
    if (disk_limit.bytes()) {
        rlim.rlim_cur = rlim.rlim_max = disk_limit.bytes();
        if (setrlimit(RLIMIT_FSIZE, &rlim) == -1) send_error(-5, errno);
        rlim.rlim_cur = rlim.rlim_max = 0;
        if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) send_error(-5, errno);
    }
    if (!pre_exec_hook()) exit(1);
    // Set all privileges to the effective user id
    // ie. drop privileges if the program is setuid, do nothing otherwise
    setreuid(geteuid(), getuid());
    setuid(getuid());
    execv(executable.c_str(), &e_args[0]);
    send_error(4, errno);
    exit(1);
}

bool DummyUnixSandbox::box_checker(pid_t box_pid) {
    int error_id = 0;
    int err_msg = 0;
    int err_ret;
    bool timed_out = false;
    while ((err_ret = get_error(error_id, err_msg)) != 0) {
        if (err_ret == -1) {
            error(5, serror("Error getting errors"));
            return false;
        }
        if (error_id < 0) {
            warning(5, serror(err_string(error_id), err_msg));
        } else {
            error(5, serror(err_string(error_id), err_msg));
            return false;
        }
    }
    // If we arrive here, exec() was successfully executed in the child.
    auto start = std::chrono::high_resolution_clock::now();
    int ret = 0;
    if (wall_time_limit.microseconds() > 0) {
        auto now = std::chrono::high_resolution_clock::now();
        size_t micros = std::chrono::duration_cast<std::chrono::microseconds>(now-start).count();
        bool waited = false;
        while (micros < wall_time_limit.microseconds()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            int what = waitpid(box_pid, &ret, WNOHANG);
            if (what > 0) {
                waited = true;
                break;
            }
            now = std::chrono::high_resolution_clock::now();
            micros = std::chrono::duration_cast<std::chrono::microseconds>(now-start).count();
        }
        if (!waited) {
            timed_out = true;
            kill(box_pid, SIGKILL);
            waitpid(box_pid, &ret, 0);
        }
    } else {
        waitpid(box_pid, &ret, 0);
    }
    // The child has exited, collect statistics
    auto now = std::chrono::high_resolution_clock::now();
    return_code = WIFEXITED(ret) ? WEXITSTATUS(ret) : 0;
    signal = WIFSIGNALED(ret) ? WTERMSIG(ret) : 0;
    if (timed_out) exit_status = "Timed out";
    else exit_status = WIFSIGNALED(ret) ? "Signaled" : "Terminated normally";
    wall_time = now-start;
    struct rusage stats;
    getrusage(RUSAGE_CHILDREN, &stats);
    memory_usage = space_limit_t::from_rlimit_unit(stats.ru_maxrss);
    running_time = stats.ru_utime;
    running_time += stats.ru_stime;
    return true;
}

size_t DummyUnixSandbox::create_box(size_t box_limit) {
    //TODO: fix this for the many weird things that could possibly happen
    for (size_t box_id = 1; box_id < box_limit; box_id++) {
        std::string current_attempt = box_base_path(base_path, box_id);
        if (mkdir(current_attempt.c_str(), box_mode) == -1 && errno != EEXIST) {
            error(4, serror("Error creating sandbox " + current_attempt));
            return 0;
        }

        struct stat statbuf;
        stat(current_attempt.c_str(), &statbuf);
        if (!S_ISDIR(statbuf.st_mode)) continue;

        current_attempt += "lock";
        int res = open(current_attempt.c_str(), O_RDWR | O_CREAT | O_EXCL, file_mode);
        if (res == -1 && errno == EEXIST) continue;
        if (res == -1) {
            warning(4, serror("Something weird happened creating sandbox " + current_attempt));
            continue;
        }
        current_attempt = box_base_path(base_path, box_id) + "file_root/";
        int err = rm_rf(current_attempt);
        if (err && err != ENOENT) {
            error(4, serror("Error deleting old file_root " + current_attempt, err));
            return 0;
        }
        if (mkdir(current_attempt.c_str(), box_mode) == -1) {
            error(4, serror("Error creating file_root " + current_attempt));
            return 0;
        }
        id_ = box_id;
        return id_;
    }
    error(4, "Could not find a free box id!");
    return 0;
}

bool DummyUnixSandbox::run(const std::string& command, const std::vector<std::string>& args) {
    BoxLocker locker(this, "run_lock");
    if (!locker.has_lock()) return false;
    if (!pre_fork_hook()) return false;
    pid_t box_pid;
    int ret = pipe(comm);
    if (ret == -1) {
        error(4, serror("Error opening pipe to child process"));
        return false;
    }
    if ((box_pid = fork()) != 0) {
        if (box_pid == -1) {
            error(4, serror("fork"));
            return false;
        }
        close(comm[1]);
        bool ret = box_checker(box_pid);
        if (!cleanup_hook()) return false;
        return ret;
    } else {
        close(comm[0]);
        if (!post_fork_hook()) exit(1);
        box_inner(command, args);
    }
    error(253, "If you read this, something went very wrong");
    return false;
}


bool DummyUnixSandbox::delete_box() {
    int err = rm_rf(box_base_path(base_path, id_));
    if (err) error(4, serror("Error deleting sandbox", err));
    return !err;
}

REGISTER_SANDBOX(DummyUnixSandbox);

#endif
