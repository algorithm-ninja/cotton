#include "util.hpp"
#ifdef COTTON_LINUX
#include "NamespaceSandbox.hpp"
#include <sched.h>

bool NamespaceSandbox::is_available() const {
    return getuid() == 0; // It works only if the sandbox is setuid
}

std::string NamespaceSandbox::err_string(int error_id) const {
    if (error_id == 100) return "Error creating the new namespace";
    return DummyUnixSandbox::err_string(error_id);
}

bool NamespaceSandbox::run(const std::string& command, const std::vector<std::string>& args) {
    BoxLocker locker(this, "run_lock");
    if (!locker.has_lock()) return false;
    // Become root again
    setreuid(geteuid(), getuid());
    // Change PID namespace for the child process
    if (unshare(CLONE_NEWPID) == -1) {
        error(4, serror(err_string(100)));
        return false;
    }
    // Drop privileges
    setreuid(geteuid(), getuid());
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
        // Become root again
        setreuid(geteuid(), getuid());
        // Change namespace
        if (unshare(CLONE_NEWNET | CLONE_NEWIPC) == -1) {
            send_error(100, errno);
            exit(1);
        }
        // Drop privileges
        setreuid(geteuid(), getuid());
        box_inner(command, args);
    }
    error(253, "If you read this, something went very wrong");
    return false;
}


REGISTER_SANDBOX(NamespaceSandbox);
#endif
