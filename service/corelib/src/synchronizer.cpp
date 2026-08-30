#include "synchronizer.hpp"

#include "patch/patchbackend.hpp"

#include "utils/corralheader.hpp"

#include <memory>
#include <spdlog/spdlog.h>

using namespace std;

namespace synqueen {

Synchronizer::Synchronizer(uv_loop_t *loop)
    : loop(loop),
      patchExchange(std::unique_ptr<PatchBackend>(createPatchBackend(loop))),
      checkLocalEvent(nullptr, deleteHandle<uv_async_t>),
      checkRemoteEvent(nullptr, deleteHandle<uv_async_t>) {}

Synchronizer::~Synchronizer() {
  // Make sure that corral does what is should
  assert(nursery == nullptr);
}

corral::Task<void> Synchronizer::run(corral::TaskStarted<> started) {
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
  timerWatcher =
      std::make_unique<TimerWatcher>(checkLocalEvent, checkRemoteEvent, loop);
  timerWatcher->startWatch();

  // The nursery will be cleared upon last task completion/cancellation
  CORRAL_WITH_NURSERY(n) {
    co_await n.start(corral::openNursery, std::ref(nursery));
    started(); // signal readiness
    co_return corral::join;
  };

  // We arrive here after all corral tasks have completed or cancelled

  // NOTE: The approach here can be fragile. In worst case scenario loop may
  // never exit if one of handles is not closed properly.
  // At this moment I don't know how painful this approach will be.
  // If it doesn't work - consider using `uvw` C++ wrapper for libuv.
  // It keeps track of all handles and can close them properly.

  // We just have to close all libuv handles and the loop will exit
  // automatically
  timerWatcher.reset();
  checkLocalEvent.reset();
  checkRemoteEvent.reset();
}

void Synchronizer::shutdown() {
  assert(nursery != nullptr);
  assert(timerWatcher != nullptr);
  timerWatcher->stopWatch();
  nursery->start([this]() -> corral::Task<void> {
    co_await corral::noncancellable(patchExchange.shutdown());
  });
  nursery->cancel();
}

void Synchronizer::loadSettings(const Settings &settings) {
  for (const auto &folderSettings : settings.folders) {
    FolderStatePtr folderState =
        std::make_shared<FolderState>(folderSettings.path);
    folderState->initialize();
    folderStates.push_back(folderState);
    SPDLOG_INFO("Loaded folder state for path: {}", folderSettings.path);
  }
}

void Synchronizer::checkAllLocal() {
  SPDLOG_INFO("Checking all local folder states...");
}

void Synchronizer::checkAllRemotes() {
  SPDLOG_INFO("Checking all remote folder states...");
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
