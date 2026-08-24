#include "hgbackend.hpp"

#include "hgprotocol.hpp"

#include <algorithm>
#include <signal.h>
#include <spdlog/spdlog.h>

using namespace std;

namespace synqueen {

using namespace patch;

synqueen::HgBackend::HgBackend(uv_loop_t *l) : hgProcess(l) {}

corral::Task<LocalStateResult>
HgBackend::checkLocalState(const string &folderPath,
                           const LocalStateCallbackPtr &callback) {
  if (!callback) {
    throw std::invalid_argument("callback cannot be null");
  }

  repoFolder = folderPath;
  auto cmdResult =
      co_await hgProcess.runCommand({"summary", "--repository", folderPath});

  LocalStateResult result;
  if (cmdResult.resultCode == 255) {
    // This means the repository is not initialized or the folder not found
    result.ok = true;
    result.initialized = false;
    result.hasUncommittedChanges = false;
  } else {
    result.ok = (cmdResult.resultCode == 0);
    result.initialized = result.ok;
    result.errorMessage = "";
  }
  (*callback)(repoFolder, result);
}

void HgBackend::preparePatch(const string &folderPath,
                             const PreparePatchCallbackPtr &callback) {
  if (!callback) {
    throw std::invalid_argument("callback cannot be null");
  }

  repoFolder = folderPath;
  // TODO: Implement preparePatch logic
}

} // namespace synqueen
