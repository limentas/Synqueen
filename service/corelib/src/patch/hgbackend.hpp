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

  virtual corral::Task<void>
  initRepoFolder(const std::string &folderPath) override;

private:
  static const char *hgRcTemplate;
  static const char *ignoreFileTemplate;
  std::string rcFileContent;
  std::string ignoreFileContent;
  HgProcess hgProcess;
};

} // namespace synqueen
