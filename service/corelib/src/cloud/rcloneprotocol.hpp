#pragma once

#include "cloud/rclonecloudgate.hpp"
#include "utils/jsontypes.hpp"

namespace synqueen::cloud {

class RcloneProtocol {
public:
  static synqueen::utils::JsonDocumentDefaultPtr
  prepareInputForExecution(json::Document &input);

  static json::Document createSetConfigPathCommand(const std::string &path);
  static json::Document createConfigEncryptionCommand(const std::string &key);
  static std::string createCheckCommandStatusCommand(long long jobId);

  static void createInitRemoteCommand(const RcloneFolderConfig &config,
                                      json::Document &document);

  static void createGDriveRemoteCommand(const RcloneFolderConfig &config,
                                        json::Document &document);
  static void createLocalRemoteCommand(const RcloneFolderConfig &config,
                                       json::Document &document);

private:
  RcloneProtocol() = default;
};

} // namespace synqueen::cloud
