#pragma once

#include "folderstate.hpp"
#include "patchexchange.hpp"
#include "settings.hpp"
#include "watchers/timerwatcher.hpp"

#include <vector>

namespace synqueen {

class Synchronizer {
public:
  Synchronizer(uv_loop_t *loop);
  ~Synchronizer();

  void loadSettings(const Settings &settings);
  void checkLocal();
  void checkRemotes();

private:
  uv_async_t *createAsyncEvent(uv_loop_t *loop, uv_async_cb callback);
  void cleanAsyncEvent(uv_async_t *event);

private:
  uv_loop_t *loop = nullptr;
  PatchExchange patchExchange;
  std::vector<FolderStatePtr> folderStates;
  // We have one timer watcher for all folders
  TimerWatcher *timerWatcher = nullptr;

  uv_async_t *checkLocalEvent = nullptr;
  uv_async_t *checkRemoteEvent = nullptr;
};

} // namespace synqueen
