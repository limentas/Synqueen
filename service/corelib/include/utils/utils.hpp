#pragma once

#include "corralheader.hpp"
#include "platform.hpp"

#include <cstdlib>
#include <filesystem>
#include <list>
#include <string>
#include <string_view>
#include <utility>
#define NOMINMAX
#include <uv.h>

#ifdef SQ_OS_WINDOWS
#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_STR "\\"
#else
#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_STR "/"
#endif

namespace synqueen {

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

std::string
replaceAll(const char *input,
           std::list<std::pair<const char *, const char *>> replacements);
std::string
replaceAll(const std::string_view &input,
           std::list<std::pair<std::string, std::string>> replacements);

// Removes all characters from the end of `input` that are present in
// `characters`
std::string removeAtEnd(const std::string &input,
                        const std::string &characters);

corral::Task<std::string> createTemporaryFolder(const std::string &templateStr,
                                                uv_loop_t *loop);

corral::Task<std::string> createTemporaryFile(const std::string &templateStr,
                                              uv_loop_t *loop);

} // namespace synqueen
