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
    co_return result;
  }

  if (cmdResult.resultCode != 0) {
    result.ok = false;
    result.errorMessage = "Failed to check local state. Exit code: " +
                          to_string(cmdResult.resultCode) +
                          "\n\tStdout:" + cmdResult.output +
                          "\n\tStderr:" + cmdResult.error;
    co_return result;
  }

  result.initialized = true;
  result.errorMessage = "";
  result.hasUncommittedChanges =
      (cmdResult.output.find("commit: (clean)") == string::npos);
  result.hasConflicts = (cmdResult.output.find("parent: (multiple)") !=
                         string::npos); // This is a heuristic; adjust as needed

  auto idResult = co_await hgProcess.runCommand(
      {"id", "-i", "--debug", "--repository", folderPath});
  if (idResult.resultCode != 0) {
    result.ok = false;
    result.errorMessage = "Failed to get last commit hash. Exit code: " +
                          to_string(idResult.resultCode) +
                          "\n\tStdout:" + idResult.output +
                          "\n\tStderr:" + idResult.error;
    co_return result;
  }
  // Remove the trailing '+' and '\n' if present, which indicates uncommitted
  // changes
  if (!idResult.output.empty() && idResult.output.back() == '\n') {
    idResult.output.pop_back();
  }
  if (!idResult.output.empty() && idResult.output.back() == '+') {
    idResult.output.pop_back();
  }
  result.lastCommitHash = idResult.output;
  result.ok = true;
  co_return result;
}

corral::Task<patch::PreparePatchResult>
HgBackend::preparePatch(const string &folderPath) {
  repoFolder = folderPath;
  // TODO: Implement preparePatch logic
  co_return patch::PreparePatchResult{};
}

} // namespace synqueen
