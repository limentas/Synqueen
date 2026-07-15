#include "basewatcher.hpp"

namespace synqueen {

BaseWatcher::BaseWatcher(uv_async_t *checkLocal, uv_async_t *checkRemotes)
    : checkLocal(checkLocal), checkRemotes(checkRemotes) {}

void BaseWatcher::checkLocalChanges() {
  if (!checkLocal)
    return;
  uv_async_send(checkLocal);
}

void BaseWatcher::checkRemoteChanges() {
  if (!checkRemotes)
    return;
  uv_async_send(checkRemotes);
}

} // namespace synqueen
