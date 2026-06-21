#include "standardpaths.hpp"

#include <cassert>

#include <uv.h>

StandardPaths *StandardPaths::self = nullptr;
bool StandardPaths::destroyed = false;

void StandardPaths::initialize() { getInstance()->initializePrivate(); }

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

void StandardPaths::initializePrivate() {
  static const int maxPathLength = 1024;
  configPath.resize(maxPathLength);
  dataPath.resize(maxPathLength);
  size_t size = maxPathLength;
  uv_os_homedir(configPath.data(), &size);
  configPath.resize(size);
  size = maxPathLength;
  uv_os_homedir(dataPath.data(), &size);
  dataPath.resize(size);
}

std::string StandardPaths::getConfigPathPrivate() { return configPath; }

std::string StandardPaths::getDataPathPrivate() { return dataPath; }
