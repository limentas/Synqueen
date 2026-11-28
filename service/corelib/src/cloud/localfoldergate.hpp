#pragma once

#include "icloudgate.hpp"

#include <filesystem>

namespace synqueen::cloud {

// Holds absolute folder path in CloudDestinationConfig::cloudPath
class LocalFolderConfig : public CloudDestinationConfig {
public:
  LocalFolderConfig(std::string id, std::filesystem::path cloudPath)
      : CloudDestinationConfig(std::move(id), std::move(cloudPath)) {}
  virtual ~LocalFolderConfig() = default;

  virtual CloudGateType gateType() const override {
    return CloudGateType::LocalFolder;
  }
};

class LocalFolderGate : public ICloudGate {
public:
  LocalFolderGate() = default;
  virtual ~LocalFolderGate() = default;

  virtual corral::Task<void> shutdown() override;

  virtual corral::Task<void>
  initDestination(const CloudDestinationConfig &config) override;

  virtual corral::Task<bool>
  doesFolderExist(const CloudDestinationConfig &config) override;

  virtual corral::Task<void>
  initFolder(const CloudDestinationConfig &config) override;

  virtual corral::Task<FileDetailsByFileName>
  listPatchFiles(const CloudDestinationConfig &config) override;

  virtual corral::Task<void>
  uploadFiles(const CloudDestinationConfig &config,
              const std::list<std::filesystem::path> &files) override;

  virtual corral::Task<std::list<std::filesystem::path>>
  downloadFiles(const CloudDestinationConfig &config,
                const std::list<std::string> &files,
                const std::filesystem::path &destination) override;
};

} // namespace synqueen::cloud
