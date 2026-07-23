#pragma once

#include <future>
#include <list>
#include <spdlog/spdlog.h>
#include <string>
#include <uv.h>

namespace synqueen {

class PatchExchange {
public:
  explicit PatchExchange(uv_loop_t *l);
  ~PatchExchange() = default;

  struct BaseResult {
    bool ok;
    std::string errorMessage;
  };

  struct LocalStateResult : public BaseResult {
    bool initialized;
    bool hasUncommittedChanges;
  };

  struct PreparePatchResult : public BaseResult {
    std::list<std::string> patches;
  };

  std::future<LocalStateResult> checkLocalState(const std::string &folderPath);
  std::future<PreparePatchResult> preparePatch(const std::string &folderPath);

  void ensureFolderInitialized(const std::string &folderPath);

private:
  void checkStatus(const std::string &folderPath);

private:
  uv_loop_t *loop = nullptr;
};

} // namespace synqueen
