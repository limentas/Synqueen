#pragma once

#include "commandresults.hpp"
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

protected:
  enum class Command { CheckLocalState, PreparePatch };
};

PatchBackend *createPatchBackend(uv_loop_t *loop);

} // namespace synqueen
