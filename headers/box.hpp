#ifndef COTTON_BOX_HPP
#define COTTON_BOX_HPP
#include <string>
#include <map>
#include <functional>
#include <cstdint>
#include <stdexcept>
#include <boost/serialization/export.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#define REGISTER_SANDBOX(sbx) __attribute__((constructor)) static void register_sandbox_ ## sbx() { \
    box_creators[#sbx] = &create_sandbox<sbx>; \
} \
BOOST_CLASS_EXPORT(sbx)


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
static const feature_mask_t clear                = 0x00000800;
static const feature_mask_t process_isolation    = 0x00001000; // Isolation from other processes

// Times should be microseconds.
// Space usages should be kilobytes 
class SandBox {
public:
    friend class boost::serialization::access;
    SandBox(const std::string& base_path) {}
    virtual bool is_available() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual int get_penality() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual feature_mask_t get_features() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t create_box() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool check() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool set_memory_limit(size_t limit) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool set_time_limit(size_t limit) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool set_wall_time_limit(size_t limit) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool set_process_limit(size_t limit) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool set_disk_limit(size_t limit) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_memory_limit() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_time_limit() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_wall_time_limit() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_process_limit() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_disk_limit() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual std::vector<std::pair<std::string, std::string>> mount() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual std::string mount(const std::string& box_path) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool mount(const std::string& box_path, const std::string& orig_path, bool rw = false) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool umount(const std::string& box_path) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool run(const std::string& command, const std::vector<std::string>& args) {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_memory_usage() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_running_time() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual size_t get_wall_time() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool clear() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    virtual bool remove() {
        throw std::invalid_argument("This method is not implemented by this sandbox!");
    }
    template <typename Archive> void serialize(Archive &ar, const unsigned int version) {};
    virtual ~SandBox() = 0;
};

typedef std::map<std::string, std::function<SandBox*(const std::string& base_path)>> BoxCreators;

extern BoxCreators box_creators;
template<typename T> SandBox* create_sandbox(const std::string& base_path) {return new T(base_path);}

#endif
