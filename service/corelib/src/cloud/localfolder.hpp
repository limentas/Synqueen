#pragma once

#include "icloudgate.hpp"

#include <filesystem>

namespace synqueen::cloud {

class LocalFolderConfig : public CloudDestinationConfig {
public:
  LocalFolderConfig() = default;
  virtual ~LocalFolderConfig() = default;

  virtual CloudGateType gateType() const override {
    return CloudGateType::LocalFolder;
  }

  std::filesystem::path folderPath;
};

class LocalFolder : public ICloudGate {
public:
  LocalFolder(const std::filesystem::path &path);
  virtual ~LocalFolder() = default;

  virtual corral::Task<FileDetailsList>
  listPatchFiles(const CloudDestinationConfig &config) override;

  virtual corral::Task<void>
  uploadFiles(const CloudDestinationConfig &config,
              const std::list<std::filesystem::path> &files) override;

  virtual corral::Task<std::list<std::filesystem::path>>
  downloadFiles(const CloudDestinationConfig &config,
                const std::list<std::string> &files,
                const std::filesystem::path &destination) override;

private:
  std::filesystem::path folderPath;
};

} // namespace synqueen::cloud
