#pragma once

#include "icloudgate.hpp"
#include "rclonerpc.hpp"

#include <filesystem>
#include <list>
#include <string>

namespace synqueen::cloud {

class RcloneCloudGate : public ICloudGate {
public:
  RcloneCloudGate(uv_loop_t &loop) : rcloneRpc(loop) {}
  virtual ~RcloneCloudGate() = default;

  virtual corral::Task<void> shutdown() override;

  virtual corral::Task<bool>
  doesFolderExist(const CloudDestinationConfig &config) override;

  virtual corral::Task<void>
  createFolder(const CloudDestinationConfig &config) override;

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
  RcloneRpc rcloneRpc;
};

} // namespace synqueen::cloud
