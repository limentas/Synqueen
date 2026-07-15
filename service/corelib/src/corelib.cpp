#include "corelib.hpp"

#include "settings.hpp"
#include "standardpaths.hpp"
#include "synchronizer.hpp"
#include "utils.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <uv.h>

namespace synqueen {

class Core {
public:
  Core(std::shared_ptr<spdlog::logger> logger = nullptr);
  ~Core() = default;

  void initialize();
  void run();
  void stop();

private:
  std::shared_ptr<spdlog::logger> createDefaultLogger();

private:
  std::shared_ptr<spdlog::logger> logger;
  uv_loop_t *loop = nullptr;
  Settings settings;
  Synchronizer *synchronizer = nullptr;
};

Core *coreInstance = nullptr;

void initialize(std::shared_ptr<spdlog::logger> logger) {
  coreInstance = new Core(logger);
  coreInstance->initialize();
}

void deinitialize() {
  if (coreInstance) {
    delete coreInstance;
    coreInstance = nullptr;
  }
}

void run() {
  if (coreInstance) {
    coreInstance->run();
  }
}

void stop() {
  if (coreInstance) {
    coreInstance->stop();
  }
}

Core::Core(std::shared_ptr<spdlog::logger> l) : logger(l) {}

void Core::initialize() {
  StandardPaths::initialize("Synqueen");

  if (!logger)
    logger = createDefaultLogger();
  spdlog::set_default_logger(logger);

  try {
    SettingsProvider settingsProvider;
    auto configPath = StandardPaths::getConfigPath();
    configPath += DIR_SEPARATOR_STR;
    configPath += "settings.json";
    settings = settingsProvider.loadSettingsFromJson(configPath);
  } catch (const std::exception &e) {
    SPDLOG_ERROR("Failed to load settings: {}", e.what());
    throw;
  }

  loop = uv_default_loop();
  if (!synchronizer) {
    synchronizer = new Synchronizer(loop);
    synchronizer->loadSettings(settings);
  }
}

void Core::run() {
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
  if (synchronizer) {
    // We need to delete everything before stopping the loop, uv_close works
    // asynchronously and requires the loop to clean handles properly.
    delete synchronizer;
    synchronizer = nullptr;
  }

  if (loop != nullptr) {
    uv_stop(loop);
    loop = nullptr;
  }
}

std::shared_ptr<spdlog::logger> Core::createDefaultLogger() {
  return std::shared_ptr<spdlog::logger>();
}

} // namespace synqueen
