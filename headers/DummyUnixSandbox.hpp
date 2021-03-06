#ifndef DUMMY_UNIX_SANDBOX_HPP
#define DUMMY_UNIX_SANDBOX_HPP
#ifdef COTTON_UNIX
#include "box.hpp"
#include "util.hpp"
#include <fcntl.h>
#include <sys/stat.h>

class DummyUnixSandbox: public Sandbox {
protected:
    // Persistent data
    space_limit_t mem_limit = 0;
    time_limit_t time_limit = 0;
    time_limit_t wall_time_limit = 0;
    size_t process_limit = 0;
    space_limit_t disk_limit = 0;
    std::string stdin_;
    std::string stdout_;
    std::string stderr_;
    space_limit_t memory_usage = 0;
    time_limit_t running_time = 0;
    time_limit_t wall_time = 0;
    std::string exit_status;
    size_t return_code = 0;
    size_t signal = 0;

    // Transient data
    int comm[2] = {0, 0};

    static const mode_t box_mode = S_IRWXU | S_IRGRP | S_IROTH;
    static const mode_t file_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    class BoxLocker {
        std::string lock_name;
        const DummyUnixSandbox* box;
        bool has_lock_;
    public:
        BoxLocker(const DummyUnixSandbox* box, const std::string& lock);
        bool has_lock() {return has_lock_;}
        ~BoxLocker();
    };

    virtual size_t create_box(size_t box_limit);

    // A negative error_id indicates a warning
    bool send_error(int error_id, int err);
    // Returns -1 if there was an error, 0 if the pipe is closed and 1 otherwise.
    int get_error(int& error_id, int& err);
    virtual std::string err_string(int error_id) const;

    bool prepare_io_redirect(const std::string& file, std::string& redir, mode_t mode);
    bool setup_io_redirect(const std::string& file, int dest_fd, mode_t mode);

    [[noreturn]] virtual void box_inner(const std::string& command, const std::vector<std::string>& args);
    virtual bool box_checker(pid_t box_pid);
    virtual bool pre_fork_hook() {return true;}
    virtual bool post_fork_hook() {return true;}
    virtual bool pre_exec_hook() {return true;}
    virtual bool cleanup_hook() {return true;}
    DummyUnixSandbox() {}
public:
    DummyUnixSandbox(const std::string& base_path): Sandbox(base_path) {}
    virtual bool is_available() const override {
        return true; // If it compiles, it should work.
    }
    virtual int get_overhead() const override {
        return 0; // No noticeable performance hits
    }
    virtual feature_mask_t get_features() const override {
        return Sandbox::memory_limit | Sandbox::cpu_limit | Sandbox::wall_time_limit |
            Sandbox::process_limit | Sandbox::disk_limit | Sandbox::memory_usage |
            Sandbox::running_time | Sandbox::wall_time | Sandbox::io_redirection |
            Sandbox::return_code | Sandbox::signal;
    }
    virtual size_t create_box() override {
        return create_box(std::numeric_limits<int>::max());
    }
    virtual std::string get_root() const override {
        return box_base_path(base_path, id_) + "file_root/";
    }
    virtual bool set_memory_limit(space_limit_t limit) override {
        mem_limit = limit;
        return true;
    }
    virtual bool set_time_limit(time_limit_t limit) override {
        time_limit = limit;
        return true;
    }
    virtual bool set_wall_time_limit(time_limit_t limit) override {
        wall_time_limit = limit;
        return true;
    }
    virtual bool set_process_limit(size_t limit) override {
        if (limit > 1) warning(4, "This sandbox has partial support for process limits!");
        process_limit = limit ? 1 : 0;
        return true;
    }
    virtual bool set_disk_limit(space_limit_t limit) override {
        disk_limit = limit;
        return true;
    }
    virtual space_limit_t get_memory_limit() const override {
        return mem_limit;
    }
    virtual time_limit_t get_time_limit() const override {
        return time_limit;
    }
    virtual time_limit_t get_wall_time_limit() const override {
        return wall_time_limit;
    }
    virtual size_t get_process_limit() const override {
        return process_limit;
    }
    virtual space_limit_t get_disk_limit() const override {
        return disk_limit;
    }
    virtual bool redirect_stdin(const std::string& stdin_file) override {
        return prepare_io_redirect(stdin_file, stdin_, O_RDONLY);
    }
    virtual bool redirect_stdout(const std::string& stdout_file) override {
        return prepare_io_redirect(stdout_file, stdout_, O_RDWR | O_CREAT | O_TRUNC);
    }
    virtual bool redirect_stderr(const std::string& stderr_file) override {
        return prepare_io_redirect(stderr_file, stderr_, O_RDWR | O_CREAT | O_TRUNC);
    }
    virtual std::string get_stdin() const override {
        return stdin_;
    }
    virtual std::string get_stdout() const override {
        return stdout_;
    }
    virtual std::string get_stderr() const override {
        return stderr_;
    }
    virtual bool run(const std::string& command, const std::vector<std::string>& args) override;
    virtual space_limit_t get_memory_usage() const override {
        return memory_usage;
    }
    virtual time_limit_t get_running_time() const override {
        return running_time;
    }
    virtual time_limit_t get_wall_time() const override {
        return wall_time;
    }
    virtual int get_return_code() const override {
        return return_code;
    }
    virtual int get_signal() const override {
        return signal;
    }
    virtual std::string get_status() const override {
        return exit_status;
    }
    virtual bool delete_box() override;
    friend class boost::serialization::access;
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
        ar & exit_status;
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

#endif
#endif
