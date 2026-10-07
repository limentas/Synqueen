#pragma once

#include "utils/corralheader.hpp"
#include "utils/corralutils.hpp"
#include "utils/uvutils.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <string>

namespace fs = std::filesystem;

void removeFileOrFolder(const std::string &folderPath);

class TestWaiter {
public:
  TestWaiter(uv_loop_t *loop)
      : loop(loop),
        timer(new uv_timer_t(), synqueen::deleteHandle<uv_timer_t>) {
    assert(loop != nullptr);
    timer->data = this;
    uv_timer_init(loop, timer.get());
  }
  ~TestWaiter() { uv_timer_stop(timer.get()); }

  corral::Task<bool> waitFor(std::chrono::milliseconds duration) {
    deadline = std::chrono::steady_clock::now() + duration;
    uv_timer_start(
        timer.get(),
        [](uv_timer_t *handle) {
          auto instance = reinterpret_cast<TestWaiter *>(handle->data);
          if (instance->notified) {
            uv_timer_stop(handle);
            instance->completer.completeCommand(true);
            return;
          }
          if (std::chrono::steady_clock::now() >= instance->deadline) {
            uv_timer_stop(handle);
            instance->completer.completeCommand(false);
            return;
          }
        },
        step.count(), step.count());
    co_return co_await synqueen::Awaitable<bool>(completer);
  }

  void notify() { notified = true; }

  void reset() {
    notified = false;
    completer.reset();
  }

private:
  static const std::chrono::milliseconds step;

  uv_loop_t *loop = nullptr;
  synqueen::TimerPtr timer;
  bool notified = false;
  std::chrono::time_point<std::chrono::steady_clock> deadline;
  synqueen::Completer<bool> completer;
};
