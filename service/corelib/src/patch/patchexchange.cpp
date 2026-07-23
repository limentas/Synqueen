#include "patchexchange.hpp"

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <uv.h>

namespace synqueen {

PatchExchange::PatchExchange(uv_loop_t *l) : loop(l) {
  if (loop == nullptr) {
    throw std::runtime_error("UV loop is not initialized");
  }
}

void PatchExchange::ensureFolderInitialized(const std::string &folderPath) {
  checkStatus(folderPath);
}

void PatchExchange::checkStatus(const std::string &folderPath) {}

} // namespace synqueen
