#include "rclonecloudgate.hpp"

#include "rcloneprotocol.hpp"

namespace synqueen::cloud {
corral::Task<void> RcloneCloudGate::shutdown() { return rcloneRpc.shutdown(); }

corral::Task<void>
RcloneCloudGate::initDestination(const CloudDestinationConfig &config) {
  if (config.gateType() != CloudGateType::Rclone) {
    throw SqException("Invalid gate type for RcloneCloudGate");
  }
  json::Document d;
  RcloneProtocol::createInitRemoteCommand(
      static_cast<const RcloneFolderConfig &>(config), d);

  auto result = co_await rcloneRpc.runCommand("config/create", std::move(d));
  if (!result.error.empty()) {
    throw SqException("Failed to initialize remote: " + result.error);
  }
  co_return;
}

corral::Task<bool>
RcloneCloudGate::doesFolderExist(const CloudDestinationConfig &config) {
  json::Document d;
  d.AddMember("fs",
              json::Value().SetString(config.id.c_str(), config.id.size(),
                                      d.GetAllocator()),
              d.GetAllocator());
  d.AddMember("remote",
              json::Value().SetString(config.cloudPath.string().c_str(),
                                      config.cloudPath.string().size(),
                                      d.GetAllocator()),
              d.GetAllocator());
  auto result = co_await rcloneRpc.runCommand("operations/stat", std::move(d));
  if (!result.error.empty()) {
    throw SqException("Failed to check if folder exists: " + result.error);
  }

  // TODO: Parse the result to determine if the folder actually exists
  co_return true;
}

corral::Task<void>
RcloneCloudGate::initFolder(const CloudDestinationConfig &config) {
  json::Document d;
  d.AddMember("fs",
              json::Value().SetString(config.id.c_str(), config.id.size(),
                                      d.GetAllocator()),
              d.GetAllocator());
  d.AddMember("remote",
              json::Value().SetString(config.cloudPath.string().c_str(),
                                      config.cloudPath.string().size(),
                                      d.GetAllocator()),
              d.GetAllocator());
  auto result = co_await rcloneRpc.runCommand("operations/mkdir", std::move(d));
  if (!result.error.empty()) {
    throw SqException("Failed to create folder: " + result.error);
  }
  co_return;
}

corral::Task<FileDetailsByFileName>
RcloneCloudGate::listPatchFiles(const CloudDestinationConfig &config) {
  return corral::Task<FileDetailsByFileName>();
}

corral::Task<void>
RcloneCloudGate::uploadFiles(const CloudDestinationConfig &config,
                             const std::list<std::filesystem::path> &files) {
  return corral::Task<void>();
}

corral::Task<std::list<std::filesystem::path>>
RcloneCloudGate::downloadFiles(const CloudDestinationConfig &config,
                               const std::list<std::string> &files,
                               const std::filesystem::path &destination) {
  return corral::Task<std::list<std::filesystem::path>>();
}

} // namespace synqueen::cloud
