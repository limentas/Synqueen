#pragma once

#include "command.hpp"
#include "patchexchange.hpp"

#include <functional>

namespace synqueen {

class PatchBackend {
public:
  virtual ~PatchBackend() = default;

  virtual void
  checkLocalState(const std::string &folderPath,
                  const patch::LocalStateCallbackPtr &callback) = 0;

  virtual void preparePatch(const std::string &folderPath,
                            const patch::PreparePatchCallbackPtr &callback) = 0;
};

PatchBackend *createPatchBackend(uv_loop_t *loop);

} // namespace synqueen
