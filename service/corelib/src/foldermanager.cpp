#include "foldermanager.hpp"

#include <spdlog/spdlog.h>

namespace synqueen {

void FolderManager::initialize() {}

void FolderManager::check() {
  // TODO: pass path as argument
  nursery.start([]() -> corral::Task<void> {
    auto result = co_await patchProvider.checkLocalState(path);
    if (!result.ok) {
      SPDLOG_ERROR("Failed to check local state for folder {}: {}", path,
                   result.errorMessage);
      co_return;
    }

    SPDLOG_INFO("Local state for folder {}: initialized={}, "
                "hasUncommittedChanges={}, hasConflicts={}, lastCommitHash={}",
                path, result.initialized, result.hasUncommittedChanges,
                result.hasConflicts, result.lastCommitHash);
  });
}

} // namespace synqueen
