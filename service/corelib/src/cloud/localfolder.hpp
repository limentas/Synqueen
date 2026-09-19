#pragma once

#include "icloudgate.hpp"

#include <filesystem>

namespace synqueen {

class LocalFolder : public synqueen::ICloudGate {
public:
  LocalFolder(std::filesystem::path path);
  virtual ~LocalFolder() = default;

  virtual synqueen::FileDetailsList
  listPatchFiles(synqueen::CloudConfig &config,
                 const std::string &path) override;

private:
  std::filesystem::path folderPath;
};

} // namespace synqueen
