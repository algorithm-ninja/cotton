#ifndef NAMESPACE_SANDBOX_HPP
#define NAMESPACE_SANDBOX_HPP
#if defined(__unix__) || defined(__APPLE__)
#include "DummyUnixSandbox.hpp"

class NamespaceSandbox: public DummyUnixSandbox {
public:
    using DummyUnixSandbox::DummyUnixSandbox;
    virtual feature_mask_t get_features() const override {
        return DummyUnixSandbox::get_features() | Sandbox::process_isolation |
            Sandbox::network_isolation;
    }
    virtual bool is_available() const override;
    virtual std::string err_string(int error_id) const override;
    virtual bool run(const std::string& command, const std::vector<std::string>& args) override;
    template <typename Archive> void serialize(Archive &ar, const unsigned int version) {
        ar & boost::serialization::base_object<DummyUnixSandbox>(*this);
    }
};

#endif
#endif
