#ifndef COTTON_BOX_HPP
#define COTTON_BOX_HPP
#include <string>
#include <map>
#include <functional>
#include <cstdint>
#include "logger.hpp"

#include <boost/serialization/export.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#define REGISTER_SANDBOX(sbx) __attribute__((constructor)) static void register_sandbox_ ## sbx() { \
    box_creators[#sbx] = &create_sandbox<sbx>; \
} \
BOOST_CLASS_EXPORT(sbx)


// Times should be microseconds.
// Space usages should be kilobytes 
class SandBox {
public:
    typedef std::function<void(int, const std::string& str)> callback_t;
protected:
    const callback_t* on_error;
    const callback_t* on_warning;
    const std::string& base_path;
    size_t id_;
    void error(int code, const std::string& err) {(*on_error)(code, err);}
    void warning(int code, const std::string& err) {(*on_warning)(code, err);}
public:
    typedef uint64_t feature_mask_t;
    static const feature_mask_t memory_limit         = 0x00000001;
    static const feature_mask_t time_limit           = 0x00000002;
    static const feature_mask_t wall_time_limit      = 0x00000004;
    static const feature_mask_t process_limit        = 0x00000008; // Limits process creation
    static const feature_mask_t process_limit_full   = 0x00000010; // Limits the number of processes
    static const feature_mask_t disk_limit           = 0x00000020; // Works for non-malicious errors
    static const feature_mask_t disk_limit_full      = 0x00000040; // Works for malicious attempts
    static const feature_mask_t folder_mount         = 0x00000080;
    static const feature_mask_t memory_usage         = 0x00000100;
    static const feature_mask_t running_time         = 0x00000200;
    static const feature_mask_t wall_time            = 0x00000400;
    static const feature_mask_t clearable            = 0x00000800;
    static const feature_mask_t process_isolation    = 0x00001000; // Isolation from other processes
    static const feature_mask_t io_redirection       = 0x00002000;
    friend class boost::serialization::access;

    void set_error_handler(const callback_t& cb) {on_error = &cb;}
    void set_warning_handler(const callback_t& cb) {on_warning = &cb;}
    size_t get_id() const {return id_;}
    static std::string box_base_path(const std::string& bp, size_t id) {
#ifdef __unix__
        return bp + "/box_" + std::to_string(id) + "/";
#else
        return "pippo";
#endif
    }

    SandBox(const std::string& base_path): base_path(base_path) {}
    virtual bool is_available() = 0;
    virtual int get_penality() = 0;
    virtual feature_mask_t get_features() = 0;
    virtual size_t create_box() = 0;
    virtual std::string get_root() = 0;
    virtual bool check() {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool set_memory_limit(size_t limit) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool set_time_limit(size_t limit) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool set_wall_time_limit(size_t limit) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool set_process_limit(size_t limit) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool set_disk_limit(size_t limit) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual size_t get_memory_limit() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual size_t get_time_limit() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual size_t get_wall_time_limit() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual size_t get_process_limit() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual size_t get_disk_limit() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual std::vector<std::pair<std::string, std::string>> mount() {
        error(254, "This method is not implemented by this sandbox!");
        return {};
    }
    virtual std::string mount(const std::string& box_path) {
        error(254, "This method is not implemented by this sandbox!");
        return "";
    }
    virtual bool mount(const std::string& box_path, const std::string& orig_path, bool rw = false) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool umount(const std::string& box_path) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool redirect_stdin(const std::string& stdin_file) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool redirect_stdout(const std::string& stdout_file) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool redirect_stderr(const std::string& stderr_file) {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual std::string get_stdin() {
        error(254, "This method is not implemented by this sandbox!");
        return "";
    }
    virtual std::string get_stdout() {
        error(254, "This method is not implemented by this sandbox!");
        return "";
    }
    virtual std::string get_stderr() {
        error(254, "This method is not implemented by this sandbox!");
        return "";
    }
    virtual bool run(const std::string& command, const std::vector<std::string>& args) = 0;
    virtual size_t get_memory_usage() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual size_t get_running_time() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual size_t get_wall_time() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual size_t get_return_code() {
        error(254, "This method is not implemented by this sandbox!");
        return 0;
    }
    virtual bool clear() {
        error(254, "This method is not implemented by this sandbox!");
        return false;
    }
    virtual bool delete_box() = 0;
    template <typename Archive> void serialize(Archive &ar, const unsigned int version) {
        ar & id_;
    };
    virtual ~SandBox() = 0;
};

typedef std::map<std::string, std::function<SandBox*(const std::string&)>> BoxCreators;

extern BoxCreators box_creators;
template<typename T> SandBox* create_sandbox(const std::string& base_path) {return new T(base_path);}

#endif
