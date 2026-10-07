#include "patchstorage.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

namespace synqueen {

PatchStorage::PatchStorage(const std::filesystem::path &basePath)
    : basePath(basePath) {
  createStorageDirectories();
  cleanAllInterimFiles();
}

void PatchStorage::movePatchToApply(const std::filesystem::path &patchPath) {
  int index;
  std::string lastCommit;
  parsePatchName(patchPath, index, lastCommit);

  auto newFilePath = basePath / PatchStorage::toApplyDir / patchPath.filename();
  moveFile(patchPath, newFilePath);

  patches[index] =
      PatchFile{index, lastCommit, newFilePath, PatchKind::ToApply};
  incomingNotifier.notify();
}

void PatchStorage::stagePatchAsApplied(const std::filesystem::path &patchPath) {
  int index;
  std::string lastCommit;
  parsePatchName(patchPath, index, lastCommit);

  auto newFilePath = basePath / PatchStorage::appliedDir / patchPath.filename();
  moveFile(patchPath, newFilePath);

  patches[index] =
      PatchFile{index, lastCommit, newFilePath, PatchKind::Applied};
  incomingNotifier.notify();
}

void PatchStorage::movePatchToOutgoing(const std::filesystem::path &patchPath) {
  int index;
  std::string lastCommit;
  parsePatchName(patchPath, index, lastCommit);

  auto newFilePath =
      basePath / PatchStorage::outgoingDir / patchPath.filename();
  moveFile(patchPath, newFilePath);

  patches[index] =
      PatchFile{index, lastCommit, newFilePath, PatchKind::Outgoing};
  outgoingNotifier.notify();
}

const std::map<int, PatchStorage::PatchFile> &
PatchStorage::listPatches() const {
  return patches;
}

utils::ISubscribable &PatchStorage::getIncomingNotifier() {
  return incomingNotifier;
}

utils::ISubscribable &PatchStorage::getOutgoingNotifier() {
  return outgoingNotifier;
}

void PatchStorage::createStorageDirectories() {
  std::filesystem::create_directories(basePath / PatchStorage::toApplyDir);
  std::filesystem::create_directories(basePath / PatchStorage::appliedDir);
  std::filesystem::create_directories(basePath / PatchStorage::outgoingDir);
}

void PatchStorage::parsePatchName(const std::filesystem::path &patchPath,
                                  int &index, std::string &lastCommit) {
  // Patch name format: "index_lastCommit.patch"
  auto patchName = patchPath.stem().string();
  auto underscorePos = patchName.find('_');
  if (underscorePos == std::string::npos) {
    throw std::invalid_argument("Invalid patch name format: " + patchName);
  }
  index = std::stoi(patchName.substr(0, underscorePos));
  lastCommit = patchName.substr(underscorePos + 1);
}

void PatchStorage::moveFile(const std::filesystem::path &src,
                            const std::filesystem::path &dst) {
  if (std::filesystem::exists(dst)) {
    SPDLOG_ERROR("PatchStorage: destination file already exists: {}",
                 dst.string());
    return;
  }
  try {
    // Try to rename the file directly first
    std::filesystem::rename(src, dst);
  } catch (const std::filesystem::filesystem_error &e) {
    SPDLOG_ERROR("PatchStorage: failed to move file from {} to {}. Error: {}",
                 src.string(), dst.string(), e.what());

    // Try to copy the file as a fallback
    // At first copy with interim suffix and then rename to the final
    // destination to not have partially copied files.
    try {
      if (std::filesystem::copy_file(
              src, dst.string() + PatchStorage::interimSuffix,
              std::filesystem::copy_options::skip_existing)) {
        std::filesystem::rename(dst.string() + PatchStorage::interimSuffix,
                                dst);
        std::filesystem::remove(src);
      }
    } catch (const std::filesystem::filesystem_error &e) {
      SPDLOG_ERROR("PatchStorage: failed to copy file from {} to {}. Error: {}",
                   src.string(), dst.string(), e.what());
      throw;
    }
  }
}

void PatchStorage::cleanAllInterimFiles() {
  cleanInterimFiles(basePath / PatchStorage::toApplyDir);
  cleanInterimFiles(basePath / PatchStorage::appliedDir);
  cleanInterimFiles(basePath / PatchStorage::outgoingDir);
}

void PatchStorage::cleanInterimFiles(const std::filesystem::path &directory) {
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() &&
        entry.path().extension() == PatchStorage::interimSuffix) {
      std::filesystem::remove(entry.path());
    }
  }
}

} // namespace synqueen
