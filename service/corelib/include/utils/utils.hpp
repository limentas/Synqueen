#pragma once

#include "platform.hpp"
#include <cstdlib>
#include <string>
#include <string_view>

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

// Replaces all occurrences of `search` in `input` with `replace` and returns
// the resulting string
std::string replaceAll(const char *input, const char *search,
                       const char *replace);
std::string replaceAll(const std::string_view &input,
                       const std::string_view &search,
                       const std::string_view &replace);