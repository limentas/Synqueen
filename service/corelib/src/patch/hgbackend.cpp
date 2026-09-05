#include "hgbackend.hpp"

#include "const.hpp"
#include "hgprotocol.hpp"
#include "utils/standardpaths.hpp"
#include "utils/utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

using namespace std;
namespace fs = std::filesystem;

namespace synqueen {

using namespace patch;

const char *HgBackend::hgRcTemplate = "[ui]\n"
                                      "username = Synqueen <s@slebe.dev>\n"
                                      "ignore.other = @appdata@/.hgignore\n";
const char *HgBackend::ignoreFileTemplate =
    "# Synqueen application-wide ignore file\n"
    "syntax: glob\n"
    "@subfolder_name@/**\n";

HgBackend::HgBackend(uv_loop_t *l)
    : rcFileContent(replaceAll(HgBackend::hgRcTemplate, "@appdata@",
                               StandardPaths::getDataPath().string())),
      ignoreFileContent(replaceAll(HgBackend::ignoreFileTemplate,
                                   "@subfolder_name@", mySubfolderName)),
      hgProcess(l) {
  // Create application-wide .hgignore file with the specified content
  fs::path ignoreFilePath =
      fs::path(StandardPaths::getDataPath()) / ".hgignore";
  if (!fs::exists(ignoreFilePath)) {
    fs::create_directories(ignoreFilePath.parent_path());
    std::ofstream ignoreFile(ignoreFilePath);
    if (!ignoreFile.is_open()) {
      throw std::runtime_error("Failed to create ignore file at: " +
                               ignoreFilePath.string());
    }
    ignoreFile << ignoreFileContent;
    ignoreFile.close();
  }
}

corral::Task<void> HgBackend::shutdown() { return hgProcess.shutdown(); }

corral::Task<LocalStateResult>
HgBackend::checkLocalState(const string &folderPath) {
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
  result.hasConflicts = (cmdResult.output.find("(merge)") != string::npos);

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
  // TODO: Implement preparePatch logic
  co_return patch::PreparePatchResult{};
}

corral::Task<void> HgBackend::initRepoFolder(const std::string &folderPath) {
  auto initResult = co_await hgProcess.runCommand({"init", folderPath});
  if (initResult.resultCode != 0) {
    throw std::runtime_error("Failed to initialize repository. Exit code: " +
                             std::to_string(initResult.resultCode) +
                             "\n\tStdout:" + initResult.output +
                             "\n\tStderr:" + initResult.error);
  }

  // Create .hg/hgrc file with the specified content
  fs::path hgRcPath = fs::path(folderPath) / ".hg" / "hgrc";
  fs::create_directories(hgRcPath.parent_path());
  std::ofstream hgRcFile(hgRcPath);
  if (!hgRcFile.is_open()) {
    throw std::runtime_error("Failed to create hgrc file at: " +
                             hgRcPath.string());
  }
  hgRcFile << rcFileContent;
  hgRcFile.close();

  co_return;
}

} // namespace synqueen
