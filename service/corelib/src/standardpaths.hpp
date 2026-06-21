#pragma once

#include <string>

class StandardPaths {
public:
  static void initialize();
  static std::string getConfigPath();
  static std::string getDataPath();

private:
  StandardPaths();
  virtual ~StandardPaths();

  static StandardPaths *getInstance();

  void initializePrivate();

  std::string getConfigPathPrivate();
  std::string getDataPathPrivate();

private:
  static StandardPaths *self;
  static bool destroyed;

  std::string configPath;
  std::string dataPath;
};
