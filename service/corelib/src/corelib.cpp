#include "corelib.hpp"

#include "standardpaths.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <uv.h>

namespace synqueen {

class Core {
public:
  Core(std::shared_ptr<spdlog::logger> logger = nullptr);
  ~Core() = default;

  void run();
  void stop();

private:
  std::shared_ptr<spdlog::logger> createDefaultLogger();

private:
  std::shared_ptr<spdlog::logger> logger;
  uv_loop_t *loop = nullptr;
};

void initialize(std::shared_ptr<spdlog::logger> logger) {
  StandardPaths::initialize("Synqueen");
}

void run() {}

void stop() {}

Core::Core(std::shared_ptr<spdlog::logger> l) : logger(std::move(l)) {
  if (!logger) {
    logger = spdlog::default_logger();
    if (!logger)
      logger = createDefaultLogger();
  }
}

void Core::run() {
  loop = uv_default_loop();
  if (loop == nullptr) {
    throw std::runtime_error("Failed to get default loop");
  }

  auto result = uv_run(loop, UV_RUN_DEFAULT);
  if (result < 0) {
    throw std::runtime_error("Failed to run loop");
  }

  result = uv_loop_close(loop);
  if (result < 0) {
    throw std::runtime_error("Failed to close loop");
  }
}

void Core::stop() {
  if (loop == nullptr) {
    return;
  }

  uv_stop(loop);
  loop = nullptr;
}

std::shared_ptr<spdlog::logger> Core::createDefaultLogger() {
  return std::shared_ptr<spdlog::logger>();
}

} // namespace synqueen
