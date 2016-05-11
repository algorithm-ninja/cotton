#ifndef COTTON_UTIL_HPP
#define COTTON_UTIL_HPP

#if defined(__unix__) || defined(__APPLE__)
#include <errno.h>
#include <string>
#include <cstring>

std::string serror(const std::string& base, int err = errno);
int rm_rf(const std::string& fld);
#endif



#endif
