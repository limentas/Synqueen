#pragma once

#include "platform.hpp"
#include <cstdlib>
#include <string>

#ifdef SQ_OS_WINDOWS
#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_STR "\\"
#else
#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_STR "/"
#endif

// Returns a string where unprintable characters are replaced with their hex
// representations
std::string toPrintable(const char *data, size_t length);
std::string toPrintable(const std::string &input);
