#pragma once

#include <filesystem>
#include <list>
#include <string>

#include "cloud/cloudtypes.hpp"
#include "utils/corralheader.hpp"

namespace synqueen::cloud {

class ICloudGate {
public:
  virtual ~ICloudGate() = default;

  virtual corral::Task<FileDetailsList>
  listPatchFiles(const CloudDestinationConfig &config) = 0;

  virtual corral::Task<void>
  uploadFiles(const CloudDestinationConfig &config,
              const std::list<std::filesystem::path> &files) = 0;

  virtual corral::Task<void>
  downloadFiles(const CloudDestinationConfig &config,
                const std::list<std::string> &files,
                const std::filesystem::path &destination) = 0;
};

} // namespace synqueen::cloud
