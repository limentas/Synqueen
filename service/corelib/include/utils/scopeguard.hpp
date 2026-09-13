#pragma once

#include <functional>

namespace synqueen {

class ScopeGuard {
public:
  explicit ScopeGuard(std::function<void()> fn);

  ~ScopeGuard() noexcept;

private:
  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;

private:
  std::function<void()> fn;
};

#define SQ_CONCAT_IMPL(a, b) a##b
#define SQ_CONCAT(a, b) SQ_CONCAT_IMPL(a, b)

#define SQ_DEFER(code) ScopeGuard SQ_CONCAT(_defer_, __LINE__)([&] { code; })

} // namespace synqueen
