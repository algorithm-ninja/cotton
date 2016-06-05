#include "util.hpp"
#ifdef COTTON_UNIX
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

std::string serror(const std::string& base, int err) {
    return base + ": " + strerror(err);
}

int rm_rf(const std::string& fld) {
    DIR *cur = opendir(fld.c_str());
    if (cur) {
        struct dirent *dir;
        while ((dir=readdir(cur))) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
            struct stat statbuf;
            std::string entry = fld + "/" + dir->d_name;
            if (stat(entry.c_str(), &statbuf) == -1) {
                return errno;
            } else {
                int tmp;
                if (S_ISDIR(statbuf.st_mode)) tmp = rm_rf(entry);
                else tmp = unlink(entry.c_str());
                if (tmp != 0) return errno;
            }
        }
        closedir(cur);
    }
    if (rmdir(fld.c_str()) == -1) return errno;
    return 0;
}

int mkdirs(const std::string& fld, mode_t mode) {
    std::vector<char> folder(fld.begin(), fld.end());
    folder.emplace_back(0);
    int start = 0;
    while (fld[start] == '/') start++;
    for (unsigned i=start; i<folder.size(); i++) {
        if (folder[i] != '/') continue;
        folder[i] = 0;
        if (mkdir(folder.data(), mode) == -1 && errno != EEXIST) return -1;
        folder[i] = '/';
    }
    if (mkdir(folder.data(), mode) == -1 && errno != EEXIST) return -1;
    return 0;
}

int Privileged::counter = 0;

#endif
