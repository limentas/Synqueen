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
        try {
          auto result = co_await patchProvider.checkLocalState(path);
          SPDLOG_INFO(
              "Local state for folder {}: initialized={}, "
              "hasUncommittedChanges={}, hasConflicts={}, lastCommitHash={}",
              path, result.initialized, result.hasUncommittedChanges,
              result.hasConflicts, result.lastCommitHash);
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Exception while checking local state for folder {}: {}",
                       path, e.what());
          co_return;
        }
      },
      std::cref(path), std::ref(patchProvider));
}

} // namespace synqueen
