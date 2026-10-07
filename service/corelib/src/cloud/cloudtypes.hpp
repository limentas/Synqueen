#pragma once

#include <chrono>
#include <cstddef>
#include <list>
#include <string>

namespace synqueen::cloud {

class CloudDestinationConfig {
public:
  CloudDestinationConfig() = default;
  virtual ~CloudDestinationConfig() = default;

  std::string driverName;
};

struct FileDetails {
  std::string fileName;
  std::size_t fileSize;
  std::chrono::time_point<std::chrono::system_clock> lastModifiedTime;
};

typedef std::list<FileDetails> FileDetailsList;

} // namespace synqueen::cloud
