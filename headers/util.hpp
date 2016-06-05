#ifndef COTTON_UTIL_HPP
#define COTTON_UTIL_HPP

#if defined(__linux__)
#define COTTON_LINUX
#endif

#if defined(__FreeBSD_kernel__)
#define COTTON_FREEBSD
#endif

#if defined(_WIN32)
#define COTTON_WINDOWS
#endif

#if defined(__unix__) || defined(__APPLE__)
#define COTTON_UNIX
#elif !(defined(COTTON_LINUX) || defined(COTTON_FREEBSD) || defined(COTTON_WINDOWS))
#define COTTON_UNKNOWN
#endif

#ifdef COTTON_UNIX
#include <errno.h>
#include <string>
#include <cstring>
#include <chrono>
#include <boost/serialization/export.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

std::string serror(const std::string& base, int err = errno);
int rm_rf(const std::string& fld);
int mkdirs(const std::string& path, mode_t mode);

struct Privileged {
    static int counter;
    Privileged() {
        if (counter == 0) setreuid(geteuid(), getuid());
        counter++;
    }
    ~Privileged() {
        counter--;
        if (counter == 0) setreuid(geteuid(), getuid());
    }
};

#endif

class time_limit_t {
    size_t microsecs_;
public:
    constexpr time_limit_t(double d = 0): microsecs_(d*1'000'000UL) {}
    constexpr time_limit_t(int d): microsecs_(d*1'000'000UL) {}
    template <typename... Args>
    constexpr time_limit_t(std::chrono::duration<Args...> duration):
        microsecs_(std::chrono::duration_cast<std::chrono::microseconds>(duration).count()) {}
    constexpr time_limit_t(struct timeval t):
        microsecs_(t.tv_sec*1'000'000 + t.tv_usec/1000) {}
    friend class boost::serialization::access;
    template <typename Archive> void serialize(Archive &ar, const unsigned int version) {
        ar & microsecs_;
    }
    size_t seconds() const {
        return (microsecs_-1)/1'000'000+1;
    }
    double double_seconds() const {
        return 1.0*microsecs_/1'000'000;
    }
    std::string to_string() const {
        return std::to_string(1.0*microsecs_/1'000'000) + 's';
    }
    static time_limit_t from_seconds(double secs) {
        return time_limit_t(secs);
    }
    size_t microseconds() const {
        return microsecs_;
    }
    static time_limit_t from_microseconds(double usecs) {
        time_limit_t tmp = 0;
        tmp.microsecs_ = usecs;
        return tmp;
    }
    time_limit_t& operator+=(const time_limit_t& other) {
        microsecs_ += other.microsecs_;
        return *this;
    }
    time_limit_t operator+(const time_limit_t& other) const {
        time_limit_t tmp = *this;
        tmp += other;
        return tmp;
    }
};

class space_limit_t {
    size_t bytes_;
public:
    constexpr space_limit_t(int s): bytes_(s*1024LLU) {}
    constexpr space_limit_t(double s = 0): bytes_(s*1024LLU) {}
    friend class boost::serialization::access;
    template <typename Archive> void serialize(Archive &ar, const unsigned int version) {
        ar & bytes_;
    }
    size_t bytes() const {
        return bytes_;
    }
    size_t kilobytes() const {
        return bytes_/1024;
    }
    std::string to_string() const {
        if (bytes_ > 1024*1024) {
            return std::to_string(1.0*bytes_/1024/1024) + "MiB";
        } else {
            return std::to_string(1.0*bytes_/1024) + "KiB";
        }
    }
    static space_limit_t from_bytes(size_t rlu) {
        space_limit_t tmp;
        tmp.bytes_ = rlu;
        return tmp;
    }
    size_t rlimit_unit() {
#if defined(__APPLE__)
        return bytes();
#else
        return kilobytes();
#endif
    }
    static space_limit_t from_rlimit_unit(size_t rlu) {
        space_limit_t tmp;
#if defined(__APPLE__)
        tmp.bytes_ = rlu;
#else
        tmp.bytes_ = rlu*1024;
#endif
        return tmp;
    }
};

#endif
