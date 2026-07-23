#include "corelib.hpp"

#include "settings.hpp"
#include "synchronizer.hpp"
#include "utils/standardpaths.hpp"
#include "utils/utils.hpp"
#include "utils/uvutils.hpp"

#include <future>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <uv.h>

using namespace std;

namespace synqueen {

class Core {
public:
  Core(shared_ptr<spdlog::logger> logger = nullptr);
  ~Core() = default;

  void initialize();
  void run();
  shared_future<void> stop();

private:
  shared_ptr<spdlog::logger> createDefaultLogger();
  void stopAsync();

private:
  shared_ptr<spdlog::logger> logger;
  LoopPtr loop;
  Settings settings;
  unique_ptr<Synchronizer> synchronizer;
  AsyncPtr stopEvent;
  promise<void> stopPromise;
  shared_future<void> stopFuture;
};

Core *coreInstance = nullptr;

void initialize(shared_ptr<spdlog::logger> logger) {
  if (coreInstance)
    return; // Already initialized

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
  if (!coreInstance) {
    return;
  }
  coreInstance->run();
}

void stop() {
  if (!coreInstance)
    return;
  auto future = coreInstance->stop();
  future.wait();
}

Core::Core(shared_ptr<spdlog::logger> l)
    : logger(l), loop(nullptr, deleteLoop), stopEvent(nullptr, deleteAsync) {}

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
  } catch (const exception &e) {
    spdlog::error("Failed to load settings: {}", e.what());
    throw;
  }

  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  if (result < 0) {
    spdlog::critical("Failed to initialize main core loop. Error: {}",
                     uv_strerror(result));
    delete l;
    throw std::runtime_error("Failed to initialize main core loop. Error: " +
                             std::string(uv_strerror(result)));
  }
  loop = LoopPtr(l, deleteLoop);

  if (!synchronizer) {
    synchronizer = make_unique<Synchronizer>(loop.get());
    synchronizer->loadSettings(settings);
  }

  auto event = new uv_async_t();
  event->data = this;
  result = uv_async_init(loop.get(), event, [](uv_async_t *handle) {
    auto core = reinterpret_cast<Core *>(handle->data);
    core->stopAsync();
  });
  if (result < 0) {
    delete event;
    throw runtime_error("Failed to create stop event. Error: " +
                        string(uv_strerror(result)));
  }
  stopEvent = AsyncPtr(event, deleteAsync);
  stopFuture = shared_future<void>();
}

void Core::run() {
  assert(loop);

  synchronizer->checkLocal();
  auto result = uv_run(loop.get(), UV_RUN_DEFAULT);
  if (result < 0) {
    throw runtime_error("Failed to run loop");
  }

  loop.reset();

  logger->debug("Core loop exited");
  stopPromise.set_value();
}

shared_future<void> Core::stop() {
  logger->trace("Core: stop");
  if (stopFuture.valid()) {
    // Already stopping
    return stopFuture;
  }

  if (stopEvent) {
    logger->trace("Core: sending stop event to loop");
    uv_async_send(stopEvent.get());
  } else {
    logger->warn("Core: stopEvent is null, cannot send stop event to loop");
    // return ready future
    auto p = promise<void>();
    p.set_value();
    return p.get_future().share();
  }

  stopPromise = promise<void>();
  stopFuture = stopPromise.get_future().share();
  return stopFuture;
}

shared_ptr<spdlog::logger> Core::createDefaultLogger() {
  return shared_ptr<spdlog::logger>(new spdlog::logger(
      "synqueen", make_shared<spdlog::sinks::stdout_color_sink_mt>()));
}

void Core::stopAsync() {
  // NOTE: The approach here can be fragile. In worst case scenario loop may
  // never exit if one of handles is not closed properly.
  // At this moment I don't know how hurt this approach will be.
  // If it doesn't work - consider using `uvw` C++ wrapper for libuv.
  // It keeps track of all handles and can close them properly.
  logger->trace("Core: stopAsync");
  // We just have to close all libuv handles and the loop will exit
  // automatically
  synchronizer.reset();
  stopEvent.reset();
}

} // namespace synqueen
