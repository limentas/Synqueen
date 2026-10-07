#pragma once

#include "patchbackend.hpp"

#include "command.hpp"
#include "hgprocess.hpp"
#include "hgprotocol.hpp"

#include <filesystem>
#include <uv.h>

namespace synqueen::patch {

class HgBackend : public PatchBackend {
public:
  HgBackend(uv_loop_t *l);
  virtual ~HgBackend() = default;

  virtual corral::Task<void> shutdown() override;

  virtual corral::Task<LocalStateResult>
  checkLocalState(const std::filesystem::path &folderPath) override;

  virtual corral::Task<PreparePatchResult>
  preparePatch(const std::filesystem::path &folderPath,
               const std::string &fromCommitHash) override;

  virtual corral::Task<ApplyPatchResult>
  applyPatches(const std::filesystem::path &folderPath,
               const std::list<std::filesystem::path> &patchFiles) override;

protected:
  virtual corral::Task<InitRepoResult>
  initRepoFolderImpl(const std::filesystem::path &folderPath) override;

  corral::Task<std::string>
  getLastCommitHash(const std::filesystem::path &folderPath);

  corral::Task<std::string>
  addAndCommit(const std::filesystem::path &folderPath,
               const std::string &message);

private:
  static const char *hgRcTemplate;
  static const char *ignoreFileTemplate;

  uv_loop_t *loop;
  std::string rcFileContent;
  std::string ignoreFileContent;
  HgProcess hgProcess;
};

} // namespace synqueen::patch
