#include "corelib.hpp"

#include "standardpaths.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <uv.h>

namespace synqueen {

class Core {
public:
  Core();
  ~Core() = default;

  void run();

private:
};

void initialize() { StandardPaths::initialize(); }

void run() {}

Core::Core() {}

void Core::run() {
  auto *loop = uv_default_loop();
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

} // namespace synqueen
