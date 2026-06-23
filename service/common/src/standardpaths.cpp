#include "standardpaths.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <uv.h>

#include <platform.hpp>

namespace synqueen {

StandardPaths *StandardPaths::self = nullptr;
bool StandardPaths::destroyed = false;

void StandardPaths::initialize(const std::string &appName) {
  getInstance()->initializePrivate(appName);
}

std::string StandardPaths::getConfigPath() {
  return getInstance()->getConfigPathPrivate();
}

std::string StandardPaths::getDataPath() {
  return getInstance()->getDataPathPrivate();
}

StandardPaths::StandardPaths() {}

StandardPaths::~StandardPaths() {
  destroyed = true;
  self = nullptr;
}

StandardPaths *StandardPaths::getInstance() {
  if (self)
    return self;

  // Make sure that the instance was not destroyed
  assert(destroyed != true);
  if (destroyed)
    return nullptr;

  static StandardPaths s;
  self = &s;
  return self;
}

void StandardPaths::initializePrivate(const std::string &appName) {
  namespace fs = std::filesystem;
  const auto home = requestHomePathPrivate();

#if defined(SQ_OS_WINDOWS)
  std::string roaming = getEnvOrEmpty("APPDATA");
  std::string local = getEnvOrEmpty("LOCALAPPDATA");

  if (roaming.empty() && !home.empty())
    roaming = (fs::path(home) / "AppData" / "Roaming").string();
  if (local.empty() && !home.empty())
    local = (fs::path(home) / "AppData" / "Local").string();

  configPath = roaming.empty() ? (fs::path(".") / appName / "config").string()
                               : (fs::path(roaming) / appName).string();
  dataPath = local.empty() ? (fs::path(".") / appName / "data").string()
                           : (fs::path(local) / appName).string();

#elif defined(SQ_OS_ANDROID)
  std::string base = getEnvOrEmpty("HOME");
  if (base.empty())
    base = "/data/local/tmp";

  configPath = (fs::path(base) / ".config" / appName).string();
  dataPath = (fs::path(base) / ".local" / "share" / appName).string();
#else
  std::string xdgConfig = getEnvOrEmpty("XDG_CONFIG_HOME");
  std::string xdgData = getEnvOrEmpty("XDG_DATA_HOME");

  if (xdgConfig.empty())
    xdgConfig =
        home.empty() ? std::string(".") : (fs::path(home) / ".config").string();
  if (xdgData.empty())
    xdgData = home.empty() ? std::string(".")
                           : (fs::path(home) / ".local" / "share").string();

  configPath = (fs::path(xdgConfig) / appName).string();
  dataPath = (fs::path(xdgData) / appName).string();
#endif
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
}

std::string StandardPaths::requestHomePathPrivate() {
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
  return std::string();
}

std::string StandardPaths::getConfigPathPrivate() { return configPath; }

std::string StandardPaths::getDataPathPrivate() { return dataPath; }

std::string StandardPaths::getEnvOrEmpty(const char *name) {
  const char *v = std::getenv(name);
  return v ? std::string(v) : std::string();
}

} // namespace synqueen
