#include "notifier.hpp"

namespace synqueen::utils {

void Notifier::addSyncHandler(void *context, std::function<void()> handler) {
  handlers.push_back({context, std::move(handler)});
}

void Notifier::removeSyncHandler(void *context) {
  handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                                [&](const HandlerEntry &entry) {
                                  return entry.context == context;
                                }),
                 handlers.end());
}

void Notifier::notify() {
  for (const auto &entry : handlers) {
    entry.handler();
  }
}

} // namespace synqueen::utils
