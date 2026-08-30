#include "utils/uvutils.hpp"

#include <list>
#include <spdlog/spdlog.h>

namespace synqueen {

void deleteSignal(uv_signal_t *signal) {
  if (signal == nullptr)
    return;
  auto result = uv_signal_stop(signal);
  if (result < 0) {
    SPDLOG_WARN("Failed to stop signal handler. Error: {}",
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
      SPDLOG_WARN(
          "Failed to close signal handler loop. There are still active handles "
          "in the loop. Error: {}",
          uv_strerror(result));
      printLoopHandles(loop);
      // TODO: We can try to close all handles here with uv_walk + uv_close +
      // uv_run
    } else {
      SPDLOG_WARN("Failed to close signal handler loop. Error: {}",
                  uv_strerror(result));
    }
  }
  // If uv_loop_close fails this call may cause problems, but we have to free
  // the memory anyway
  delete loop;
}

void deletePipe(uv_pipe_t *pipe) {
  if (pipe == nullptr)
    return;
  auto *handle = reinterpret_cast<uv_handle_t *>(pipe);
  if (uv_is_closing(handle)) {
    return;
  }

  uv_close(handle,
           [](uv_handle_t *h) { delete reinterpret_cast<uv_pipe_t *>(h); });
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

  SPDLOG_DEBUG("Active handles in the loop:");
  for (const auto &h : handleList) {
    SPDLOG_DEBUG("Handle type: {}, has ref: {}, is closing: {}",
                 uv_handle_type_name(h->type), uv_has_ref(h), uv_is_closing(h));
  }
}

} // namespace synqueen
