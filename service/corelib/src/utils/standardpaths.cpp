#include "utils/standardpaths.hpp"

#include "utils/platform.hpp"
#include "utils/uvheader.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>

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

void StandardPaths::initializeInTemp(const std::string &appName) {
  if (self != nullptr)
    return;

  assert(destroyed != true && "StandardPaths instance was already destroyed");

  static std::once_flag flag;
  std::call_once(flag, [appName]() {
    static StandardPaths s;
    StandardPaths::self = &s;
    s.initializeInTempPrivate(appName);
  });
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
  checkAppNamePrivate(appName);

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

  makeSureFoldersExist();
}

void StandardPaths::initializeInTempPrivate(const std::string &appName) {
  checkAppNamePrivate(appName);

  auto basePath = fs::temp_directory_path() / appName;
  configPath = basePath / "config";
  dataPath = basePath / "data";
  this->tempPath = basePath / "temp";

  makeSureFoldersExist();
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

void StandardPaths::checkAppNamePrivate(const std::string &appName) {}

void StandardPaths::makeSureFoldersExist() {
  std::error_code ec;
  fs::create_directories(getConfigPathPrivate(), ec);
  if (ec) {
    std::cerr << "Failed to create config directory: " << getConfigPathPrivate()
              << ", error: " << ec.message() << std::endl;
  }
  ec.clear();
  fs::create_directories(getDataPathPrivate(), ec);
  if (ec) {
    std::cerr << "Failed to create data directory: " << getDataPathPrivate()
              << ", error: " << ec.message() << std::endl;
  }
  ec.clear();
  fs::remove_all(getTempPathPrivate(), ec);
  if (ec) {
    std::cerr << "Failed to remove temp directory: " << getTempPathPrivate()
              << ", error: " << ec.message() << std::endl;
  }
  ec.clear();
  fs::create_directories(getTempPathPrivate(), ec);
  if (ec) {
    std::cerr << "Failed to create temp directory: " << getTempPathPrivate()
              << ", error: " << ec.message() << std::endl;
  }
}

fs::path StandardPaths::getConfigPathPrivate() { return configPath; }

fs::path StandardPaths::getDataPathPrivate() { return dataPath; }

fs::path StandardPaths::getTempPathPrivate() { return tempPath; }

std::string StandardPaths::getEnvOrEmpty(const char *name) {
  const char *v = std::getenv(name);
  return v ? std::string(v) : std::string();
}

} // namespace synqueen
