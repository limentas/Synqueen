#include "foldermanager.hpp"

#include "utils/logger.hpp"

#include <cassert>

namespace fs = std::filesystem;

namespace synqueen {

void FolderManager::initialize(corral::Nursery &nursery) {
  this->nursery = &nursery;
}

void FolderManager::check() {
  assert(nursery != nullptr);
  nursery->start(
      [](const fs::path &path,
         IPatchProvider &patchProvider) -> corral::Task<void> {
        auto result = co_await patchProvider.checkLocalState(path);
        if (!result.ok) {
          SPDLOG_ERROR("Failed to check local state for folder {}: {}", path,
                       result.errorMessage);
          co_return;
        }

        SPDLOG_INFO(
            "Local state for folder {}: initialized={}, "
            "hasUncommittedChanges={}, hasConflicts={}, lastCommitHash={}",
            path, result.initialized, result.hasUncommittedChanges,
            result.hasConflicts, result.lastCommitHash);
      },
      std::cref(path), std::ref(patchProvider));
}

} // namespace synqueen
