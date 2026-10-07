#include "hgbackend.hpp"

#include "const.hpp"
#include "hgprotocol.hpp"
#include "utils/exceptions.hpp"
#include "utils/standardpaths.hpp"
#include "utils/systeminfo.hpp"
#include "utils/utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

using namespace std;
using namespace synqueen::utils;

namespace fs = std::filesystem;

namespace synqueen::patch {

const char *HgBackend::hgRcTemplate = "# Synqueen Mercurial configuration\n"
                                      "[ui]\n"
                                      "username = Synqueen on @hostname@\n"
                                      "ignore.other = @appdata@/.hgignore\n";
const char *HgBackend::ignoreFileTemplate =
    "# Synqueen application-wide ignore file\n"
    "syntax: glob\n"
    "@subfolder_name@/**\n";

HgBackend::HgBackend(uv_loop_t *l)
    : loop(l), rcFileContent(replaceAll(
                   HgBackend::hgRcTemplate,
                   {{"@hostname@", SystemInfo::getHostname()},
                    {"@appdata@", StandardPaths::getDataPath().string()}})),
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
HgBackend::checkLocalState(const fs::path &folderPath) {
  const auto repoPath = folderPath.string();
  auto cmdResult =
      co_await hgProcess.runCommand({"summary", "--repository", repoPath});

  LocalStateResult result;
  // TODO: hg always returns 255 if a fatal error occurs, make the check more
  // robust
  if (cmdResult.resultCode == 255) {
    // This means the repository is not initialized or the folder not found
    result.initialized = false;
    result.hasUncommittedChanges = false;
    co_return result;
  }

  if (cmdResult.resultCode != 0) {
    throw std::runtime_error("Failed to check local state. Exit code: " +
                             to_string(cmdResult.resultCode) +
                             "\n\tStdout:" + cmdResult.output +
                             "\n\tStderr:" + cmdResult.error);
  }

  result.initialized = true;
  result.hasUncommittedChanges =
      (cmdResult.output.find("commit: (clean)") == string::npos);
  result.hasConflicts = (cmdResult.output.find("(merge)") != string::npos);

  // Remove the trailing '+' and '\n' if present, which indicates uncommitted
  // changes
  result.lastCommitHash = co_await getLastCommitHash(folderPath);
  co_return result;
}

corral::Task<PreparePatchResult>
HgBackend::preparePatch(const fs::path &folderPath,
                        const std::string &fromCommitHash) {
  auto lastCommitHash = co_await addAndCommit(folderPath, "Initial commit");
  auto currentCommitHash = co_await getLastCommitHash(folderPath);
  if (currentCommitHash == fromCommitHash) {
    // No new changes to prepare
    co_return PreparePatchResult{};
  }
  auto tempFile = co_await createTemporaryFile("hg_bundle_XXXXXX", loop);
  std::list<std::string> args = {"bundle", "--repository", folderPath.string(),
                                 "--type", "zstd-v2"};
  if (!fromCommitHash.empty()) {
    args.push_back("--base");
    args.push_back(fromCommitHash);
  } else {
    args.push_back("--all");
  }
  args.push_back(tempFile);

  auto bundleResult = co_await hgProcess.runCommand(args);
  if (bundleResult.resultCode != 0) {
    throw std::runtime_error("Failed to create patch bundle. Exit code: " +
                             to_string(bundleResult.resultCode) +
                             "\n\tStdout:" + bundleResult.output +
                             "\n\tStderr:" + bundleResult.error);
  }

  co_return PreparePatchResult{.patchFilePath = fs::path(tempFile),
                               .lastIncludedCommitHash = currentCommitHash};
}

corral::Task<ApplyPatchResult>
HgBackend::applyPatches(const std::filesystem::path &folderPath,
                        const std::list<std::filesystem::path> &patchFiles) {
  co_await addAndCommit(folderPath, "TODO: commit message");
  std::list<std::string> args = {"unbundle", "--repository",
                                 folderPath.string()};
  // Mercurial applies changesets in the right order based on parent information
  // No need to sort the files somehow
  for (const auto &patchFile : patchFiles) {
    args.push_back(patchFile.string());
  }
  auto applyResult = co_await hgProcess.runCommand(args);
  if (applyResult.resultCode != 0) {
    throw std::runtime_error("Failed to apply patch. Exit code: " +
                             std::to_string(applyResult.resultCode) +
                             "\n\tStdout:" + applyResult.output +
                             "\n\tStderr:" + applyResult.error);
  }
  // Patch applied successfully, now we need to update the working directory
  auto updateResult = co_await hgProcess.runCommand(
      {"update", "--repository", folderPath.string()});
  if (updateResult.resultCode != 0) {
    throw std::runtime_error("Failed to update working directory after "
                             "applying patch. Exit code: " +
                             std::to_string(updateResult.resultCode) +
                             "\n\tStdout:" + updateResult.output +
                             "\n\tStderr:" + updateResult.error);
  }

  // TODO: handle conflicts

  co_return ApplyPatchResult{.lastIncludedCommitHash =
                                 co_await getLastCommitHash(folderPath),
                             .hasConflicts = false};
}

corral::Task<InitRepoResult>
HgBackend::initRepoFolderImpl(const fs::path &folderPath) {
  if (fs::exists(folderPath / ".hg")) {
    throw synqueen::SqAlreadyUsed(
        "The specified folder is already a Mercurial repository: " +
        folderPath.string());
    // TODO: Implement handling for already used repository case if needed
  }

  // TODO: check for permissions

  auto initResult =
      co_await hgProcess.runCommand({"init", folderPath.string()});
  if (initResult.resultCode != 0) {
    throw std::runtime_error("Failed to initialize repository. Exit code: " +
                             std::to_string(initResult.resultCode) +
                             "\n\tStdout:" + initResult.output +
                             "\n\tStderr:" + initResult.error);
  }

  // Create .hg/hgrc file with the specified content
  fs::path hgRcPath = fs::path(folderPath) / ".hg" / "hgrc";
  std::error_code ec;
  fs::create_directories(hgRcPath.parent_path(), ec);
  if (ec) {
    throw std::runtime_error("Failed to create directories for hgrc file at: " +
                             hgRcPath.parent_path().string() +
                             ". Error: " + ec.message());
  }
  std::ofstream hgRcFile(hgRcPath);
  if (!hgRcFile.is_open()) {
    throw std::runtime_error("Failed to create hgrc file at: " +
                             hgRcPath.string());
  }
  hgRcFile << rcFileContent;
  hgRcFile.close();

  auto hash = co_await addAndCommit(folderPath, "TODO: a message");
  co_return InitRepoResult{.lastCommitHash = hash};
}

corral::Task<std::string>
HgBackend::getLastCommitHash(const std::filesystem::path &folderPath) {
  auto idResult = co_await hgProcess.runCommand(
      {"id", "-i", "--debug", "--repository", folderPath.string()});
  if (idResult.resultCode != 0) {
    throw std::runtime_error("Failed to get last commit hash. Exit code: " +
                             std::to_string(idResult.resultCode) +
                             "\n\tStdout:" + idResult.output +
                             "\n\tStderr:" + idResult.error);
  }
  // Remove the trailing '+' and '\n' if present, which indicates uncommitted
  // changes
  auto lastCommitHash = removeAtEnd(idResult.output, "+\n");
  // If the last commit hash is all zeros, it indicates no commits have been
  // made yet.
  if (lastCommitHash == "0000000000000000000000000000000000000000") {
    lastCommitHash.clear();
  }
  co_return lastCommitHash;
}

corral::Task<std::string>
HgBackend::addAndCommit(const std::filesystem::path &folderPath,
                        const std::string &message) {
  // Add untracked files and remove missing files
  auto addResult = co_await hgProcess.runCommand(
      {"addremove", "--repository", folderPath.string()});
  if (addResult.resultCode != 0) {
    throw std::runtime_error("Failed to add untracked files. Exit code: " +
                             std::to_string(addResult.resultCode) +
                             "\n\tStdout:" + addResult.output +
                             "\n\tStderr:" + addResult.error);
  }
  auto commitResult = co_await hgProcess.runCommand(
      {"commit", "--repository", folderPath.string(), "-m", message});
  // result code = 1 when there is nothing to commit
  if (commitResult.resultCode != 0 && commitResult.resultCode != 1) {
    throw std::runtime_error("Failed to commit changes. Exit code: " +
                             std::to_string(commitResult.resultCode) +
                             "\n\tStdout:" + commitResult.output +
                             "\n\tStderr:" + commitResult.error);
  }
  co_return co_await getLastCommitHash(folderPath);
}

} // namespace synqueen::patch
