#include "hgbackend.hpp"

#include "hgprotocol.hpp"

#include <algorithm>
#include <signal.h>
#include <spdlog/spdlog.h>

using namespace std;

namespace synqueen {

using namespace patch;

synqueen::HgBackend::HgBackend(uv_loop_t *l) : hgProcess(l) {}

corral::Task<void> HgBackend::shutdown() { return hgProcess.shutdown(); }

corral::Task<LocalStateResult>
HgBackend::checkLocalState(const string &folderPath) {
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
  co_return result;
}

corral::Task<patch::PreparePatchResult>
HgBackend::preparePatch(const string &folderPath) {
  repoFolder = folderPath;
  // TODO: Implement preparePatch logic
  co_return patch::PreparePatchResult{};
}

} // namespace synqueen
