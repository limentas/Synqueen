#pragma once

#include "command.hpp"
#include "utils/corralheader.hpp"

#include <string>

namespace synqueen {

class IPatchProvider {
public:
  virtual ~IPatchProvider() = default;

  virtual corral::Task<patch::LocalStateResult>
  checkLocalState(const std::string &folderPath) = 0;

  virtual corral::Task<patch::PreparePatchResult>
  preparePatch(const std::string &folderPath) = 0;
};

} // namespace synqueen
