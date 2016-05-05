#ifndef COTTON_UTIL_HPP
#define COTTON_UTIL_HPP

#ifdef __unix__
#include <string>
#include <cstring>

std::string serror(const std::string& base, int err = errno);
int rm_rf(const std::string& fld);
#endif



#endif
