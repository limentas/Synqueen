#pragma once

#include "rapidjson/document.h"
#include <filesystem>
#include <string>
#include <vector>

namespace synqueen {

struct CloudSyncPointBase {
  std::string driver;
};

struct FolderSettings {
  std::filesystem::path path;
  std::vector<CloudSyncPointBase> cloudSyncPoints;
};

struct Settings {
  std::vector<FolderSettings> folders;
};

class SettingsProvider {
public:
  ~SettingsProvider() = default;

  Settings loadSettingsFromJson(const std::filesystem::path &path);
  void saveSettingsToJson(const std::filesystem::path &path,
                          const Settings &settings);

private:
  Settings createDefaultSettingsFile(const std::filesystem::path &path);
  bool validateSchema(const rapidjson::Document &document,
                      std::string &errorMessage);

private:
  static const std::string jsonSchema;
  static const int myVersion = 1;
  static const int defaultBufferSize = 1024;
};

} // namespace synqueen
