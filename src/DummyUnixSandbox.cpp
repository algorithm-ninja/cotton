#ifdef __unix__
#include "box.hpp"
#include "util.hpp"
#include <limits>

class DummyUnixSandbox: public Sandbox {
protected:
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

    static const mode_t box_mode = S_IRWXU | S_IRGRP | S_IROTH; 
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
            Sandbox::running_time | Sandbox::wall_time | Sandbox::io_redirection;
    }
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
            id_ = box_id;
            return id_;
        }
        error(4, "Could not find a free box id!");
        return 0;
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
        stdin_ = stdin_file;
        int fd = open((box_base_path(base_path, id_) + stdin_).c_str(), O_RDONLY);
        if (fd == -1) error(4, serror("Cannot open stdin file"));
        else close(fd);
        return fd != -1;
    }
    virtual bool redirect_stdout(const std::string& stdout_file) {
        stdout_ = stdout_file;
        int fd = open((box_base_path(base_path, id_) + stdout_).c_str(), O_RDWR | O_CREAT | O_TRUNC);
        if (fd == -1) error(4, serror("Cannot create stdout file"));
        else close(fd);
        return fd != -1;
    }
    virtual bool redirect_stderr(const std::string& stderr_file) {
        stderr_ = stderr_file;
        int fd = open((box_base_path(base_path, id_) + stderr_).c_str(), O_RDWR | O_CREAT | O_TRUNC);
        if (fd == -1) warning(4, serror("Cannot create stderr file"));
        else close(fd);
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
