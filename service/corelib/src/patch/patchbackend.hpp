#pragma once

#include "command.hpp"
#include "ipatchprovider.hpp"
#include "utils/corralheader.hpp"

#include <filesystem>
#include <string>
#include <uv.h>

namespace synqueen::patch {

class PatchBackend : public IPatchProvider {
public:
  virtual ~PatchBackend() = default;

  virtual corral::Task<void> shutdown() = 0;

  virtual corral::Task<InitRepoResult>
  initRepoFolder(const std::filesystem::path &folderPath);

  virtual corral::Task<ApplyPatchResult>
  applyPatches(const std::filesystem::path &folderPath,
               const std::list<std::filesystem::path> &patchFiles) = 0;

protected:
  virtual corral::Task<InitRepoResult>
  initRepoFolderImpl(const std::filesystem::path &folderPath) = 0;
};

PatchBackend *createPatchBackend(uv_loop_t *loop);

} // namespace synqueen::patch
