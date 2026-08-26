#include "patchexchange.hpp"

#include "patchbackend.hpp"

#include <cctype>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <uv.h>

namespace synqueen {

using namespace patch;

PatchExchange::PatchExchange(std::unique_ptr<PatchBackend> backendPtr)
    : backend(std::move(backendPtr)) {}

corral::Task<void> PatchExchange::shutdown() { return backend->shutdown(); }

corral::Task<patch::LocalStateResult>
PatchExchange::checkLocalState(const std::string &folderPath) {
  return backend->checkLocalState(folderPath);
}

corral::Task<patch::PreparePatchResult>
PatchExchange::preparePatch(const std::string &folderPath) {
  return backend->preparePatch(folderPath);
}

void PatchExchange::ensureFolderInitialized(const std::string &folderPath) {}

} // namespace synqueen
