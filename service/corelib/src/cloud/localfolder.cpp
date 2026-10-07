#include "localfolder.hpp"

namespace chrono = std::chrono;
namespace fs = std::filesystem;

namespace synqueen::cloud {

LocalFolder::LocalFolder(std::filesystem::path path) {
  folderPath = std::move(path);
}

corral::Task<FileDetailsList>
LocalFolder::listPatchFiles(const CloudDestinationConfig &config) {
  // TODO: use assert + reinterpret_cast instead of dynamic_cast for
  // performance. We don't really need RTTI here.
  auto &localConfig = dynamic_cast<const LocalFolderConfig &>(config);
  FileDetailsList fileList;
  for (const auto &entry : fs::directory_iterator(localConfig.folderPath)) {
    if (entry.is_regular_file()) {
      FileDetails fileDetails;
      fileDetails.fileName = entry.path().filename().string();
      fileDetails.fileSize = entry.file_size();
      fileDetails.lastModifiedTime = chrono::clock_cast<chrono::system_clock>(
          fs::last_write_time(entry.path()));
      fileList.push_back(fileDetails);
    }
  }
  return corral::just(fileList);
}

corral::Task<void>
LocalFolder::uploadFiles(const CloudDestinationConfig &config,
                         const std::list<std::filesystem::path> &files) {
  // We don't care here about integrity
  auto &localConfig = dynamic_cast<const LocalFolderConfig &>(config);
  for (const auto &file : files) {
    fs::copy(file, localConfig.folderPath / file.filename(),
             fs::copy_options::update_existing);
  }
  return corral::noop();
}

corral::Task<void>
LocalFolder::downloadFiles(const CloudDestinationConfig &config,
                           const std::list<std::string> &files,
                           const std::filesystem::path &destination) {
  // We don't care here about integrity
  auto &localConfig = dynamic_cast<const LocalFolderConfig &>(config);
  for (const auto &file : files) {
    fs::copy(localConfig.folderPath / file, destination / file,
             fs::copy_options::update_existing);
  }
  return corral::noop();
}

} // namespace synqueen::cloud
