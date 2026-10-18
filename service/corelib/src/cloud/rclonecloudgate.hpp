#pragma once

#include "icloudgate.hpp"

#include <filesystem>
#include <list>
#include <string>

namespace synqueen::cloud {

class RcloneCloudGate : public ICloudGate {
public:
  RcloneCloudGate() = default;
  virtual ~RcloneCloudGate() = default;

  virtual corral::Task<FileDetailsByFileName>
  listPatchFiles(const CloudDestinationConfig &config) override;

  virtual corral::Task<void>
  uploadFiles(const CloudDestinationConfig &config,
              const std::list<std::filesystem::path> &files) override;

  virtual corral::Task<std::list<std::filesystem::path>>
  downloadFiles(const CloudDestinationConfig &config,
                const std::list<std::string> &files,
                const std::filesystem::path &destination) override;

private:
  std::string remote;
};

} // namespace synqueen::cloud
