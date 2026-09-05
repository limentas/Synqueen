#include "utils/utils.hpp"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <ranges>
#include <string>

std::string toPrintable(const char *data, size_t length) {
  std::string output;
  output.reserve(length * 4); // Worst case: all characters are unprintable
  for (size_t i = 0; i < length; ++i) {
    auto c = data[i];
    if (std::isprint(static_cast<unsigned char>(c))) {
      output += c;
    } else {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "\\x%02X",
               static_cast<unsigned char>(c));
      output += buffer;
    }
  }
  return output;
}

std::string toPrintable(const std::string &input) {
  return toPrintable(input.data(), input.size());
}

std::string replaceAll(const char *input, const char *search,
                       const char *replace) {
  return replaceAll(std::string_view(input), std::string_view(search),
                    std::string_view(replace));
}

std::string replaceAll(const std::string_view &input,
                       const std::string_view &search,
                       const std::string_view &replace) {
  assert(!search.empty() && "Search string must not be empty");
  return input | std::views::split(search) | std::views::join_with(replace) |
         std::ranges::to<std::string>();
}
