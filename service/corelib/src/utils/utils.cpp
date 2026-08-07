#include "utils/utils.hpp"

#include <cctype>
#include <cstdio>
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
