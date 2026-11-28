#include "settings.hpp"

#include "cloud/cloudtypes.hpp"
#include "cloud/localfoldergate.hpp"
#include "cloud/rclonecloudgate.hpp"

#include "utils/exceptions.hpp"
#include "utils/logger.hpp"

#include <rapidjson/document.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/schema.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <memory>
#include <stdexcept>

using namespace rapidjson;
using namespace synqueen::cloud;

namespace fs = std::filesystem;

namespace synqueen {

Settings SettingsProvider::loadSettingsFromJson(const fs::path &path) {
  auto fp = std::unique_ptr<FILE, decltype(&std::fclose)>(
      std::fopen(path.string().c_str(), "rb"),
      &std::fclose); // non-Windows use "r"
  if (!fp) {
    return createDefaultSettingsFile(path);
  }

  char readBuffer[defaultBufferSize];
  FileReadStream bis(fp.get(), readBuffer, sizeof(readBuffer));
  EncodedInputStream<UTF8<>, FileReadStream> eis(bis); // wraps bis into eis

  Document document;
  document.ParseStream<kParseNoFlags, UTF8<>>(eis);

  if (document.HasParseError()) {
    throw SqException("Failed to parse settings.json");
  }

  std::string errorMessage;
  if (!validateSchema(document, errorMessage)) {
    throw SqException("Settings JSON does not conform to schema. Error: " +
                      errorMessage);
  }

  auto version = document["version"].GetInt();
  if (version != myVersion) {
    throw SqException("Settings JSON version mismatch");
  }

  Settings settings;
  if (document.HasMember("folders")) {
    auto folders = document["folders"].GetArray();
    for (const auto &folder : folders) {
      FolderSettings folderSettings;
      if (folder.HasMember("id")) {
        folderSettings.id = folder["id"].GetString();
      }
      if (folder.HasMember("path")) {
        folderSettings.path = folder["path"].GetString();
      }
      if (folder.HasMember("cloudDestinations")) {
        auto cloudDestinations = folder["cloudDestinations"].GetArray();
        for (const auto &cloudDest : cloudDestinations) {
          if (!cloudDest.HasMember("gateType")) {
            SPDLOG_ERROR(
                "Cloud destination missing 'gateType' field for folder "
                "{}. Skipping this entry.",
                folderSettings.path);
            continue;
          }
          auto gateType =
              static_cast<CloudGateType>(cloudDest["gateType"].GetInt());

          if (!cloudDest.HasMember("id")) {
            SPDLOG_ERROR("Cloud destination missing 'id' field for folder "
                         "{}. Skipping this entry.",
                         folderSettings.path);
            continue;
          }
          auto id = cloudDest["id"].GetString();

          if (!cloudDest.HasMember("cloudPath")) {
            SPDLOG_ERROR(
                "Cloud destination missing 'cloudPath' field for folder "
                "{}. Skipping this entry.",
                folderSettings.path);
            continue;
          }
          auto path = cloudDest["cloudPath"].GetString();

          switch (gateType) {
          case CloudGateType::LocalFolder: {
            auto c = std::make_shared<LocalFolderConfig>(id, fs::path(path));
            folderSettings.cloudDestinations.push_back(c);
            break;
          }
          case CloudGateType::Rclone: {
            if (!cloudDest.HasMember("remoteType")) {
              SPDLOG_ERROR(
                  "Cloud destination missing 'remoteType' field for folder "
                  "{}. Skipping this entry.",
                  folderSettings.path);
              continue;
            }
            auto remoteType = cloudDest["remoteType"].GetString();
            if (!cloudDest.HasMember("useEncryption")) {
              SPDLOG_ERROR(
                  "Cloud destination missing 'useEncryption' field for folder "
                  "{}. Skipping this entry.",
                  folderSettings.path);
              continue;
            }
            auto useEncryption = cloudDest["useEncryption"].GetBool();
            auto c = std::make_shared<RcloneFolderConfig>(id, fs::path(path));
            c->remoteType = rcloneRemoteTypeFromString(remoteType);
            c->useEncryption = useEncryption;

            folderSettings.cloudDestinations.push_back(c);
            break;
          }
          default:
            SPDLOG_ERROR(
                "Unknown cloud gate type for folder {}. Skipping this entry.",
                folderSettings.path);
            continue;
          }
        }
      }
      settings.folders.push_back(folderSettings);
    }
  }

  return settings;
}

void SettingsProvider::saveSettingsToJson(const fs::path &path,
                                          const Settings &settings) {
  std::ofstream ofs;
  ofs.open(path, std::ios::out | std::ios::binary);
  if (!ofs.is_open()) {
    throw SqException("Failed to open settings.json for writing");
  }
  OStreamWrapper osw(ofs);
  typedef EncodedOutputStream<UTF8<>, OStreamWrapper> OutputStream;
  OutputStream eos(osw); // wraps os into eos
  PrettyWriter<OutputStream> writer(eos);

  writer.StartObject();
  writer.Key("version");
  writer.Int(myVersion);
  writer.Key("folders");
  writer.StartArray();
  for (const auto &folder : settings.folders) {
    writer.StartObject();
    writer.Key("id");
    auto strId = folder.id;
    writer.String(strId.c_str(), strId.length());
    writer.Key("path");
    auto strPath = folder.path.string();
    writer.String(strPath.c_str(), strPath.length());
    writer.Key("cloudDestinations");
    writer.StartArray();
    for (const auto &cloudDestPtr : folder.cloudDestinations) {
      writer.StartObject();
      writer.Key("id");
      writer.String(cloudDestPtr->id.c_str(), cloudDestPtr->id.length());
      writer.Key("cloudPath");
      auto pathStr = cloudDestPtr->cloudPath.string();
      writer.String(pathStr.c_str(), pathStr.length());
      writer.Key("gateType");
      writer.Int(static_cast<int>(cloudDestPtr->gateType()));
      if (cloudDestPtr->gateType() == CloudGateType::Rclone) {
        auto rclonePtr =
            std::static_pointer_cast<RcloneFolderConfig>(cloudDestPtr);
        writer.Key("remoteType");
        auto remoteTypeStr = rcloneRemoteTypeToString(rclonePtr->remoteType);
        writer.String(remoteTypeStr.c_str(), remoteTypeStr.length());
        writer.Key("useEncryption");
        writer.Bool(rclonePtr->useEncryption);
      }
      writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
  }
  writer.EndArray();
  writer.EndObject();
}

Settings SettingsProvider::createDefaultSettingsFile(const fs::path &path) {
  auto s = Settings();
  saveSettingsToJson(path, s);
  return s;
}

bool SettingsProvider::validateSchema(const Document &document,
                                      std::string &errorMessage) {
  SchemaDocument schema(Document().Parse(jsonSchema.c_str()));
  SchemaValidator validator(schema);

  if (!document.Accept(validator)) {
    // Standard way of getting errors lacks of details, so we output internal
    // json representation of the error.
    StringBuffer ss;
    typedef EncodedOutputStream<UTF8<>, StringBuffer> OutputStream;
    OutputStream eos(ss, false); // wraps ss into eos
    PrettyWriter<OutputStream> writer(eos);
    validator.GetError().Accept(writer);
    errorMessage = ss.GetString();
    return false;
  }

  return true;
}

std::string SettingsProvider::rcloneRemoteTypeToString(
    RcloneFolderConfig::RemoteType remoteType) {
  switch (remoteType) {
  case RcloneFolderConfig::RemoteType::Local:
    return "Local";
  case RcloneFolderConfig::RemoteType::GoogleDrive:
    return "GoogleDrive";
  case RcloneFolderConfig::RemoteType::OneDrive:
    return "OneDrive";
  default:
    throw SqException("Unsupported Rclone remote type");
  }
}

RcloneFolderConfig::RemoteType
SettingsProvider::rcloneRemoteTypeFromString(const std::string &remoteTypeStr) {
  if (remoteTypeStr == "Local") {
    return RcloneFolderConfig::RemoteType::Local;
  } else if (remoteTypeStr == "GoogleDrive") {
    return RcloneFolderConfig::RemoteType::GoogleDrive;
  } else if (remoteTypeStr == "OneDrive") {
    return RcloneFolderConfig::RemoteType::OneDrive;
  } else {
    throw SqException("Unsupported Rclone remote type string");
  }
}

} // namespace synqueen
