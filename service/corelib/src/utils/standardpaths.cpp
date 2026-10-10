#include "utils/standardpaths.hpp"

#include "utils/platform.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>

#include <uv.h>

namespace fs = std::filesystem;

namespace synqueen {

StandardPaths *StandardPaths::self = nullptr;
bool StandardPaths::destroyed = false;

void StandardPaths::initialize(const std::string &appName) {
  if (self != nullptr)
    return;

  assert(destroyed != true && "StandardPaths instance was already destroyed");

  static std::once_flag flag;
  std::call_once(flag, [appName]() {
    static StandardPaths s;
    StandardPaths::self = &s;
    s.initializePrivate(appName);
  });
}

fs::path StandardPaths::getConfigPath() {
  return getInstance()->getConfigPathPrivate();
}

fs::path StandardPaths::getDataPath() {
  return getInstance()->getDataPathPrivate();
}

fs::path StandardPaths::getTempPath() {
  return getInstance()->getTempPathPrivate();
}

StandardPaths::~StandardPaths() {
  destroyed = true;
  self = nullptr;
}

StandardPaths *StandardPaths::getInstance() {
  assert(self != nullptr);
  return self;
}

void StandardPaths::initializePrivate(const std::string &appName) {
  assert(!appName.empty());
  assert(appName.contains('/') == false && "App name should not contain '/'");
  assert(appName.contains('\\') == false && "App name should not contain '\\'");

  const auto home = requestHomePathPrivate();
#if defined(SQ_OS_WINDOWS)
  fs::path roaming = getEnvOrEmpty("APPDATA");
  fs::path local = getEnvOrEmpty("LOCALAPPDATA");

  if (roaming.empty() && !home.empty())
    roaming = (home / "AppData" / "Roaming");
  if (local.empty() && !home.empty())
    local = (home / "AppData" / "Local");

  configPath = roaming.empty() ? (fs::path(".") / appName / "config")
                               : (roaming / appName);
  dataPath =
      local.empty() ? (fs::path(".") / appName / "data") : (local / appName);

#elif defined(SQ_OS_ANDROID)
  std::string base = getEnvOrEmpty("HOME");
  if (base.empty())
    base = "/data/local/tmp";

  configPath = (fs::path(base) / ".config" / appName);
  dataPath = (fs::path(base) / ".local" / "share" / appName);
#else
  fs::path xdgConfig = getEnvOrEmpty("XDG_CONFIG_HOME");
  fs::path xdgData = getEnvOrEmpty("XDG_DATA_HOME");

  if (xdgConfig.empty())
    xdgConfig = home.empty() ? fs::path(".") : (home / ".config");
  if (xdgData.empty())
    xdgData = home.empty() ? fs::path(".") : (home / ".local" / "share");

  configPath = (fs::path(xdgConfig) / appName);
  dataPath = (fs::path(xdgData) / appName);
#endif
  tempPath = dataPath / "temp";

  std::error_code ec;
  fs::create_directories(configPath, ec);
  if (ec) {
    std::cerr << "Failed to create config directory: " << configPath
              << ", error: " << ec.message() << std::endl;
  }
  ec.clear();
  fs::create_directories(dataPath, ec);
  if (ec) {
    std::cerr << "Failed to create data directory: " << dataPath
              << ", error: " << ec.message() << std::endl;
  }
  ec.clear();
  fs::remove_all(tempPath, ec);
  if (ec) {
    std::cerr << "Failed to remove temp directory: " << tempPath
              << ", error: " << ec.message() << std::endl;
  }
  ec.clear();
  fs::create_directories(tempPath, ec);
  if (ec) {
    std::cerr << "Failed to create temp directory: " << tempPath
              << ", error: " << ec.message() << std::endl;
  }
}

fs::path StandardPaths::requestHomePathPrivate() {
  size_t size = 0;
  uv_os_homedir(nullptr, &size);
  if (size == 0)
    return {};

  std::string out(size, '\0');
  if (uv_os_homedir(out.data(), &size) == 0) {
    if (size > 0)
      out.resize(size - 1); // strip trailing '\0'
    return out;
  }
  return fs::path();
}

fs::path StandardPaths::getConfigPathPrivate() { return configPath; }

fs::path StandardPaths::getDataPathPrivate() { return dataPath; }

fs::path StandardPaths::getTempPathPrivate() { return tempPath; }

std::string StandardPaths::getEnvOrEmpty(const char *name) {
  const char *v = std::getenv(name);
  return v ? std::string(v) : std::string();
}

} // namespace synqueen
