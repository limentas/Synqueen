#include "utils/scopeguard.hpp"

#include "spdlog/spdlog.h"

namespace synqueen {

ScopeGuard::ScopeGuard(std::function<void()> f) : fn(std::move(f)) {}

ScopeGuard::~ScopeGuard() noexcept {
  try {
    fn();
  } catch (std::exception &e) {
    SPDLOG_ERROR("Uncaught exception occurred in ScopeGuard: {}", e.what());
  }
}

} // namespace synqueen
