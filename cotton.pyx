import errno as pyerrno
import os
import json
import signal
import resource
import traceback
from timeit import default_timer as timer

cimport cpython
from libc.stdlib cimport malloc, free
from libc.stdio cimport printf
from libc.errno cimport errno
from libc.string cimport strerror, strdup
from libc.signal cimport SIGCHLD
from posix.unistd cimport geteuid, getuid, getpid, getgid, read, write, execve, fork, usleep
from posix.wait cimport waitpid, WNOHANG, WIFSIGNALED, WTERMSIG, WIFEXITED, WEXITSTATUS

cdef extern from "sched.h":
    int clone(int (*f)(void *), void *stack, int flags, void *arg, ...)
    int unshare(int flags)
    enum: CLONE_NEWUSER
    enum: CLONE_NEWNS
    enum: CLONE_NEWUTS
    enum: CLONE_NEWIPC
    enum: CLONE_NEWPID
    enum: CLONE_NEWNET

# Constants
cdef enum: STACK_SIZE = 1<<20
cdef enum: BUF_SIZE = 1<<13
cdef enum: H_SIZE = sizeof(int)

cdef read_exactly(int fd, int size, int timeout):
    cdef char buf[BUF_SIZE]
    cdef int lr
    s = []
    if timeout:
        lastread = timer()
    while size > 0:
        lr = read(fd, buf, min(size, BUF_SIZE))
        if lr == -1:
            if errno not in (pyerrno.EAGAIN, pyerrno.EWOULDBLOCK):
                raise OSError(errno, strerror(errno))
        if lr > 0:
            size -= lr
            s.append(<bytes> buf[:lr])
            if timeout:
                lastread = timer()
        if timeout and timer() - lastread > timeout:
            raise TimeoutError("Read timeout")
    return b''.join(s)

cdef write_all(int fd, int size, unsigned char* s, int timeout):
    cdef char buf[BUF_SIZE]
    cdef int lw
    if timeout:
        lastwrite = timer()
    while size > 0:
        lw = write(fd, s, size)
        if lw == -1:
            if errno not in (pyerrno.EAGAIN, pyerrno.EWOULDBLOCK):
                raise OSError(errno, strerror(errno))
        if lw > 0:
            size -= lw
            s += lw
            if timeout:
                lastread = timer()
        if timeout and timer() - lastread > timeout:
            raise TimeoutError("Write timeout")

cdef read_from_fd(int fd, int timeout = 0):
    cdef int sz;
    cdef unsigned char* c = <unsigned char*> &sz
    data = read_exactly(fd, H_SIZE, timeout)
    for i in xrange(H_SIZE):
        c[i] = data[i]
    data = read_exactly(fd, sz, timeout)
    return json.loads(data.decode())

cdef write_to_fd(int fd, obj, int timeout = 0):
    data = json.dumps(obj).encode()
    cdef int sz = len(data)
    cdef unsigned char* c = <unsigned char*> &sz
    write_all(fd, H_SIZE, c, timeout)
    write_all(fd, sz, data, timeout)

cdef int wrap(void* __self):
    cdef cpython.PyObject *o
    o = <cpython.PyObject *> __self
    cpython.Py_XINCREF(o)
    self = <object> o
    try:
        return Cotton.child_proc(self)
    except:
        traceback.print_exc()
        pass

cdef class Cotton:
    cdef int parent_child_pipe[2]
    cdef int child_parent_pipe[2]
    cdef int pid

    cdef is_child(self):
        return getpid() == 1

    cdef recv(self, timeout=0):
        return read_from_fd(self.parent_child_pipe[0] if self.is_child() else self.child_parent_pipe[0], timeout)

    cdef send(self, obj, timeout=0):
        write_to_fd(self.child_parent_pipe[1] if self.is_child() else self.parent_child_pipe[1], obj, timeout)

    cdef char** list_to_strings(self, lst):
        lst = list(map(lambda x: x.encode(), lst))
        cdef char** ret = <char**> malloc(sizeof(char**) * (len(lst)+1))
        ret[len(lst)] = NULL
        for i in xrange(len(lst)):
            ret[i] = strdup(lst[i])
        return ret

    cdef char** env_to_strings(self, dct):
        return self.list_to_strings(["=".join(i) for i in dct.iteritems()])

    cdef child_proc(self):
