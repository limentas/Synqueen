#include "taskscheduler.hpp"

#include <spdlog/spdlog.h>

#include "utils/scopeguard.hpp"

namespace synqueen {

TaskScheduler *TaskScheduler::instance = nullptr;

void TaskScheduler::initialize(uv_loop_t *loop, corral::Nursery *nursery) {
  if (TaskScheduler::instance != nullptr)
    return;
  std::once_flag flag;
  std::call_once(flag, [loop, nursery]() {
    static TaskScheduler t;
    TaskScheduler::instance = &t;
    t.loop = loop;
    t.nursery = nursery;
  });
}

void TaskScheduler::runOnMainThread(std::function<void()> task) {
  getInstance()->runOnMainThreadPrivate(task);
}

void TaskScheduler::runOnMainThreadAsync(
    std::function<corral::Task<void>()> task) {
  getInstance()->runOnMainThreadAsyncPrivate(task);
}

TaskScheduler *TaskScheduler::getInstance() {
  assert(instance != nullptr);
  return instance;
}

void TaskScheduler::runOnMainThreadPrivate(std::function<void()> task) {
  uv_async_t *async = new uv_async_t;
  auto res = uv_async_init(instance->loop, async, [](uv_async_t *handle) {
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
    SPDLOG_ERROR("runOnMainThreadPrivate: Failed to initialize uv_async_t: {}",
                 res);
    delete async;
    return;
  }
  async->data = new std::function<void()>(task);
  res = uv_async_send(async);
  if (res != 0) {
    SPDLOG_ERROR("runOnMainThreadPrivate: Failed to send uv_async_t: {}", res);
  }
}

void TaskScheduler::runOnMainThreadAsyncPrivate(
    std::function<corral::Task<void>()> task) {
  uv_async_t *async = new uv_async_t;
  auto res = uv_async_init(instance->loop, async, [](uv_async_t *handle) {
    std::function<corral::Task<void>()> *task =
        static_cast<std::function<corral::Task<void>()> *>(handle->data);
    instance->nursery->start([task, handle]() -> corral::Task<void> {
      SQ_DEFER(delete task);
      SQ_DEFER(uv_close(reinterpret_cast<uv_handle_t *>(handle),
                        [](uv_handle_t *h) { delete h; }));
      try {
        co_await (*task)();
      } catch (std::exception &e) {
        SPDLOG_ERROR("Uncaught exception occurred in "
                     "runOnMainThreadAsyncPrivate task: {}",
                     e.what());
      }
    });
  });
  if (res != 0) {
    SPDLOG_ERROR(
        "runOnMainThreadAsyncPrivate: Failed to initialize uv_async_t: {}",
        res);
    delete async;
    return;
  }
  async->data = new std::function<corral::Task<void>()>(task);
  res = uv_async_send(async);
  if (res != 0) {
    SPDLOG_ERROR("runOnMainThreadAsyncPrivate: Failed to send uv_async_t: {}",
                 res);
  }
}

} // namespace synqueen
