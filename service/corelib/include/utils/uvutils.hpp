#pragma once

#include <uv.h>

#include <memory>

namespace synqueen {

typedef std::unique_ptr<uv_signal_t, void (*)(uv_signal_t *)> SignalPtr;
typedef std::unique_ptr<uv_loop_t, void (*)(uv_loop_t *)> LoopPtr;
typedef std::unique_ptr<uv_async_t, void (*)(uv_async_t *)> AsyncPtr;
typedef std::unique_ptr<uv_timer_t, void (*)(uv_timer_t *)> TimerPtr;
typedef std::unique_ptr<uv_pipe_t, void (*)(uv_pipe_t *)> PipePtr;
typedef std::shared_ptr<uv_async_t> SharedAsyncPtr;

void deleteSignal(uv_signal_t *signal);
void deleteLoop(uv_loop_t *loop);
template <typename T> void deleteHandle(T *handle) {
  if (handle == nullptr)
    return;
  uv_close(reinterpret_cast<uv_handle_t *>(handle),
           [](uv_handle_t *h) { delete reinterpret_cast<T *>(h); });
}
void deletePipe(uv_pipe_t *pipe);

void printLoopHandles(uv_loop_t *loop);

} // namespace synqueen
