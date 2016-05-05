#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "NamespaceSandbox.hpp"
#include <sched.h>

bool NamespaceSandbox::is_available() const {
    return getuid() == 0; // It works only if the sandbox is setuid
}

std::string NamespaceSandbox::err_string(int error_id) const {
    if (error_id == 100) return "Error creating the new namespace!";
    return DummyUnixSandbox::err_string(error_id);
}

[[noreturn]] void NamespaceSandbox::box_inner(const std::string& command, const std::vector<std::string>& args) {
    // Become root again
    setreuid(geteuid(), getuid());
    // Change namespace
    if (unshare(CLONE_NEWNET | CLONE_NEWPID | CLONE_NEWIPC) == -1) {
        send_error(100, errno);
        exit(1);
    }
    // Drop privileges
    setreuid(geteuid(), getuid());
    DummyUnixSandbox::box_inner(command, args);
}


REGISTER_SANDBOX(NamespaceSandbox);
#endif
