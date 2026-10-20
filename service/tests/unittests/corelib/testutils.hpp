#pragma once

#include "utils/corralheader.hpp"
#include "utils/corralutils.hpp"
#include "utils/uvutils.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <spdlog/spdlog.h>
#include <string>

namespace fs = std::filesystem;

void removeFileOrFolder(const std::filesystem::path &folderPath);
void cleanupAppFolders();
int filesInFolder(const std::filesystem::path &folderPath);
std::string readAll(const std::filesystem::path &filePath);

// Creates temporary folder or file in StandardPaths::getTempPath()
std::filesystem::path createTemporaryFolder(const std::string &name);

class TestConditionalWaiter {
public:
  TestConditionalWaiter(uv_loop_t &loop, std::function<bool()> condition);
  ~TestConditionalWaiter();

  corral::Task<bool> waitFor(std::chrono::milliseconds duration);

private:
  static const std::chrono::milliseconds step;

  uv_loop_t &loop;
  synqueen::TimerPtr timer;
  std::function<bool()> condition;
  std::chrono::time_point<std::chrono::steady_clock> deadline;
  synqueen::Completer<bool> completer;
};

class TestNotifiableWaiter {
public:
  TestNotifiableWaiter(uv_loop_t &loop);
  ~TestNotifiableWaiter();

  corral::Task<bool> waitFor(std::chrono::milliseconds duration);
  void notify();
  void reset();

private:
  static const std::chrono::milliseconds step;

  uv_loop_t &loop;
  synqueen::TimerPtr timer;
  bool notified = false;
  std::chrono::time_point<std::chrono::steady_clock> deadline;
  synqueen::Completer<bool> completer;
};

corral::Task<void> sleepFor(uv_loop_t &loop,
                            std::chrono::milliseconds duration);