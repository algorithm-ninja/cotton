import errno as pyerrno
import os
import signal
import stat
import pickle
import resource
import tempfile
import traceback
from timeit import default_timer as timer

cimport cpython
from libc.stdlib cimport malloc, free
from libc.stdio cimport printf
from libc.errno cimport errno
from libc.string cimport strerror, strdup
from libc.signal cimport SIGCHLD
from posix.unistd cimport geteuid, getuid, getpid, getgid, read, write, execve, fork, usleep, chdir, dup2
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

cdef extern from "sys/mount.h":
    int mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data)
    int umount(const char *target)
    enum: MS_BIND
    enum: MS_RDONLY

cdef extern from "unistd.h":
    int chroot(const char *path)

# Constants
cdef enum: STACK_SIZE = 1<<20
cdef enum: CHUNK_SIZE = 1<<13
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
    return pickle.loads(data)

cdef write_to_fd(int fd, object obj, int timeout = 0):
    data = pickle.dumps(obj)
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
    cdef int size
    cdef int go_on
    cdef int _wall_limit
    cdef object mounts
    cdef object td
    cdef object env
    cdef object res_limits
    cdef object stdin_file
    cdef object stdout_file
    cdef object stderr_file

    cdef is_child(self):
        return getpid() == 1

    cdef recv(self, timeout=0):
        return read_from_fd(self.parent_child_pipe[0] if self.is_child() else self.child_parent_pipe[0], timeout)

    cdef send(self, obj, timeout=0):
        if self.is_child():
            write_to_fd(self.child_parent_pipe[1], obj, timeout)
        else:
            write_to_fd(self.parent_child_pipe[1], obj, timeout)
            data = self.recv(timeout)
            if isinstance(data, Exception):
                raise data
            return data

    cdef char** list_to_strings(self, lst):
        lst = list(map(lambda x: x.encode(), lst))
        cdef char** ret = <char**> malloc(sizeof(char**) * (len(lst)+1))
        ret[len(lst)] = NULL
        for i in xrange(len(lst)):
            ret[i] = strdup(lst[i])
        return ret

    cdef char** env_to_strings(self):
        return self.list_to_strings(["=".join(i) for i in self.env.iteritems()])

    cdef run_handler(self, data):
        get_stdout = data["get_stdout"]
        get_stderr = data["get_stderr"]
        cdef int command_pid
        cdef int return_code
        command_pid = fork()
        if not command_pid:
            for (res, limit) in self.res_limits.iteritems():
                resource.setrlimit(res, limit)
            chdir(self.td.encode())
            chroot(self.td.encode())
            newin = os.open(self.stdin_file, os.O_RDONLY | os.O_CREAT)
            dup2(newin, 0)
            os.close(newin)
            newout = os.open(self.stdout_file, os.O_WRONLY | os.O_CREAT | os.O_TRUNC)
            dup2(newout, 1)
            os.close(newout)
            newerr = os.open(self.stderr_file, os.O_WRONLY | os.O_CREAT | os.O_TRUNC)
            dup2(newin, 2)
            os.close(newerr)
            execve(
                data["command"][0].encode(),
                self.list_to_strings(data["command"]),
                self.env_to_strings()
            )
            print(strerror(errno))
        else:
            data = dict()
            ex_start = timer()
            if self._wall_limit > 0:
                done = 0
                while timer() - ex_start < self._wall_limit:
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
            if get_stdout:
                try:
                    with open(os.path.join(self.td, self.stdout_file), "rb") as out:
                        data["stdout"] = out.read()
                except:
                    data["stdout"] = ""
            if get_stderr:
                try:
                    with open(os.path.join(self.td, self.stderr_file), "rb") as err:
                        data["stderr"] = err.read()
                except:
                    data["stderr"] = ""
            return data


    cdef cleanup_handler(self, data):
        for m in self.mounts[::-1]:
            umount(m.encode())
        umount(self.td.encode())
        os.rmdir(self.td)
        self.go_on = 0

    cdef do_work(self):
        try:
            data = self.recv(10)
            if data.get("action") is None:
                raise ValueError("You must tell me what to do!")
            ret = None
            if data["action"] == "run":
                ret = self.run_handler(data)
            elif data["action"] == "set_wall_limit":
                self._wall_limit = data["limit"]
            elif data["action"] == "setrlimit":
                self.res_limits[data["res"]] = data["limit"]
            elif data["action"] == "set_env":
                self.env[data["var"]] = data["val"]
            elif data["action"] == "set_stdin_file":
                self.stdin_file = data["file"]
            elif data["action"] == "set_stdout_file":
                self.stdout_file = data["file"]
            elif data["action"] == "set_stderr_file":
                self.stderr_file = data["file"]
            elif data["action"] == "cleanup":
                ret = self.cleanup_handler(data)
            elif data["action"] == "delete_file":
                try:
                    os.unlink(os.path.join(self.td, data["path"]))
                except FileNotFoundError:
                    pass
            elif data["action"] == "create_file":
                path = os.path.join(self.td, data["path"])
                try:
                    os.makedirs(os.path.dirname(path))
                except FileExistsError:
                    pass
                with open(path, "w"):
                    pass
                os.chmod(path, data["mode"])
            elif data["action"] == "add_to_file":
                with open(os.path.join(self.td, data["path"]), 'ab') as f:
                    f.write(data["contents"])
            elif data["action"] == "allow_path":
                orig = data["path"]
                while data["path"].startswith(os.sep):
                    data["path"] = data["path"][1:]
                dst = os.path.join(self.td, data["path"])
                try:
                    os.makedirs(dst)
                except FileExistsError:
                    pass
                mount(orig.encode(), dst.encode(), b"", MS_BIND | MS_RDONLY, b"")
                self.mounts.append(dst)
            else:
                raise ValueError("Unknown action %s!" % data['action'])
            self.send(ret)
        except Exception as e:
            self.send(e)

    cdef child_proc(self):
        self.go_on = 1
        self.mounts = []
        self.env = dict()
        self.res_limits = {
            resource.RLIMIT_STACK: (resource.RLIM_INFINITY, resource.RLIM_INFINITY),
            resource.RLIMIT_MEMLOCK: (0, 0)
        }
        self._wall_limit = 0
        self.stdin_file = "stdin"
        self.stdout_file = "stdout"
        self.stderr_file = "stderr"
