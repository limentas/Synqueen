#pragma once

#include <spdlog/spdlog.h>

#include <list>
#include <stacktrace>
#include <stdexcept>
#include <string>

namespace synqueen {

struct StackFrameInfo {
  std::string contextName;
  std::stacktrace trace;
};

class SqException : public std::runtime_error {
public:
  explicit SqException(const std::string &message)
      : std::runtime_error(message),
        frames{{"Exception created", std::stacktrace::current()}} {}

  void addRethrowContext(const std::string &name) {
    frames.push_back({name, std::stacktrace::current()});
  }

  std::list<StackFrameInfo> frames;
};

class SqDoesNotExist : public SqException {
public:
  explicit SqDoesNotExist(const std::string &message) : SqException(message) {}
};

class SqAlreadyUsed : public SqException {
public:
  explicit SqAlreadyUsed(const std::string &message) : SqException(message) {}
};

class SqPermissionDenied : public SqException {
public:
  explicit SqPermissionDenied(const std::string &message)
      : SqException(message) {}
};

class SqNotADirectory : public SqException {
public:
  explicit SqNotADirectory(const std::string &message) : SqException(message) {}
};

class SqUnknownError : public SqException {
public:
  explicit SqUnknownError(const std::string &message) : SqException(message) {}
};

} // namespace synqueen

template <>
struct fmt::formatter<synqueen::SqException> : fmt::formatter<std::string> {
  auto format(const synqueen::SqException &e, format_context &ctx) const
      -> decltype(ctx.out()) {

    auto out = ctx.out();
    out = fmt::format_to(out, "SqException.\n\tMessage: {}", e.what());
    for (const auto &frame : e.frames) {
      out = fmt::format_to(out, "\n\tContext: {}\n\tStacktrace: {}",
                           frame.contextName, std::to_string(frame.trace));
    }
    return out;
  }
};
