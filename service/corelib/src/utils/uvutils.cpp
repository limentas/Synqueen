#include "utils/uvutils.hpp"

#include <iostream>
#include <list>
#include <spdlog/spdlog.h>

namespace synqueen {

void deleteSignal(uv_signal_t *signal) {
  if (signal == nullptr)
    return;
  auto result = uv_signal_stop(signal);
  if (result < 0) {
    spdlog::warn("Failed to stop signal handler. Error: {}",
                 uv_strerror(result));
  }
  uv_close(reinterpret_cast<uv_handle_t *>(signal), [](uv_handle_t *handle) {
    delete reinterpret_cast<uv_signal_t *>(handle);
  });
}

void deleteLoop(uv_loop_t *loop) {
  if (loop == nullptr)
    return;
  auto result = uv_loop_close(loop);
  if (result < 0) {
    if (result == UV_EBUSY) {
      spdlog::warn(
          "Failed to close signal handler loop. There are still active handles "
          "in the loop. Error: {}",
          uv_strerror(result));
      printLoopHandles(loop);
      // TODO: We can try to close all handles here with uv_walk + uv_close +
      // uv_run
    } else {
      spdlog::warn("Failed to close signal handler loop. Error: {}",
                   uv_strerror(result));
    }
  }
  // If uv_loop_close fails this call may cause problems, but we have to free
  // the memory anyway
  delete loop;
}

void deleteAsync(uv_async_t *async) {
  if (async == nullptr)
    return;
  uv_close(reinterpret_cast<uv_handle_t *>(async), [](uv_handle_t *handle) {
    delete reinterpret_cast<uv_async_t *>(handle);
  });
}

void printLoopHandles(uv_loop_t *loop) {
  std::list<uv_handle_t *> handleList;
  uv_walk(
      loop,
      [](uv_handle_t *h, void *arg) {
        auto handleList = reinterpret_cast<std::list<uv_handle_t *> *>(arg);
        handleList->push_back(h);
      },
      &handleList);

  std::cout << "Active handles in loop: " << std::endl;
  for (const auto &h : handleList) {
    std::cout << "Handle type: " << uv_handle_type_name(h->type)
              << ", has ref:" << uv_has_ref(h)
              << ", is closing:" << uv_is_closing(h) << std::endl;
  }
}

} // namespace synqueen
