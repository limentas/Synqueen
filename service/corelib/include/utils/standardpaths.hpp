#pragma once

#include <string>

namespace synqueen {

class StandardPaths {
public:
  static void initialize(const std::string &appName);
  static std::string getConfigPath();
  static std::string getDataPath();

private:
  StandardPaths();
  virtual ~StandardPaths();

  static StandardPaths *getInstance();

  void initializePrivate(const std::string &appName);

  std::string requestHomePathPrivate();
  std::string getConfigPathPrivate();
  std::string getDataPathPrivate();

  std::string getEnvOrEmpty(const char *name);

private:
  static StandardPaths *self;
  static bool destroyed;

  std::string configPath;
  std::string dataPath;
};

} // namespace synqueen
