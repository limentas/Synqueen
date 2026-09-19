#pragma once

#include <chrono>
#include <cstddef>
#include <list>
#include <string>

namespace synqueen {

class CloudConfig {
public:
  CloudConfig() = default;
  virtual ~CloudConfig() = default;
};

struct FileDetails {
  std::string fileName;
  std::size_t fileSize;
  std::chrono::time_point<std::chrono::system_clock> lastModifiedTime;
};

typedef std::list<FileDetails> FileDetailsList;

class ICloudGate {
public:
  virtual ~ICloudGate() = default;

  virtual FileDetailsList listPatchFiles(CloudConfig &config,
                                         const std::string &path) = 0;
};

} // namespace synqueen
