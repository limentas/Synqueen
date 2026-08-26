#pragma once

#include "utils/corralheader.hpp"
#include <list>
#include <queue>
#include <spdlog/spdlog.h>
#include <string>
#include <uv.h>

#include "command.hpp"

namespace synqueen {

class PatchBackend;

// This is a wrapper around the PatchBackend that queues async requests and do
// calls one by one
class PatchExchange {
public:
  explicit PatchExchange(std::unique_ptr<PatchBackend> backendPtr);
  ~PatchExchange() = default;

  corral::Task<void> shutdown();

  corral::Task<patch::LocalStateResult>
  checkLocalState(const std::string &folderPath);

  corral::Task<patch::PreparePatchResult>
  preparePatch(const std::string &folderPath);

  void ensureFolderInitialized(const std::string &folderPath);

private:
  std::unique_ptr<PatchBackend> backend;
};

} // namespace synqueen
