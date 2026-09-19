#pragma once

#include <string>

namespace synqueen {

class SystemInfo {
public:
  static std::string getOSName();
  static std::string getOSVersion();
  static std::string getArchitecture();
  static std::string getHostname();
  static std::string getSysName();

private:
  SystemInfo() = default;
  ~SystemInfo();

  static SystemInfo *getInstance();

  void initializePrivate();

  inline std::string getOSNameImpl() { return osName; }
  inline std::string getOSVersionImpl() { return osVersion; }
  inline std::string getArchitectureImpl() { return architecture; }
  inline std::string getHostnameImpl() { return hostname; }
  inline std::string getSysNameImpl() { return sysName; }

private:
  static SystemInfo *self;
  static bool destroyed;

  std::string osName;
  std::string osVersion;
  std::string architecture;
  std::string hostname;
  std::string sysName;
};

} // namespace synqueen
