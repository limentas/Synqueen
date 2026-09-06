#pragma once

#include "command.hpp"
#include "ipatchprovider.hpp"
#include "utils/corralheader.hpp"

#include <filesystem>
#include <string>
#include <uv.h>

namespace synqueen {

class PatchBackend : public IPatchProvider {
public:
  virtual ~PatchBackend() = default;

  virtual corral::Task<void> shutdown() = 0;

  virtual corral::Task<void>
  initRepoFolder(const std::filesystem::path &folderPath) = 0;
};

PatchBackend *createPatchBackend(uv_loop_t *loop);

} // namespace synqueen
