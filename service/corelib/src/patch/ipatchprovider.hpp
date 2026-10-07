#pragma once

#include "command.hpp"
#include "utils/corralheader.hpp"

#include <filesystem>
#include <string>

namespace synqueen::patch {

class IPatchProvider {
public:
  virtual ~IPatchProvider() = default;

  virtual corral::Task<LocalStateResult>
  checkLocalState(const std::filesystem::path &folderPath) = 0;

  virtual corral::Task<PreparePatchResult>
  preparePatch(const std::filesystem::path &folderPath,
               const std::string &fromCommitHash) = 0;
};

} // namespace synqueen::patch
