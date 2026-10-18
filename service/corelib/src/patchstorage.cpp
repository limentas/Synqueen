#include "patchstorage.hpp"

#include "utils/logger.hpp"

#include <filesystem>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace synqueen {

PatchStorage::PatchStorage(const std::filesystem::path &basePath)
    : basePath(basePath) {
  createStorageDirectories();
  cleanAllInterimFiles();
  loadAllPatches();
}

void PatchStorage::movePatchesToApply(
    const std::list<std::filesystem::path> &patchPaths) {
  auto moved = false;
  for (const auto &patchPath : patchPaths) {
    auto fileName = patchPath.filename();
    auto newFilePath = basePath / PatchStorage::toApplyDir / fileName;
    if (moveFile(patchPath, newFilePath))
      moved = true;

    SPDLOG_INFO("Moved patch to apply: {}", newFilePath);
    patchesToApply.insert_or_assign(fileName.string(), newFilePath);
  }

  if (moved) {
    incomingNotifier.notify();
  }
}

void PatchStorage::stagePatchAsApplied(const std::filesystem::path &patchPath) {
  auto fileName = patchPath.filename();
  auto newFilePath = basePath / PatchStorage::appliedDir / fileName;
  moveFile(patchPath, newFilePath);

  auto fileNameStr = fileName.string();
  patchesApplied.insert_or_assign(fileNameStr, newFilePath);
  patchesToApply.erase(fileNameStr);

  SPDLOG_INFO("Staged patch as applied: {}", newFilePath);
  // We don't need to notify because the patch was already here
}

void PatchStorage::movePatchToOutgoing(const std::filesystem::path &patchPath,
                                       int index,
                                       const std::string &lastCommit) {
  auto fileName = formatPatchName(index, lastCommit);
  auto newFilePath = basePath / PatchStorage::outgoingDir / fileName;
  moveFile(patchPath, newFilePath);
  outgoingPatches.insert_or_assign(fileName,
                                   PatchFile{index, lastCommit, newFilePath});
  SPDLOG_INFO("Moved patch to outgoing: {}", newFilePath);
  outgoingNotifier.notify();
}

const IPatchStorage::PatchesByFileName &
PatchStorage::listOutgoingPatches() const {
  return outgoingPatches;
}

const IPatchStorage::PatchFilesByFileName &
PatchStorage::listPatchesToApply() const {
  return patchesToApply;
}

const IPatchStorage::PatchFilesByFileName &
PatchStorage::listAppliedPatches() const {
  return patchesApplied;
}

utils::ISubscribable &PatchStorage::getIncomingNotifier() {
  return incomingNotifier;
}

utils::ISubscribable &PatchStorage::getOutgoingNotifier() {
  return outgoingNotifier;
}

void PatchStorage::createStorageDirectories() {
  // Does nothing if the directories already exist
  // May throw an exception
  std::filesystem::create_directories(basePath / PatchStorage::toApplyDir);
  std::filesystem::create_directories(basePath / PatchStorage::appliedDir);
  std::filesystem::create_directories(basePath / PatchStorage::outgoingDir);
}

void PatchStorage::loadAllPatches() {
  loadPatchFiles(basePath / PatchStorage::toApplyDir, patchesToApply);
  loadPatchFiles(basePath / PatchStorage::appliedDir, patchesApplied);
  loadPatches(basePath / PatchStorage::outgoingDir, outgoingPatches);
}

bool PatchStorage::moveFile(const std::filesystem::path &src,
                            const std::filesystem::path &dst) {
  if (std::filesystem::exists(dst)) {
    SPDLOG_ERROR("PatchStorage: destination file already exists: {}", dst);
    return false;
  }
  try {
    // Try to rename the file directly first
    std::filesystem::rename(src, dst);
  } catch (const std::filesystem::filesystem_error &e) {
    SPDLOG_ERROR("PatchStorage: failed to move file from {} to {}. Error: {}",
                 src, dst, e.what());

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
                   src, dst, e.what());
      throw;
    }
  }
  return true;
}

void PatchStorage::cleanAllInterimFiles() {
  cleanInterimFiles(basePath / PatchStorage::toApplyDir);
  cleanInterimFiles(basePath / PatchStorage::appliedDir);
  cleanInterimFiles(basePath / PatchStorage::outgoingDir);
}

std::string PatchStorage::formatPatchName(int index,
                                          const std::string &lastCommit) {
  return std::to_string(index) + "_" + lastCommit + ".patch";
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

void PatchStorage::loadPatches(const std::filesystem::path &folder,
                               IPatchStorage::PatchesByFileName &patches) {
  fs::directory_iterator it(folder);
  for (const auto &entry : it) {
    if (entry.is_regular_file() && entry.path().extension() == ".patch") {
      PatchStorage::PatchFile patchFile;
      parsePatchName(entry.path(), patchFile.index, patchFile.lastCommit);
      patchFile.path = entry.path();
      patches.insert({entry.path().filename().string(), patchFile});
    }
  }
}

void PatchStorage::loadPatchFiles(const std::filesystem::path &folder,
                                  PatchFilesByFileName &patchFiles) {
  fs::directory_iterator it(folder);
  for (const auto &entry : it) {
    if (entry.is_regular_file() && entry.path().extension() == ".patch") {
      const auto &path = entry.path();
      patchFiles.insert({path.filename().string(), path});
    }
  }
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
