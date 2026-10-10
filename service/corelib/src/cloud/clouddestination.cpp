#include "clouddestination.hpp"

#include "utils/standardpaths.hpp"

#include <spdlog/spdlog.h>

namespace synqueen::cloud {

CloudDestination::CloudDestination(ICloudGate &cloudGate,
                                   const CloudDestinationConfig &config,
                                   IPatchStorage &patchStorage)
    : cloudGate(cloudGate), config(config), patchStorage(patchStorage) {}

void CloudDestination::initialize(corral::Nursery &nursery) {
  this->nursery = &nursery;
}

void CloudDestination::synchronize() {
  assert(nursery != nullptr);
  nursery->start([this]() -> corral::Task<void> {
    try {
      auto cloudPatches = co_await cloudGate.listPatchFiles(config);
      auto localPatches = patchStorage.listPatches();
      // Compare cloudPatches and localPatches and perform necessary
      // synchronization actions
      std::list<std::string> missingLocalPatches;
      for (const auto &cloudPatch : cloudPatches) {
        auto it = localPatches.find(cloudPatch.index);
        if (it != localPatches.end())
          continue; // Patch exists => go next
        missingLocalPatches.push_back(cloudPatch.fileName);
      }

      co_await pullPatches(missingLocalPatches);
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Failed to synchronize cloud destination: {}", e.what());
    }

    co_return;
  });
}

corral::Task<void>
CloudDestination::pullPatches(const std::list<std::string> &patches) {
  if (patches.empty())
    co_return;

  auto tempPath = StandardPaths::getTempPath();
  auto downloadedFiles =
      co_await cloudGate.downloadFiles(config, patches, tempPath);
  patchStorage.movePatchesToApply(downloadedFiles);
  co_return;
}

} // namespace synqueen::cloud
