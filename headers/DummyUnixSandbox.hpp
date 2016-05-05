#ifndef DUMMY_UNIX_SANDBOX_HPP
#define DUMMY_UNIX_SANDBOX_HPP
#ifdef __unix__
#include "box.hpp"
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
    virtual std::string err_string(int error_id);

    bool prepare_io_redirect(const std::string& file, std::string& redir, mode_t mode);
    bool setup_io_redirect(const std::string& file, int dest_fd, mode_t mode);

    [[noreturn]] virtual void box_inner(const std::string& command, const std::vector<std::string>& args);
    virtual bool box_checker(pid_t box_pid);
public:
    using Sandbox::Sandbox;
    virtual bool is_available() const override {
        return true; // If it compiles, it should work.
    }
    virtual int get_penality() const override {
        return 0; // No noticeable performance hits
    }
    virtual feature_mask_t get_features() const override {
        return Sandbox::memory_limit | Sandbox::time_limit | Sandbox::wall_time_limit |
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
    virtual bool set_memory_limit(size_t limit) override {
        mem_limit = limit;
        return true;
    }
    virtual bool set_time_limit(size_t limit) override {
        time_limit = limit;
        return true;
    }
    virtual bool set_wall_time_limit(size_t limit) override {
        wall_time_limit = limit;
        return true;
    }
    virtual bool set_process_limit(size_t limit) override {
        if (limit > 1) warning(4, "This sandbox has partial support for process limits!");
        process_limit = limit ? 1 : 0;
        return true;
    }
    virtual bool set_disk_limit(size_t limit) override {
        disk_limit = limit;
        return true;
    }
    virtual size_t get_memory_limit() const override {
        return memory_limit;
    }
    virtual size_t get_time_limit() const override {
        return time_limit;
    }
    virtual size_t get_wall_time_limit() const override {
        return wall_time_limit;
    }
    virtual size_t get_process_limit() const override {
        return process_limit;
    }
    virtual size_t get_disk_limit() const override {
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
    virtual size_t get_memory_usage() const override {
        return memory_usage;
    }
    virtual size_t get_running_time() const override {
        return running_time;
    }
    virtual size_t get_wall_time() const override {
        return wall_time;
    }
    virtual size_t get_return_code() const override {
        return return_code;
    }
    virtual size_t get_signal() const override {
        return signal;
    }
    virtual bool delete_box() override;
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

#endif
#endif
