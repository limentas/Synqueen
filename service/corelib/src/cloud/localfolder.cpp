#include "localfolder.hpp"

namespace chrono = std::chrono;
namespace fs = std::filesystem;

namespace synqueen {

LocalFolder::LocalFolder(std::filesystem::path path) {
  folderPath = std::move(path);
}

synqueen::FileDetailsList
LocalFolder::listPatchFiles(synqueen::CloudConfig &config,
                            const std::string &path) {
  synqueen::FileDetailsList fileList;
  for (const auto &entry : fs::directory_iterator(folderPath / path)) {
    if (entry.is_regular_file()) {
      synqueen::FileDetails fileDetails;
      fileDetails.fileName = entry.path().filename().string();
      fileDetails.fileSize = entry.file_size();
      fileDetails.lastModifiedTime = chrono::clock_cast<chrono::system_clock>(
          fs::last_write_time(entry.path()));
      fileList.push_back(fileDetails);
    }
  }
  return fileList;
}

} // namespace synqueen
