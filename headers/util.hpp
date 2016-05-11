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

std::string serror(const std::string& base, int err = errno);
int rm_rf(const std::string& fld);
#endif



#endif
