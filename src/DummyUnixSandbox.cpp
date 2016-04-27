#ifdef __unix__
#include "box.hpp"
#include "util.hpp"
#include <limits>
#include <chrono>
#include <thread>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>

class DummyUnixSandbox: public Sandbox {
protected:
    // Persistent data
    size_t mem_limit;
    size_t time_limit;
    size_t wall_time_limit;
    size_t process_limit;
    size_t disk_limit;
    std::string stdin_;
    std::string stdout_;
    std::string stderr_;
    size_t memory_usage;
    size_t running_time;
    size_t wall_time;
    size_t return_code;
    size_t signal;

    // Transient data
    int comm[2];

    static const mode_t box_mode = S_IRWXU | S_IRGRP | S_IROTH;
    virtual size_t create_box(size_t box_limit) {
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
            int res = open(current_attempt.c_str(), O_RDWR | O_CREAT | O_EXCL);
            if (res == -1 && errno == EEXIST) continue;
            if (res == -1) {
                warning(4, serror("Something weird happened creating sandbox " + current_attempt));
                continue;
            }
            current_attempt = box_base_path(base_path, box_id) + "file_root/";
            int err = rm_rf(current_attempt);
            if (err) {
                error(4, serror("Error deleting old file_root for " + current_attempt, err));
                return 0;
            }
            if (mkdir(current_attempt.c_str(), box_mode) == -1) {
                error(4, serror("Error creating file_root for " + current_attempt));
                return 0;
            }
            id_ = box_id;
            return id_;
        }
        error(4, "Could not find a free box id!");
        return 0;
    }

    // A negative error_id indicates a warning
    bool send_error(int error_id, int err) {
        int tmp[2] = {error_id, err};
        return 2*sizeof(int) == write(comm[1], (void*)tmp, 2*sizeof(int));
    }

    // Returns -1 if there was an error, 0 if the pipe is closed and 1 otherwise.
    int get_error(int& error_id, int& err) {
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

    std::string err_string(int error_id) {
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
            default: return "Unknown error";
        }
    }

    [[noreturn]] virtual void box_inner(const std::string& command, const std::vector<std::string>& args) {
        // Set up IO redirection.
        if (stdin_ != "") {
            int stdin_fd = open((box_base_path(base_path, id_) + stdin_).c_str(), O_RDONLY);
            if (stdin_fd == -1) {
                send_error(1, errno);
                exit(1);
            }
            dup2(stdin_fd, fileno(stdin));
        }
        if (stdout_ != "") {
            int stdout_fd = open((box_base_path(base_path, id_) + stdout_).c_str(), O_RDWR);
            if (stdout_fd == -1) {
                send_error(2, errno);
                exit(1);
            }
            dup2(stdout_fd, fileno(stdout));
        }
        if (stderr_ != "") {
            int stderr_fd = open((box_base_path(base_path, id_) + stderr_).c_str(), O_RDWR);
            if (stderr_fd == -1) {
                send_error(3, errno);
                exit(1);
            }
            dup2(stderr_fd, fileno(stderr));
        }
        // Make the pipe close on the call to exec()
        fcntl(comm[1], F_SETFD, FD_CLOEXEC);
        // Build arguments for execve
        std::string executable = get_root() + command;
        std::vector<char*> e_args;
        e_args.push_back(const_cast<char*>(executable.c_str()));
        for (auto arg: args) e_args.push_back(const_cast<char*>(arg.c_str()));
        e_args.push_back(nullptr);
        // Set up limits
        struct rlimit rlim;
        rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_STACK, &rlim) == -1) send_error(-1, errno);
        if (mem_limit) {
            rlim.rlim_cur = rlim.rlim_max = mem_limit*1024;
            if (setrlimit(RLIMIT_AS, &rlim) == -1) send_error(-2, errno);
        }
        if (time_limit) {
            rlim.rlim_cur = rlim.rlim_max = (time_limit-1)/1000000+1;
            if (setrlimit(RLIMIT_CPU, &rlim) == -1) send_error(-3, errno);
        }
        if (process_limit) {
            rlim.rlim_cur = rlim.rlim_max = process_limit;
            if (setrlimit(RLIMIT_NPROC, &rlim) == -1) send_error(-4, errno);
        }
        if (disk_limit) {
            rlim.rlim_cur = rlim.rlim_max = disk_limit*1024;
            if (setrlimit(RLIMIT_FSIZE, &rlim) == -1) send_error(-5, errno);
            rlim.rlim_cur = rlim.rlim_max = 0;
            if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) send_error(-5, errno);
        }
        execv(executable.c_str(), &e_args[0]);
        send_error(4, errno);
        exit(1);
    }

    virtual bool box_checker(pid_t box_pid) {
        int error_id = 0;
        int err_msg = 0;
        int err_ret;
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
        if (wall_time_limit > 0) {
            auto now = std::chrono::high_resolution_clock::now();
            size_t micros = std::chrono::duration_cast<std::chrono::microseconds>(now-start).count();
            bool waited = false;
            while (micros < wall_time_limit) {
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
        wall_time = std::chrono::duration_cast<std::chrono::microseconds>(now-start).count();
        struct rusage stats;
        getrusage(RUSAGE_CHILDREN, &stats);
        memory_usage = stats.ru_maxrss;
        running_time = (stats.ru_utime.tv_sec+stats.ru_stime.tv_sec)*1000000ULL + (stats.ru_utime.tv_usec+stats.ru_stime.tv_usec);
        return true;
    }
public:
    using Sandbox::Sandbox;
    virtual bool is_available() {
        return true; // If it compiles, it should work.
    }
    virtual int get_penality() {
        return 0; // No noticeable performance hits
    }
    virtual feature_mask_t get_features() {
        return Sandbox::memory_limit | Sandbox::time_limit | Sandbox::wall_time_limit |
            Sandbox::process_limit | Sandbox::disk_limit | Sandbox::memory_usage |
            Sandbox::running_time | Sandbox::wall_time | Sandbox::io_redirection |
            Sandbox::return_code | Sandbox::signal;
    }
    virtual size_t create_box() {
        return create_box(std::numeric_limits<int>::max());
    }
    virtual std::string get_root() {
        return box_base_path(base_path, id_) + "file_root/";
    }
    virtual bool set_memory_limit(size_t limit) {
        mem_limit = limit;
        return true;
    }
    virtual bool set_time_limit(size_t limit) {
        time_limit = limit;
        return true;
    }
    virtual bool set_wall_time_limit(size_t limit) {
        wall_time_limit = limit;
        return true;
    }
    virtual bool set_process_limit(size_t limit) {
        if (limit > 1) warning(4, "This sandbox has partial support for process limits!");
        process_limit = limit ? 1 : 0;
        return true;
    }
    virtual bool set_disk_limit(size_t limit) {
        disk_limit = limit;
        return true;
    }
    virtual size_t get_memory_limit() {
        return memory_limit;
    }
    virtual size_t get_time_limit() {
        return time_limit;
    }
    virtual size_t get_wall_time_limit() {
        return wall_time_limit;
    }
    virtual size_t get_process_limit() {
        return process_limit;
    }
    virtual size_t get_disk_limit() {
        return disk_limit;
    }
    virtual bool redirect_stdin(const std::string& stdin_file) {
        if (stdin_file == "") return true;
        int fd = open((box_base_path(base_path, id_) + stdin_file).c_str(), O_RDONLY);
        if (fd == -1) error(4, serror("Cannot open stdin file"));
        else {
            close(fd);
            stdin_ = stdin_file;
        }
        return fd != -1;
    }
    virtual bool redirect_stdout(const std::string& stdout_file) {
        if (stdout_file == "") return true;
        int fd = open((box_base_path(base_path, id_) + stdout_file).c_str(), O_RDWR | O_CREAT | O_TRUNC);
        if (fd == -1) error(4, serror("Cannot create stdout file"));
        else {
            close(fd);
            stdout_ = stdout_file;
        }
        return fd != -1;
    }
    virtual bool redirect_stderr(const std::string& stderr_file) {
        if (stderr_file == "") return true;
        int fd = open((box_base_path(base_path, id_) + stderr_file).c_str(), O_RDWR | O_CREAT | O_TRUNC);
        if (fd == -1) error(4, serror("Cannot create stderr file"));
        else {
            close(fd);
            stderr_ = stderr_file;
        }
        return fd != -1;
    }
    virtual std::string get_stdin() {
        return stdin_;
    }
    virtual std::string get_stdout() {
        return stdout_;
    }
    virtual std::string get_stderr() {
        return stderr_;
    }
    virtual bool run(const std::string& command, const std::vector<std::string>& args) {
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
            return box_checker(box_pid);
        } else {
            close(comm[0]);
            box_inner(command, args);
        }
        error(253, "If you read this, something went very wrong");
        return false;
    }
    virtual size_t get_memory_usage() {
        return memory_usage;
    }
    virtual size_t get_running_time() {
        return running_time;
    }
    virtual size_t get_wall_time() {
        return wall_time;
    }
    virtual size_t get_return_code() {
        return return_code;
    }
    virtual size_t get_signal() {
        return signal;
    }
    virtual bool delete_box() {
        int err = rm_rf(box_base_path(base_path, id_));
        if (err) error(4, serror("Error deleting sandbox", err));
        return !err;
    }
    template <typename Archive> void serialize(Archive &ar, const unsigned int version) {
        ar & boost::serialization::base_object<Sandbox>(*this);
        ar & mem_limit;
        ar & time_limit;
        ar & wall_time_limit;
        ar & process_limit;
        ar & disk_limit;
        ar & stdin_;
        ar & stdout_;
        ar & stderr_;
        ar & memory_usage;
        ar & running_time;
        ar & wall_time;
        ar & return_code;
        ar & signal;
    };
    virtual ~DummyUnixSandbox() = default;
    //virtual bool check();
    //virtual std::vector<std::pair<std::string, std::string>> mount()
    //virtual std::string mount(const std::string& box_path)
    //virtual bool mount(const std::string& box_path, const std::string& orig_path, bool rw = false)
    //virtual bool umount(const std::string& box_path)
    //virtual bool clear()
};

REGISTER_SANDBOX(DummyUnixSandbox);

#endif
