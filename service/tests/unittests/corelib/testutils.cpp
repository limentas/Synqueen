#include "testutils.hpp"

#include "utils/logger.hpp"
#include "utils/standardpaths.hpp"

using namespace std::chrono_literals;

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

std::filesystem::path createTemporaryFolder(const std::string &name) {
  auto tempPath = synqueen::StandardPaths::getTempPath() / name;
  fs::create_directories(tempPath);
  return tempPath;
}

const std::chrono::milliseconds TestWaiter::step = 50ms;
