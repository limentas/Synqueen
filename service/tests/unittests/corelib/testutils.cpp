#include "testutils.hpp"

#include "utils/logger.hpp"
#include "utils/standardpaths.hpp"

#include <fstream>

using namespace std::chrono_literals;

const std::chrono::milliseconds TestConditionalWaiter::step = 50ms;
const std::chrono::milliseconds TestNotifiableWaiter::step = 50ms;

void removeFileOrFolder(const std::filesystem::path &folderPath) {
  std::error_code ec;
  fs::remove_all(folderPath, ec);
  if (ec) {
    SPDLOG_ERROR("Failed to remove file or folder: {}. Error: {}", folderPath,
                 ec.message());
  }
}

void cleanupAppFolders() {
  removeFileOrFolder(synqueen::StandardPaths::getConfigPath());
  removeFileOrFolder(synqueen::StandardPaths::getDataPath());
  removeFileOrFolder(synqueen::StandardPaths::getTempPath());
}

int filesInFolder(const std::filesystem::path &folderPath) {
  return std::distance(fs::directory_iterator(folderPath),
                       fs::directory_iterator{});
}

std::string readAll(const std::filesystem::path &filePath) {
  std::ifstream file(filePath, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

std::filesystem::path createTemporaryFolder(const std::string &name) {
  auto tempPath = synqueen::StandardPaths::getTempPath() / name;
  fs::create_directories(tempPath);
  return tempPath;
}

TestConditionalWaiter::TestConditionalWaiter(uv_loop_t &loop,
                                             std::function<bool()> condition)
    : loop(loop), condition(std::move(condition)),
      timer(new uv_timer_t(), synqueen::deleteHandle<uv_timer_t>) {
  timer->data = this;
  uv_timer_init(&loop, timer.get());
}
TestConditionalWaiter::~TestConditionalWaiter() { uv_timer_stop(timer.get()); }

corral::Task<bool>
TestConditionalWaiter::waitFor(std::chrono::milliseconds duration) {
  deadline = std::chrono::steady_clock::now() + duration;
  uv_timer_start(
      timer.get(),
      [](uv_timer_t *handle) {
        auto instance = reinterpret_cast<TestConditionalWaiter *>(handle->data);
        if (instance->condition()) {
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

TestNotifiableWaiter::TestNotifiableWaiter(uv_loop_t &loop)
    : loop(loop), timer(new uv_timer_t(), synqueen::deleteHandle<uv_timer_t>) {
  timer->data = this;
  uv_timer_init(&loop, timer.get());
}

TestNotifiableWaiter::~TestNotifiableWaiter() { uv_timer_stop(timer.get()); }

corral::Task<bool>
TestNotifiableWaiter::waitFor(std::chrono::milliseconds duration) {
  deadline = std::chrono::steady_clock::now() + duration;
  uv_timer_start(
      timer.get(),
      [](uv_timer_t *handle) {
        auto instance = reinterpret_cast<TestNotifiableWaiter *>(handle->data);
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

void TestNotifiableWaiter::notify() { notified = true; }

void TestNotifiableWaiter::reset() {
  notified = false;
  completer.reset();
}

corral::Task<void> sleepFor(uv_loop_t &loop,
                            std::chrono::milliseconds duration) {
  TestConditionalWaiter waiter(loop, []() { return false; });
  co_await waiter.waitFor(duration);
  co_return;
}
