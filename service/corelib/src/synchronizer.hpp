#pragma once

#include "folderstate.hpp"
#include "patch/patchexchange.hpp"
#include "settings.hpp"
#include "utils/corralheader.hpp"
#include "utils/uvutils.hpp"
#include "watchers/timerwatcher.hpp"

#include <vector>

namespace synqueen {

class Synchronizer {
public:
  Synchronizer(uv_loop_t *loop);
  ~Synchronizer();

  corral::Task<void> run(corral::TaskStarted<> started = {});
  void shutdown();

  void loadSettings(const Settings &settings);

  void checkAllLocal();
  void checkAllRemotes();

private:
  uv_async_t *createAsyncEvent(uv_loop_t *loop, uv_async_cb callback);

private:
  uv_loop_t *loop = nullptr;
  PatchExchange patchExchange;
  std::vector<FolderStatePtr> folderStates;
  // We have one timer watcher for all folders
  std::unique_ptr<TimerWatcher> timerWatcher;

  SharedAsyncPtr checkLocalEvent;
  SharedAsyncPtr checkRemoteEvent;

  corral::Nursery *nursery = nullptr;
};

} // namespace synqueen
