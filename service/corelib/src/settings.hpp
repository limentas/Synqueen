#pragma once

#include "rapidjson/document.h"
#include <string>
#include <vector>

namespace synqueen {

struct CloudSyncPointBase {
  std::string driver;
};

struct FolderSettings {
  std::string path;
  std::vector<CloudSyncPointBase> cloudSyncPoints;
};

struct Settings {
  std::vector<FolderSettings> folders;
};

class SettingsProvider {
public:
  ~SettingsProvider() = default;

  Settings loadSettingsFromJson(const std::string &path);
  void saveSettingsToJson(const std::string &path, const Settings &settings);

private:
  Settings createDefaultSettingsFile(const std::string &path);
  bool validateSchema(const rapidjson::Document &document,
                      std::string &errorMessage);

private:
  static const std::string jsonSchema;
  static const int myVersion = 1;
};

} // namespace synqueen