#        print(self.recv(10))
        cdef int command_pid
        cdef int return_code
        env = dict()
        res_limits = {
            resource.RLIMIT_STACK: (resource.RLIM_INFINITY, resource.RLIM_INFINITY),
            resource.RLIMIT_MEMLOCK: (0, 0)
        }
        wall_limit = 0

        printf("UID: %d\nEUID: %d\nPID: %d\n", getuid(), geteuid(), getpid())
        while True:
            data = self.recv(10)
            if data["action"] == "run":
                command_pid = fork()
                if not command_pid:
                    for (res, limit) in res_limits.iteritems():
                        resource.setrlimit(res, limit)
                    execve(
                        data["command"][0].encode(),
                        self.list_to_strings(data["command"]),
                        self.env_to_strings(env)
                    )
                    print(strerror(errno))
                else:
                    data = dict()
                    ex_start = timer()
                    if wall_limit > 0: 
                        done = 0
                        while timer() - ex_start < wall_limit:
                            done = waitpid(command_pid, &return_code, WNOHANG)
                            usleep(100)
                            if done:
                                break
                        if not done:
                            os.kill(command_pid, signal.SIGKILL)
                            waitpid(command_pid, &return_code, 0)
                    else:
                        waitpid(command_pid, &return_code, 0)
                    data["ret"] = WEXITSTATUS(return_code) if WIFEXITED(return_code) else 0
                    data["sig"] = WTERMSIG(return_code) if WIFSIGNALED(return_code) else 0
                    data["wall_time"] = timer() - ex_start
                    res = resource.getrusage(resource.RUSAGE_CHILDREN)
                    data["mem"] = res.ru_maxrss
                    data["time"] = res.ru_utime
                    self.send(data)
            elif data["action"] == "set_wall_limit":
                wall_limit = data["limit"]
            elif data["action"] == "setrlimit":
                res_limits[data["res"]] = data["limit"]
            elif data["action"] == "set_env":
                env[data["var"]] = data["val"]
        return 0

    def __init__(self):
        cdef int original_uid = getuid()
        cdef int original_gid = getgid()
        cdef char* stack = <char*> malloc(STACK_SIZE)
        self.parent_child_pipe = os.pipe()
        self.child_parent_pipe = os.pipe()

        self.pid = clone(
            wrap,
            stack + STACK_SIZE,
            CLONE_NEWPID | CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWIPC | CLONE_NEWNS | SIGCHLD,
            <void*> self
        )
#        with open("/proc/%d/uid_map" % pid, "w") as f:
#            f.write("0 %s 1\n" % original_uid)
#        try:
#            with open("/proc/%d/setgroups" % pid, "w") as f:
#                f.write("deny")
#        except OSError as e:
#            if e.errno != pyerrno.ENOENT:
#                raise
#        with open("/proc/%d/gid_map" % pid, "w") as f:
#            f.write("0 %s 1\n" % original_gid)
#        self.send(['a', 'b', 'c', 15, 1.5])

    def rlimit(self, res, limit):
        self.send({
            "action": "setrlimit",
            "res": res,
            "limit": limit
        })

    def wall_limit(self, limit):
        self.send({
            "action": "set_wall_limit",
            "limit": limit
        })

    def set_env(self, var, val):
        self.send({
            "action": "set_env",
            "var": var,
            "val": val
        })

    def memory_limit(self, limit):
        self.rlimit(resource.RLIMIT_AS, (limit, limit))

    def time_limit(self, limit):
        self.rlimit(resource.RLIMIT_CPU, (limit, limit))

    def process_limit(self, limit):
        self.rlimit(resource.RLIMIT_NPROC, (limit+1, limit+1))

    def run(self, command):
        if isinstance(command, basestring) or (not all(isinstance(itm, basestring) for itm in command)):
            raise ValueError("command must be a list of strings!")
        self.send({
            "action": "run",
            "command": command
        })
        return self.recv()
