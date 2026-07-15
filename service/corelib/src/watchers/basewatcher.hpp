#pragma once

#include <list>
#include <memory>
#include <uv.h>

namespace synqueen {

class BaseWatcher {
public:
  BaseWatcher(uv_async_t *checkLocal, uv_async_t *checkRemotes);
  virtual ~BaseWatcher() = default;

  virtual void startWatch() = 0;
  virtual void stopWatch() = 0;

protected:
  void checkLocalChanges();
  void checkRemoteChanges();

private:
  uv_async_t *checkLocal = nullptr;
  uv_async_t *checkRemotes = nullptr;
};

typedef std::list<std::shared_ptr<BaseWatcher>> WatcherList;

} // namespace synqueen
