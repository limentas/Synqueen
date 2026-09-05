#pragma once

#include <filesystem>
#include <string>

namespace synqueen {

class StandardPaths {
public:
  static void initialize(const std::string &appName);
  static std::filesystem::path getConfigPath();
  static std::filesystem::path getDataPath();

private:
  StandardPaths();
  virtual ~StandardPaths();

  static StandardPaths *getInstance();

  void initializePrivate(const std::string &appName);

  std::filesystem::path requestHomePathPrivate();
  std::filesystem::path getConfigPathPrivate();
  std::filesystem::path getDataPathPrivate();

  std::string getEnvOrEmpty(const char *name);

private:
  static StandardPaths *self;
  static bool destroyed;

  std::filesystem::path configPath;
  std::filesystem::path dataPath;
};

} // namespace synqueen
