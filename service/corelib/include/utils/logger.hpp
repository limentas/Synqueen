#pragma once

#include <filesystem>
#include <spdlog/spdlog.h>
#include <stacktrace>

template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string> {
  auto format(const std::filesystem::path &path, format_context &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", path.string());
  }
};

template <>
struct fmt::formatter<std::stacktrace> : fmt::formatter<std::string> {
  auto format(const std::stacktrace &st, format_context &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", std::to_string(st));
  }
};
