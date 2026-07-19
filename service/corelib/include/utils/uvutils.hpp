#pragma once

#include <uv.h>

#include <memory>

namespace synqueen {

typedef std::unique_ptr<uv_signal_t, void (*)(uv_signal_t *)> SignalPtr;
typedef std::unique_ptr<uv_loop_t, void (*)(uv_loop_t *)> LoopPtr;
typedef std::unique_ptr<uv_async_t, void (*)(uv_async_t *)> AsyncPtr;

void deleteSignal(uv_signal_t *signal);
void deleteLoop(uv_loop_t *loop);
void deleteAsync(uv_async_t *async);

void printLoopHandles(uv_loop_t *loop);

} // namespace synqueen