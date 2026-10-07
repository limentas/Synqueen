#pragma once

#include <filesystem>
#include <functional>
#include <list>
#include <memory>
#include <string>

namespace synqueen::patch {

struct BaseResult {};

struct LocalStateResult : public BaseResult {
  bool initialized = false;
  bool hasUncommittedChanges = false;
  bool hasConflicts = false;
  std::string lastCommitHash;
};

struct InitRepoResult : public BaseResult {
  std::string lastCommitHash;
};

struct PreparePatchResult : public BaseResult {
  std::filesystem::path patchFilePath;
  std::string lastIncludedCommitHash;
};

struct ApplyPatchResult : public BaseResult {
  std::string lastIncludedCommitHash;
  bool hasConflicts = false;
};

} // namespace synqueen::patch
