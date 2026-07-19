#include "synchronizer.hpp"

namespace synqueen {

Synchronizer::Synchronizer(uv_loop_t *loop) : loop(loop), patchExchange(loop) {
  checkLocalEvent = createAsyncEvent(loop, [](uv_async_t *handle) {
    auto synchronizer = reinterpret_cast<Synchronizer *>(handle->data);
    synchronizer->checkLocal();
  });
  checkRemoteEvent = createAsyncEvent(loop, [](uv_async_t *handle) {
    auto synchronizer = reinterpret_cast<Synchronizer *>(handle->data);
    synchronizer->checkRemotes();
  });
}

Synchronizer::~Synchronizer() {
  if (timerWatcher) {
    delete timerWatcher;
    timerWatcher = nullptr;
  }

  cleanAsyncEvent(checkLocalEvent);
  checkLocalEvent = nullptr;

  cleanAsyncEvent(checkRemoteEvent);
  checkRemoteEvent = nullptr;
}

void Synchronizer::loadSettings(const Settings &settings) {
  for (const auto &folderSettings : settings.folders) {
    FolderStatePtr folderState =
        std::make_shared<FolderState>(folderSettings.path);
    folderState->initialize();
    folderStates.push_back(folderState);
  }

  timerWatcher = new TimerWatcher(checkLocalEvent, checkRemoteEvent, loop);
  timerWatcher->startWatch();
}

void Synchronizer::checkLocal() {}

void Synchronizer::checkRemotes() {}

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

void Synchronizer::cleanAsyncEvent(uv_async_t *event) {
  if (event == nullptr)
    return;
  uv_close(reinterpret_cast<uv_handle_t *>(event), [](uv_handle_t *handle) {
    delete reinterpret_cast<uv_async_t *>(handle);
  });
}

} // namespace synqueen
