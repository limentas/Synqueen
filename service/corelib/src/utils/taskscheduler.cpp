#include "taskscheduler.hpp"

#include <mutex>
#include <spdlog/spdlog.h>

#include "utils/scopeguard.hpp"

namespace synqueen {

TaskScheduler::TaskScheduler(uv_loop_t &loop) : loop(loop) {}

void TaskScheduler::runInEventLoop(std::function<void()> task) {
  auto async = new uv_async_t;
  async->data = new std::function<void()>(task);
  auto res = uv_async_init(&loop, async, [](uv_async_t *handle) {
    std::function<void()> *task =
        static_cast<std::function<void()> *>(handle->data);
    SQ_DEFER(delete task);
    SQ_DEFER(uv_close(reinterpret_cast<uv_handle_t *>(handle),
                      [](uv_handle_t *h) { delete h; }));
    try {
      (*task)();
    } catch (std::exception &e) {
      SPDLOG_ERROR(
          "Uncaught exception occurred in runOnMainThreadPrivate task: {}",
          e.what());
    }
  });
  if (res != 0) {
    SPDLOG_ERROR("runInEventLoopPrivate: Failed to initialize uv_async_t: {}",
                 res);
    delete async;
    return;
  }
  res = uv_async_send(async);
  if (res != 0) {
    SPDLOG_ERROR("runInEventLoopPrivate: Failed to send uv_async_t: {}", res);
  }
}

AsyncTaskScheduler::AsyncTaskScheduler(uv_loop_t &loop,
                                       corral::Nursery &nursery)
    : TaskScheduler(loop), nursery(nursery) {}

void AsyncTaskScheduler::runInEventLoopAsync(
    std::function<corral::Task<void>()> task) {
  uv_async_t *async = new uv_async_t;
  struct AsyncData {
    std::function<corral::Task<void>()> task;
    corral::Nursery *nursery;
  };
  AsyncData *data = new AsyncData{task};
  nursery.start(corral::openNursery, std::ref(data->nursery));
  async->data = data;

  auto res = uv_async_init(&loop, async, [](uv_async_t *handle) {
    auto context = static_cast<AsyncData *>(handle->data);
    context->nursery->start([context, handle]() -> corral::Task<void> {
      SQ_DEFER(delete context);
      SQ_DEFER(uv_close(reinterpret_cast<uv_handle_t *>(handle),
                        [](uv_handle_t *h) { delete h; }));
      try {
        co_await (context->task)();
      } catch (std::exception &e) {
        SPDLOG_ERROR("Uncaught exception occurred in "
                     "runInEventLoopAsyncPrivate task: {}",
                     e.what());
      }
    });
  });
  if (res != 0) {
    SPDLOG_ERROR(
        "runInEventLoopAsyncPrivate: Failed to initialize uv_async_t: {}", res);
    delete async;
    return;
  }
  async->data = new std::function<corral::Task<void>()>(task);
  res = uv_async_send(async);
  if (res != 0) {
    SPDLOG_ERROR("runInEventLoopAsyncPrivate: Failed to send uv_async_t: {}",
                 res);
  }
}

} // namespace synqueen
