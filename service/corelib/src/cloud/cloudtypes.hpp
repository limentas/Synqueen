#pragma once

#include <chrono>
#include <cstddef>
#include <list>
#include <memory>
#include <string>

namespace synqueen::cloud {

enum class CloudGateType { LocalFolder, Rclone };

class CloudDestinationConfig {
public:
  CloudDestinationConfig() = default;
  virtual ~CloudDestinationConfig() = default;

  virtual CloudGateType gateType() const = 0;
};

typedef std::shared_ptr<CloudDestinationConfig> CloudDestinationConfigPtr;

struct FileDetails {
  int index;
  std::string fileName;
  std::size_t fileSize;
  std::chrono::time_point<std::chrono::system_clock> lastModifiedTime;
};

typedef std::list<FileDetails> FileDetailsList;

} // namespace synqueen::cloud
