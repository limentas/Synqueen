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

  virtual corral::Task<void> shutdown() override;

  virtual corral::Task<patch::LocalStateResult>
  checkLocalState(const std::string &folderPath) override;

  virtual corral::Task<patch::PreparePatchResult>
  preparePatch(const std::string &folderPath) override;

private:
  HgProcess hgProcess;

  std::string repoFolder;
};

} // namespace synqueen
