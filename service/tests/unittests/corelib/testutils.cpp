#include "testutils.hpp"

using namespace std::chrono_literals;

void removeFileOrFolder(const std::string &folderPath) {
  std::error_code ec;
  fs::remove_all(folderPath, ec);
  if (ec) {
    SPDLOG_ERROR("Failed to remove file or folder: {}. Error: {}", folderPath,
                 ec.message());
  }
}

const std::chrono::milliseconds TestWaiter::step = 50ms;
