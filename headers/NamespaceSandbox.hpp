#ifndef NAMESPACE_SANDBOX_HPP
#define NAMESPACE_SANDBOX_HPP
#include "util.hpp"
#ifdef COTTON_LINUX
#include "DummyUnixSandbox.hpp"
#include <boost/serialization/map.hpp>

class NamespaceSandbox: public DummyUnixSandbox {
    std::map<std::string, std::pair<std::string, bool>> mountpoints;
    virtual bool pre_fork_hook();
    virtual bool post_fork_hook();
    virtual bool pre_exec_hook();
    virtual bool cleanup_hook();
public:
    using DummyUnixSandbox::DummyUnixSandbox;
    virtual feature_mask_t get_features() const override {
        return DummyUnixSandbox::get_features() | Sandbox::process_isolation |
            Sandbox::network_isolation | Sandbox::folder_mount;
    }
    virtual bool is_available() const override;
    virtual std::string err_string(int error_id) const override;
    template <typename Archive> void serialize(Archive &ar, const unsigned int version) {
        ar & boost::serialization::base_object<DummyUnixSandbox>(*this);
        ar & mountpoints;
    }
    virtual std::string mount(const std::string& box_path) const override;
    virtual bool mount(const std::string& box_path, const std::string& orig_path, bool rw = false) override;
    virtual bool umount(const std::string& box_path) override;
};

#endif
#endif
