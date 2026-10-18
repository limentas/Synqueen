#include "clouddestination.hpp"

#include "utils/standardpaths.hpp"

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

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
      const auto cloudPatches = co_await cloudGate.listPatchFiles(config);
      const auto &patchesToApply = patchStorage.listPatchesToApply();
      const auto &appliedPatches = patchStorage.listAppliedPatches();
      // Compare cloudPatches and local patches and perform necessary
      // synchronization actions
      std::list<std::string> missingLocalPatches;
      for (const auto &cloudPatch : std::views::keys(cloudPatches)) {
        auto itToApply = patchesToApply.find(cloudPatch);
        if (itToApply != patchesToApply.end())
          continue; // Patch exists => go next
        auto itApplied = appliedPatches.find(cloudPatch);
        if (itApplied != appliedPatches.end())
          continue; // Patch already applied
        missingLocalPatches.push_back(cloudPatch);
      }
      co_await pullPatches(missingLocalPatches);

      // TODO: Upload outgoing patches
      std::list<fs::path> patchesToUpload;
      for (const auto &patches = patchStorage.listOutgoingPatches();
           const auto &[fileName, file] : patches) {
        if (cloudPatches.find(fileName) == cloudPatches.end()) {
          patchesToUpload.push_back(file.path);
        }
      }
      co_await cloudGate.uploadFiles(config, patchesToUpload);
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
