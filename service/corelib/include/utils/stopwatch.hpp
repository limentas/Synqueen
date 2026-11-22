#pragma once

#include <chrono>

#include <spdlog/spdlog.h>

namespace synqueen {

// Inspired by spdlog's stopwatch implementation
class Stopwatch {
  using clock = std::chrono::steady_clock;
  std::chrono::time_point<clock> startPoint;

public:
  Stopwatch() : startPoint{clock::now()} {}

  std::chrono::duration<double> elapsed() const {
    return std::chrono::duration<double>(clock::now() - startPoint);
  }

  std::chrono::milliseconds elapsed_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() -
                                                                 startPoint);
  }

  std::chrono::microseconds elapsed_us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(clock::now() -
                                                                 startPoint);
  }

  void reset() { startPoint = clock::now(); }
};

} // namespace synqueen

template <>
struct fmt::formatter<synqueen::Stopwatch> : fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const synqueen::Stopwatch &sw, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    auto elapsedUs = sw.elapsed_us().count();
    if (elapsedUs < 10'000) {
      return fmt::format_to(ctx.out(), "{:L}us", elapsedUs);
    }
    if (elapsedUs < 1'000'000) {
      return fmt::format_to(ctx.out(), "{:L}ms", sw.elapsed_ms().count());
    }
    return fmt::format_to(ctx.out(), "{}s",
                          std::to_string(sw.elapsed().count()));
  }
};
