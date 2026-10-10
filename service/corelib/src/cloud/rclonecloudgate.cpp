#include "rclonecloudgate.hpp"

namespace synqueen::cloud {

corral::Task<FileDetailsList>
RcloneCloudGate::listPatchFiles(const CloudDestinationConfig &config) {
  return corral::Task<FileDetailsList>();
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
