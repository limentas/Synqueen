#include "rcloneprotocol.hpp"

namespace synqueen::cloud {

synqueen::utils::JsonDocumentDefaultPtr
RcloneProtocol::prepareInputForExecution(json::Document &input) {
  auto modifiedInput = std::make_shared<json::Document>(std::move(input));
  modifiedInput->AddMember("_async", json::Value().SetBool(true),
                           modifiedInput->GetAllocator());
  return modifiedInput;
}

json::Document
RcloneProtocol::createSetConfigPathCommand(const std::string &path) {
  json::Document input;
  input.SetObject().AddMember(
      "path",
      json::Value().SetString(path.c_str(), path.size(), input.GetAllocator()),
      input.GetAllocator());
  return input;
}

json::Document
RcloneProtocol::createConfigEncryptionCommand(const std::string &key) {
  json::Document input;
  input.SetObject().AddMember(
      "configPassword",
      json::Value().SetString(key.c_str(), key.size(), input.GetAllocator()),
      input.GetAllocator());
  return input;
}

std::string RcloneProtocol::createCheckCommandStatusCommand(long long jobId) {
  return std::string("{ \"jobid\": ") + std::to_string(jobId) + " }";
}

void RcloneProtocol::createInitRemoteCommand(const RcloneFolderConfig &config,
                                             json::Document &document) {
  if (config.remoteType == RcloneFolderConfig::RemoteType::GoogleDrive) {
    createGDriveRemoteCommand(config, document);
  } else if (config.remoteType == RcloneFolderConfig::RemoteType::Local) {
    createLocalRemoteCommand(config, document);
  }
}

void RcloneProtocol::createGDriveRemoteCommand(const RcloneFolderConfig &config,
                                               json::Document &document) {
  auto &root = document.SetObject();
  root.AddMember("type", "drive", document.GetAllocator());
  root.AddMember("name",
                 json::Value().SetString(config.id.c_str(), config.id.size(),
                                         document.GetAllocator()),
                 document.GetAllocator());
  json::Value parameters(json::kObjectType);
  parameters.AddMember("client_id", "", document.GetAllocator());
  parameters.AddMember("client_secret", "", document.GetAllocator());
  parameters.AddMember("config_is_local", true, document.GetAllocator());
  parameters.AddMember("description", "Syqueen remote",
                       document.GetAllocator());
  parameters.AddMember("skip_gdocs", true, document.GetAllocator());
  parameters.AddMember("scope", "drive.file", document.GetAllocator());
  parameters.AddMember("skip_checksum_gphotos", true, document.GetAllocator());
  // Shared and team drives are not supported atm
  parameters.AddMember("config_change_team_drive", false,
                       document.GetAllocator());
  root.AddMember("parameters", parameters, document.GetAllocator());

  json::Value opt(json::kObjectType);
  opt.AddMember("obscure", true, document.GetAllocator());
  opt.AddMember("noOutput", true, document.GetAllocator());
  opt.AddMember("nonInteractive", true, document.GetAllocator());
  root.AddMember("opt", opt, document.GetAllocator());
}

void RcloneProtocol::createLocalRemoteCommand(const RcloneFolderConfig &config,
                                              json::Document &document) {
  auto &root = document.SetObject();
  root.AddMember("type", "local", document.GetAllocator());
  root.AddMember("name",
                 json::Value().SetString(config.id.c_str(), config.id.size(),
                                         document.GetAllocator()),
                 document.GetAllocator());
  json::Value parameters(json::kObjectType);
  parameters.AddMember("description", "Syqueen remote",
                       document.GetAllocator());
  root.AddMember("parameters", parameters, document.GetAllocator());

  json::Value opt(json::kObjectType);
  opt.AddMember("obscure", true, document.GetAllocator());
  opt.AddMember("noOutput", true, document.GetAllocator());
  opt.AddMember("nonInteractive", true, document.GetAllocator());
  root.AddMember("opt", opt, document.GetAllocator());
}

} // namespace synqueen::cloud
