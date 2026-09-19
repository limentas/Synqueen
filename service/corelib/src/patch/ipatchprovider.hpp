#pragma once

#include "command.hpp"
#include "utils/corralheader.hpp"

#include <filesystem>
#include <string>

namespace synqueen {

class IPatchProvider {
public:
  virtual ~IPatchProvider() = default;

  virtual corral::Task<patch::LocalStateResult>
  checkLocalState(const std::filesystem::path &folderPath) = 0;
};

} // namespace synqueen
