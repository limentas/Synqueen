#pragma once

#include "utils/uvutils.hpp"
#include <list>
#include <memory>
#include <uv.h>

namespace synqueen {

class BaseWatcher {
public:
  BaseWatcher(SharedAsyncPtr checkLocal, SharedAsyncPtr checkRemotes);
  virtual ~BaseWatcher() = default;

  virtual void startWatch() = 0;
  virtual void stopWatch() = 0;

protected:
  void checkLocalChanges();
  void checkRemoteChanges();

private:
  SharedAsyncPtr checkLocal;
  SharedAsyncPtr checkRemotes;
};

typedef std::list<std::shared_ptr<BaseWatcher>> WatcherList;

} // namespace synqueen
