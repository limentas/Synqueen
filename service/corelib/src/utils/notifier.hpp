#pragma once

#include <functional>
#include <vector>

namespace synqueen::utils {

class ISubscribable {
public:
  virtual ~ISubscribable() = default;
  virtual void addSyncHandler(std::function<void()> handler) = 0;
};

// A simple notifier class that allows registering synchronous handlers
// and notifying them when an event occurs.
class Notifier : public ISubscribable {
public:
  Notifier() = default;
  ~Notifier() = default;

public:
  void addSyncHandler(std::function<void()> handler) override;

  void notify();

private:
  std::vector<std::function<void()>> handlers;
};

} // namespace synqueen::utils
