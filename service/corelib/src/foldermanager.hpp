#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "ipatchstorage.hpp"
#include "patch/ipatchprovider.hpp"
#include "utils/corralheader.hpp"
#include "utils/standardpaths.hpp"

namespace synqueen {

class FolderManager {
public:
  explicit FolderManager(const std::string &id,
                         const std::filesystem::path &repoPath,
                         patch::IPatchProvider &patchProvider,
                         IPatchStorage &patchStorage, corral::Nursery &nursery);
  ~FolderManager() = default;

  void synchronize();

protected:
  void loadFolderState();

private:
  std::string id;
  std::filesystem::path repoPath;
  int nextOutgoingPatchIndex = 1;
  std::string lastOutgoingCommit;
  patch::IPatchProvider &patchProvider;
  IPatchStorage &patchStorage;
  corral::Nursery *nursery = nullptr;
  bool isSynchronizing = false;
};

typedef std::unique_ptr<FolderManager> FolderManagerPtr;

} // namespace synqueen
