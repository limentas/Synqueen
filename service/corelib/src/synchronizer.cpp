#include "synchronizer.hpp"

#include "patch/patchbackend.hpp"

#include <memory>
#include <spdlog/spdlog.h>

using namespace std;

namespace synqueen {

Synchronizer::Synchronizer(uv_loop_t *loop)
    : loop(loop),
      patchExchange(std::unique_ptr<PatchBackend>(createPatchBackend(loop))),
      checkLocalEvent(nullptr, deleteHandle<uv_async_t>),
      checkRemoteEvent(nullptr, deleteHandle<uv_async_t>) {
  checkLocalEvent = SharedAsyncPtr(
      createAsyncEvent(loop,
                       [](uv_async_t *handle) {
                         auto synchronizer =
                             reinterpret_cast<Synchronizer *>(handle->data);
                         synchronizer->checkAllLocal();
                       }),
      deleteHandle<uv_async_t>);
  checkRemoteEvent = SharedAsyncPtr(
      createAsyncEvent(loop,
                       [](uv_async_t *handle) {
                         auto synchronizer =
                             reinterpret_cast<Synchronizer *>(handle->data);
                         synchronizer->checkAllRemotes();
                       }),
      deleteHandle<uv_async_t>);
}

Synchronizer::~Synchronizer() {
  timerWatcher.reset();
  checkLocalEvent.reset();
  checkRemoteEvent.reset();
}

corral::Task<void> Synchronizer::shutdown() {
  timerWatcher->stopWatch();
  co_await patchExchange.shutdown();
  co_return;
}

void Synchronizer::loadSettings(const Settings &settings) {
  for (const auto &folderSettings : settings.folders) {
    FolderStatePtr folderState =
        std::make_shared<FolderState>(folderSettings.path);
    folderState->initialize();
    folderStates.push_back(folderState);
    spdlog::info("Loaded folder state for path: {}", folderSettings.path);
  }

  timerWatcher =
      std::make_unique<TimerWatcher>(checkLocalEvent, checkRemoteEvent, loop);
  timerWatcher->startWatch();
}

void Synchronizer::checkAllLocal() {
  spdlog::info("Checking all local folder states...");
}

void Synchronizer::checkAllRemotes() {
  spdlog::info("Checking all remote folder states...");
}

uv_async_t *Synchronizer::createAsyncEvent(uv_loop_t *loop,
                                           uv_async_cb callback) {
  auto event = new uv_async_t();
  auto result = uv_async_init(loop, event, callback);
  if (result < 0) {
    delete event;
    throw std::runtime_error("Failed to create async event. Error: " +
                             std::string(uv_strerror(result)));
  }
  event->data = this;
  return event;
}

} // namespace synqueen
