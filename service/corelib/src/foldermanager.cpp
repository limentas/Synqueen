#include "foldermanager.hpp"

#include "utils/logger.hpp"
#include "utils/scopeguard.hpp"

#include <cassert>

namespace fs = std::filesystem;

namespace synqueen {
using namespace patch;

void FolderManager::loadState(corral::Nursery &nursery) {
  this->nursery = &nursery;
  loadFolderState();
}

void FolderManager::setupState(corral::Nursery &nursery) {
  this->nursery = &nursery;
  createDataPaths();
}

void FolderManager::synchronize() {
  assert(nursery != nullptr);
  if (isSynchronizing)
    return;
  isSynchronizing = true;
  nursery->start(
      [this](const fs::path &repoPath,
             IPatchProvider &patchProvider) -> corral::Task<void> {
        auto resetFlag = ScopeGuard([this]() { isSynchronizing = false; });
        try {
          auto result = co_await patchProvider.checkLocalState(repoPath);
          SPDLOG_DEBUG(
              "Local state for folder {}: initialized={}, "
              "hasUncommittedChanges={}, hasConflicts={}, lastCommitHash={}",
              repoPath, result.initialized, result.hasUncommittedChanges,
              result.hasConflicts, result.lastCommitHash);

          if (result.hasConflicts) {
            // TODO: do we need to do something about conflicts?
          }
          if (result.hasUncommittedChanges) {
            // What was the last commit hash that is already bundled?
            auto preparePatchResult = co_await patchProvider.preparePatch(
                repoPath, lastIncludedCommit);

            if (preparePatchResult.lastIncludedCommitHash.empty()) {
              // How is this possible? hasUncommittedChanges is wrong?
              throw std::runtime_error("Failed to prepare patch: last included "
                                       "commit hash is empty");
            }

            addOutgoingPatch(preparePatchResult.patchFilePath,
                             preparePatchResult.lastIncludedCommitHash);
            lastIncludedCommit = preparePatchResult.lastIncludedCommitHash;
          }
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Exception while checking local state for folder {}: {}",
                       repoPath, e.what());
          co_return;
        }
      },
      std::cref(repoPath), std::ref(patchProvider));
}

void FolderManager::loadFolderState() {
  incomingPatches = loadPatches(incomingPatchesPath);
  outgoingPatches = loadPatches(outgoingPatchesPath);
  auto maxIncomingPatchIndex = 0, maxOutgoingPatchIndex = 0;
  std::string lastIncomingCommit, lastOutgoingCommit;
  if (!incomingPatches.empty()) {
    parsePatchName(incomingPatches.back(), maxIncomingPatchIndex,
                   lastIncomingCommit);
  }
  if (!outgoingPatches.empty()) {
    parsePatchName(outgoingPatches.back(), maxOutgoingPatchIndex,
                   lastOutgoingCommit);
  }

  nextOutgoingPatchIndex =
      std::max(maxIncomingPatchIndex, maxOutgoingPatchIndex) + 1;

  if (maxIncomingPatchIndex > maxOutgoingPatchIndex) {
    lastIncludedCommit = lastIncomingCommit;
  } else {
    // Outgoing patch takes precedence if its index is equal to incoming patch
    // index. In practice this means a simultaneous edit (with or without
    // conflicts)
    lastIncludedCommit = lastOutgoingCommit;
  }
}

void FolderManager::createDataPaths() {
  if (fs::exists(dataPath)) {
    // TODO: export existing data if necessary
    return;
  }

  std::error_code ec;
  fs::create_directories(dataPath, ec);
  if (ec) {
    throw std::runtime_error("Failed to create data directory at: " +
                             dataPath.string() + ". Error: " + ec.message());
  }
  fs::create_directories(incomingPatchesPath, ec);
  if (ec) {
    throw std::runtime_error(
        "Failed to create incoming patches directory at: " +
        incomingPatchesPath.string() + ". Error: " + ec.message());
  }
  fs::create_directories(outgoingPatchesPath, ec);
  if (ec) {
    throw std::runtime_error(
        "Failed to create outgoing patches directory at: " +
        outgoingPatchesPath.string() + ". Error: " + ec.message());
  }
}

std::list<std::string>
FolderManager::loadPatches(const std::filesystem::path &path) {
  std::list<std::string> patches;
  fs::directory_iterator it(path);
  for (const auto &entry : it) {
    if (entry.is_regular_file() && entry.path().extension() == ".patch") {
      patches.push_back(entry.path().filename().string());
    }
  }
  // TODO: At this moment lexicographical order will not work properly for patch
  // indices with different number of digits. Consider using std::set here for
  // faster lookups. And then determine max patch index passing through it.
  patches.sort();
  return patches;
}

void FolderManager::addOutgoingPatch(const std::filesystem::path &patchFile,
                                     const std::string &lastIncludedCommit) {
  // Move the patch file to the outgoing folder
  std::error_code ec;
  auto newPath = outgoingPatchesPath / (std::to_string(nextOutgoingPatchIndex) +
                                        "_" + lastIncludedCommit + ".patch");
  fs::rename(patchFile, newPath, ec);
  if (ec) {
    throw std::runtime_error("Failed to move patch file " + patchFile.string() +
                             " to outgoing patches directory at: " +
                             newPath.string() + ". Error: " + ec.message());
  }
  ++nextOutgoingPatchIndex;
}

void FolderManager::parsePatchName(const std::string &patchName,
                                   int &patchIndex, std::string &commit) {
  auto underscorePos = patchName.find('_');
  if (underscorePos == std::string::npos) {
    throw std::runtime_error("Invalid patch name format: " + patchName);
  }
  auto dotPos = patchName.find('.');
  if (dotPos == std::string::npos) {
    throw std::runtime_error("Invalid patch name format: " + patchName);
  }
  patchIndex = std::stoi(patchName.substr(0, underscorePos));
  commit = patchName.substr(underscorePos + 1, dotPos - underscorePos - 1);
}

} // namespace synqueen
