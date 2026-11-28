#include "localfoldergate.hpp"

#include "utils/exceptions.hpp"
#include "utils/logger.hpp"

namespace chrono = std::chrono;
namespace fs = std::filesystem;

namespace synqueen::cloud {

corral::Task<void> LocalFolderGate::shutdown() { return corral::noop(); }

corral::Task<void>
LocalFolderGate::initDestination(const CloudDestinationConfig &config) {
  // No initialization needed for local folder
  co_return;
}

corral::Task<bool>
LocalFolderGate::doesFolderExist(const CloudDestinationConfig &config) {
  try {
    return corral::just(fs::exists(config.cloudPath) &&
                        fs::is_directory(config.cloudPath));
  } catch (const std::exception &e) {
    SPDLOG_ERROR("Failed to check if local folder exists: {}", e.what());
    throw SqException("Failed to check if local folder exists: " +
                      std::string(e.what()));
  }
}

corral::Task<void>
LocalFolderGate::initFolder(const CloudDestinationConfig &config) {
  try {
    if (fs::exists(config.cloudPath)) {
      SPDLOG_ERROR("Local folder already exists: {}",
                   config.cloudPath.string());
      throw SqAlreadyExists("Local folder already exists: " +
                            config.cloudPath.string());
    }
    fs::create_directories(config.cloudPath);
  } catch (const std::exception &e) {
    SPDLOG_ERROR("Failed to create local folder: {}", e.what());
    throw SqException("Failed to create local folder: " +
                      std::string(e.what()));
  }
  co_return;
}

corral::Task<FileDetailsByFileName>
LocalFolderGate::listPatchFiles(const CloudDestinationConfig &config) {
  SPDLOG_INFO("Listing patch files in local folder: {}",
              config.cloudPath.string());
  try {
    FileDetailsByFileName fileList;
    for (const auto &entry : fs::directory_iterator(config.cloudPath)) {
      if (entry.is_regular_file()) {
        FileDetails fileDetails;
        fileDetails.fileName = entry.path().filename().string();
        fileDetails.fileSize = entry.file_size();
        fileDetails.lastModifiedTime = chrono::clock_cast<chrono::system_clock>(
            fs::last_write_time(entry.path()));
        fileList.insert_or_assign(fileDetails.fileName, fileDetails);
      }
    }
    return corral::just(fileList);

  } catch (const std::exception &e) {
    SPDLOG_ERROR("Failed to list patch files in local folder: {}", e.what());
    throw SqException("Failed to list patch files in local folder: " +
                      std::string(e.what()));
  }
}

corral::Task<void>
LocalFolderGate::uploadFiles(const CloudDestinationConfig &config,
                             const std::list<fs::path> &files) {
  // We don't care here about integrity
  SPDLOG_INFO("Uploading patch files to local folder: {}",
              config.cloudPath.string());
  for (const auto &file : files) {
    try {
      fs::copy(file, config.cloudPath / file.filename(),
               fs::copy_options::update_existing);
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Failed to upload patch file {} to local folder: {}",
                   file.string(), e.what());
      throw SqException("Failed to upload patch file " + file.string() +
                        " to local folder: " + std::string(e.what()));
    }
  }
  return corral::noop();
}

corral::Task<std::list<fs::path>>
LocalFolderGate::downloadFiles(const CloudDestinationConfig &config,
                               const std::list<std::string> &files,
                               const fs::path &destination) {
  // We don't care here about integrity
  std::list<fs::path> result;
  SPDLOG_INFO("Downloading patch files from local folder: {}",
              config.cloudPath.string());
  for (const auto &file : files) {
    auto dest = destination / file;
    try {
      fs::copy(config.cloudPath / file, dest,
               fs::copy_options::update_existing);
      result.push_back(dest);
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Failed to download patch file {} from local folder: {}",
                   file, e.what());
      throw SqException("Failed to download patch file " + file +
                        " from local folder: " + std::string(e.what()));
    }
  }
  return corral::just(result);
}

} // namespace synqueen::cloud
