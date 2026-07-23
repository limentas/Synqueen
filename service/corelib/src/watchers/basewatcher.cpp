#include "basewatcher.hpp"

#include <uv.h>

namespace synqueen {

BaseWatcher::BaseWatcher(SharedAsyncPtr checkLocal, SharedAsyncPtr checkRemotes)
    : checkLocal(checkLocal), checkRemotes(checkRemotes) {}

void BaseWatcher::checkLocalChanges() {
  if (!checkLocal)
    return;
  uv_async_send(checkLocal.get());
}

void BaseWatcher::checkRemoteChanges() {
  if (!checkRemotes)
    return;
  uv_async_send(checkRemotes.get());
}

} // namespace synqueen
