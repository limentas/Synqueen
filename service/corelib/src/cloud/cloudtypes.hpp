#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace synqueen::cloud {

enum class CloudGateType { LocalFolder, Rclone };

class CloudDestinationConfig {
public:
  CloudDestinationConfig(std::string id, std::filesystem::path cloudPath)
      : id(std::move(id)), cloudPath(std::move(cloudPath)) {}
  virtual ~CloudDestinationConfig() = default;

  virtual CloudGateType gateType() const = 0;

  // Unique identifier for the cloud destination
  std::string id;
  // The subpath within the cloud destination where files are stored
  std::filesystem::path cloudPath;
};

typedef std::shared_ptr<CloudDestinationConfig> CloudDestinationConfigPtr;

struct FileDetails {
  int index;
  std::string fileName;
  std::size_t fileSize;
  std::chrono::time_point<std::chrono::system_clock> lastModifiedTime;
};

typedef std::map<std::string, FileDetails> FileDetailsByFileName;

} // namespace synqueen::cloud
