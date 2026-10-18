#include "foldermanager.hpp"

#include "utils/logger.hpp"
#include "utils/scopeguard.hpp"

#include <cassert>

namespace fs = std::filesystem;

namespace synqueen {
using namespace patch;

void FolderManager::initialize(corral::Nursery &nursery) {
  this->nursery = &nursery;
  loadFolderState();
  patchStorage.getIncomingNotifier().addSyncHandler(
      [this]() { synchronize(); });
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
            // TODO: actually we need to check against last included commit. It
            // may happen that everything was committed but we failed to move
            // the patch file.
            auto preparePatchResult = co_await patchProvider.preparePatch(
                repoPath, lastOutgoingCommit);

            if (preparePatchResult.lastIncludedCommitHash.empty()) {
              // How is this possible? hasUncommittedChanges is wrong?
              throw std::runtime_error("Failed to prepare patch: last included "
                                       "outgoing commit hash is empty");
            }

            patchStorage.movePatchToOutgoing(
                preparePatchResult.patchFilePath, nextOutgoingPatchIndex,
                preparePatchResult.lastIncludedCommitHash);
            ++nextOutgoingPatchIndex;
            lastOutgoingCommit = preparePatchResult.lastIncludedCommitHash;
          }
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Exception while checking local state for folder {}: {}",
                       repoPath, e.what());
          co_return;
        }

        // Check for the patches to apply
        try {
          auto patchesToApply =
              std::views::values(patchStorage.listPatchesToApply());
          if (!patchesToApply.empty()) {
            auto applyPatchesResult = co_await patchProvider.applyPatches(
                repoPath, std::list<std::filesystem::path>(
                              patchesToApply.begin(), patchesToApply.end()));

            for (const auto &patchPath : patchesToApply) {
              patchStorage.stagePatchAsApplied(patchPath);
            }
          }
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Exception while applying patches for folder {}: {}",
                       repoPath, e.what());
        }
      },
      std::cref(repoPath), std::ref(patchProvider));
}

void FolderManager::loadFolderState() {
  auto outgoingPatches = patchStorage.listOutgoingPatches();
  if (outgoingPatches.empty()) {
    nextOutgoingPatchIndex = 1;
    lastOutgoingCommit.clear();
  } else {
    auto maxIndex = 0;
    for (auto it = outgoingPatches.begin(); it != outgoingPatches.end(); ++it) {
      if (it->second.index <= maxIndex)
        continue;
      lastOutgoingCommit = it->second.lastCommit;
      maxIndex = it->second.index;
    }
    nextOutgoingPatchIndex = maxIndex + 1;
  }
}

} // namespace synqueen
