#include "utils/systeminfo.hpp"

#include <cassert>
#include <mutex>
#define NOMINMAX
#include <uv.h>

#include <spdlog/spdlog.h>

namespace synqueen {

SystemInfo *SystemInfo::self = nullptr;
bool SystemInfo::destroyed = false;

std::string SystemInfo::getOSName() { return getInstance()->getOSNameImpl(); }
std::string SystemInfo::getOSVersion() {
  return getInstance()->getOSVersionImpl();
}
std::string SystemInfo::getArchitecture() {
  return getInstance()->getArchitectureImpl();
}
std::string SystemInfo::getHostname() {
  return getInstance()->getHostnameImpl();
}
std::string SystemInfo::getSysName() { return getInstance()->getSysNameImpl(); }

SystemInfo::~SystemInfo() {
  destroyed = true;
  self = nullptr;
}

SystemInfo *SystemInfo::getInstance() {
  if (SystemInfo::self != nullptr)
    return SystemInfo::self;

  static std::once_flag flag;
  std::call_once(flag, []() {
    static SystemInfo s;
    SystemInfo::self = &s;
    s.initializePrivate();
  });
  return SystemInfo::self;
}

void SystemInfo::initializePrivate() {
  uv_utsname_t utsname;
  auto res = uv_os_uname(&utsname);
  if (res != 0) {
    SPDLOG_ERROR("Failed to get OS uname. Error: {}", uv_strerror(res));
  } else {
    osName = utsname.version;
    osVersion = utsname.release;
    architecture = utsname.machine;
    sysName = utsname.sysname;
  }

  size_t size = 1024;
  hostname.resize(size);
  res = uv_os_gethostname(hostname.data(), &size);
  if (res != 0) {
    SPDLOG_ERROR("Failed to get hostname. Error: {}", uv_strerror(res));
    hostname = "Unknown Hostname";
  } else {
    hostname.resize(size);
  }
}

} // namespace synqueen
