#pragma once

#include "patchbackend.hpp"

#include "command.hpp"
#include "hgprocess.hpp"
#include "hgprotocol.hpp"

#include <uv.h>

namespace synqueen {

class HgBackend : public PatchBackend {
public:
  HgBackend(uv_loop_t *l);
  virtual ~HgBackend() = default;

  virtual corral::Task<patch::LocalStateResult>
  checkLocalState(const std::string &folderPath,
                  const patch::LocalStateCallbackPtr &callback) override;

  virtual void
  preparePatch(const std::string &folderPath,
               const patch::PreparePatchCallbackPtr &callback) override;

private:
  HgProcess hgProcess;

  std::string repoFolder;
};

} // namespace synqueen
