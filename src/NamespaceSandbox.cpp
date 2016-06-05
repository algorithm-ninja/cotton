#include "util.hpp"
#ifdef COTTON_LINUX
#include "NamespaceSandbox.hpp"
#include <sched.h>
#include <sys/mount.h>
#include <unistd.h>

bool NamespaceSandbox::is_available() const {
    return getuid() == 0; // It works only if the sandbox is setuid
}

std::string NamespaceSandbox::err_string(int error_id) const {
    if (error_id == 100) return "Error creating the new namespace";
    if (error_id == 101) return "Error setting up mountpoints";
    if (error_id == 102) return "Error changing the root";
    if (error_id == 103) return "Error cleaning up mountpoints";
    return DummyUnixSandbox::err_string(error_id);
}


bool NamespaceSandbox::pre_fork_hook() {
    Privileged p;
    // Change PID namespace for the child process
    if (unshare(CLONE_NEWPID) == -1) {
        error(4, serror(err_string(100)));
        return false;
    }
    return true;
}

bool NamespaceSandbox::post_fork_hook() {
    Privileged p;
    // Change namespace
    if (unshare(CLONE_NEWNET | CLONE_NEWIPC | CLONE_NEWNS) == -1) {
        send_error(100, errno);
        return false;
    }
    return true;
}

bool NamespaceSandbox::cleanup_hook() {
    if (mountpoints.empty()) return true;
    Privileged p;
    for (const auto& mnt: mountpoints) {
        std::string target = get_root() + mnt.first;
        if (::umount(target.c_str()) == -1) {
            send_error(103, errno);
            return false;
        }
    }
    return true;
}

bool NamespaceSandbox::pre_exec_hook() {
    if (mountpoints.empty()) return true;
    Privileged p;
    for (const auto& mnt: mountpoints) {
        std::string source = mnt.second.first;
        std::string target = get_root() + mnt.first;
        unsigned long flags = MS_BIND | MS_NODEV | MS_NOSUID;
        if (!mnt.second.second) flags |= MS_RDONLY;
        if (::mount(source.c_str(), target.c_str(), NULL, flags, NULL) == -1) {
            send_error(101, errno);
            return false;
        }
    }
    if (chroot(".") == -1) {
        send_error(102, errno);
        return false;
    }
    return true;
}

std::string NamespaceSandbox::mount(const std::string& box_path) const {
    if (mountpoints.count(box_path)) return mountpoints.at(box_path).first;
    else return "";
}

bool NamespaceSandbox::mount(const std::string& box_path, const std::string& orig_path, bool rw) {
    if (mkdirs(get_root() + box_path, box_mode) == -1) {
        error(4, serror("mkdirs"));
        return false;
    }
    mountpoints[box_path] = {orig_path, rw};
    return true;
}

bool NamespaceSandbox::umount(const std::string& box_path) {
    return mountpoints.erase(box_path);
}

REGISTER_SANDBOX(NamespaceSandbox);
#endif
