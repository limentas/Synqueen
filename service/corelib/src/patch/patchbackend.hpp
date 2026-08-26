#pragma once

#include "command.hpp"
#include "patchexchange.hpp"
#include "utils/corralheader.hpp"

#include <functional>

namespace synqueen {

class PatchBackend {
public:
  virtual ~PatchBackend() = default;

  virtual corral::Task<void> shutdown() = 0;

  virtual corral::Task<patch::LocalStateResult>
  checkLocalState(const std::string &folderPath) = 0;

  virtual corral::Task<patch::PreparePatchResult>
  preparePatch(const std::string &folderPath) = 0;
};

PatchBackend *createPatchBackend(uv_loop_t *loop);

} // namespace synqueen
