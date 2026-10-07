#include "notifier.hpp"

namespace synqueen::utils {

void Notifier::addSyncHandler(std::function<void()> handler) {
  handlers.push_back(std::move(handler));
}

void Notifier::notify() {
  for (const auto &handler : handlers) {
    handler();
  }
}

} // namespace synqueen::utils