#        print(self.recv(10))
        self.td = tempfile.mkdtemp()
        mnt_opt = ("size=%sK" % self.size).encode()
        mount(os.path.basename(self.td).encode(), self.td.encode(), b"tmpfs", 0, <char*>mnt_opt)
        printf("UID: %d\nEUID: %d\nPID: %d\n", getuid(), geteuid(), getpid())
        while self.go_on:
            self.do_work()
        return 0

    def __init__(self, size=256*1024):
        cdef int original_uid = getuid()
        cdef int original_gid = getgid()
        cdef char* stack = <char*> malloc(STACK_SIZE)
        self.parent_child_pipe = os.pipe()
        self.child_parent_pipe = os.pipe()
        self.size = size

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

    def set_stdin_file(self, file):
        self.send({
            "action": "set_stdin_file",
            "file": file
        })

    def set_stdout_file(self, file):
        self.send({
            "action": "set_stdout_file",
            "file": file
        })

    def set_stderr_file(self, file):
        self.send({
            "action": "set_stderr_file",
            "file": file
        })

    def delete_file(self, path):
        self.send({
            "action": "delete_file",
            "path": path
        })

    def create_file(self, path, mode):
        self.send({
            "action": "create_file",
            "path": path,
            "mode": mode
        })

    def add_to_file(self, path, contents):
        self.send({
            "action": "add_to_file",
            "path": path,
            "contents": contents
        })

    def add_file(self, path, contents, mode=stat.S_IRUSR | stat.S_IWUSR):
        self.delete_file(path)
        self.create_file(path, mode)
        cur = 0
        while cur < len(contents):
            self.add_to_file(path, contents[cur:cur+CHUNK_SIZE])
            cur += CHUNK_SIZE

    def add_file_from_path(self, path, orig_path):
        self.delete_file(path)
        self.create_file(path, os.stat(orig_path)[stat.ST_MODE])
        with open(orig_path, 'rb') as f:
            cur = 0
            while True:
                data = f.read(CHUNK_SIZE)
                if len(data) == 0:
                    break
                self.add_to_file(path, data)
                cur += len(data)

    def allow_path(self, path):
        self.send({
            "action": "allow_path",
            "path": path
        })

    def memory_limit(self, limit):
        self.rlimit(resource.RLIMIT_AS, (limit, limit))

    def time_limit(self, limit):
        self.rlimit(resource.RLIMIT_CPU, (limit, limit))

    def process_limit(self, limit):
        self.rlimit(resource.RLIMIT_NPROC, (limit+1, limit+1))

    def run(self, command, get_stdout=True, get_stderr=True):
        if isinstance(command, basestring) or (not all(isinstance(itm, basestring) for itm in command)):
            raise ValueError("command must be a list of strings!")
        return self.send({
            "action": "run",
            "command": command,
            "get_stdout": get_stdout,
            "get_stderr": get_stderr
        })

    def cleanup(self, get_files=False):
        self.send({
            "action": "cleanup",
            "get_files": get_files
        })
