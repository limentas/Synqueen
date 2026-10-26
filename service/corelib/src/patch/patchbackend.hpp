#pragma once

#include "command.hpp"
#include "ipatchprovider.hpp"
#include "utils/corralheader.hpp"
#include "utils/uvheader.hpp"

#include <filesystem>
#include <string>

namespace synqueen::patch {

class PatchBackend : public IPatchProvider {
public:
  virtual ~PatchBackend() = default;

  virtual corral::Task<void> shutdown() = 0;

  virtual corral::Task<InitRepoResult>
  initRepoFolder(const std::filesystem::path &folderPath);

protected:
  virtual corral::Task<InitRepoResult>
  initRepoFolderImpl(const std::filesystem::path &folderPath) = 0;
};

PatchBackend *createPatchBackend(uv_loop_t &loop, corral::Nursery &nursery);

} // namespace synqueen::patch
