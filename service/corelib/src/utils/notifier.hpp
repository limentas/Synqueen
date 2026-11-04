#pragma once

#include <functional>
#include <vector>

namespace synqueen::utils {

class ISubscribable {
public:
  virtual ~ISubscribable() = default;
  virtual void addSyncHandler(void *context, std::function<void()> handler) = 0;
  virtual void removeSyncHandler(void *context) = 0;
};

// A simple notifier class that allows registering synchronous handlers
// and notifying them when an event occurs.
class Notifier : public ISubscribable {
public:
  Notifier() = default;
  ~Notifier() = default;

public:
  virtual void addSyncHandler(void *context,
                              std::function<void()> handler) override;
  virtual void removeSyncHandler(void *context) override;

  void notify();

private:
  struct HandlerEntry {
    void *context;
    std::function<void()> handler;
  };

  std::vector<HandlerEntry> handlers;
};

} // namespace synqueen::utils
