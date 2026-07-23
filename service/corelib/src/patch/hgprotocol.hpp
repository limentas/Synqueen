#pragma once

#include <cstddef>
#include <string>

namespace synqueen {

class HgProtocol {
public:
  enum class HgChannel { Output, Error, Result, Debug, Input, Line };
  struct HgOutput {
    bool hasData;
    HgChannel channel;
    std::string data;
  };

  static void parseHgOutput(const char *buffer, size_t length,
                            HgOutput &hgOutput);
};

} // namespace synqueen
