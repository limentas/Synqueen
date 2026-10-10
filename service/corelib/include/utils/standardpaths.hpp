#pragma once

#include <filesystem>
#include <string>

namespace synqueen {

class StandardPaths {
public:
  static void initialize(const std::string &appName);
  static std::filesystem::path getConfigPath();
  static std::filesystem::path getDataPath();
  static std::filesystem::path getTempPath();

  // Puts all standard paths (config, data, temp) inside a temporary directory.
  // For testing purposes.
  static void initializeInTemp(const std::string &appName);

private:
  StandardPaths() = default;
  ~StandardPaths();

  static StandardPaths *getInstance();

  void initializePrivate(const std::string &appName);
  void initializeInTempPrivate(const std::string &appName);

  std::filesystem::path requestHomePathPrivate();
  void checkAppNamePrivate(const std::string &appName);
  void makeSureFoldersExist();
  std::filesystem::path getConfigPathPrivate();
  std::filesystem::path getDataPathPrivate();
  std::filesystem::path getTempPathPrivate();

  std::string getEnvOrEmpty(const char *name);

private:
  static StandardPaths *self;
  static bool destroyed;

  std::filesystem::path configPath;
  std::filesystem::path dataPath;
  std::filesystem::path tempPath;
};

} // namespace synqueen
