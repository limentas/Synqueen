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
                         IPatchStorage &patchStorage)
      : id(id), repoPath(repoPath), patchProvider(patchProvider),
        patchStorage(patchStorage),
        dataPath(StandardPaths::getDataPath() / "folders" / id),
        incomingPatchesPath(dataPath / "incoming"),
        outgoingPatchesPath(dataPath / "outgoing") {}
  ~FolderManager() = default;

  void loadState(corral::Nursery &nursery);
  void setupState(corral::Nursery &nursery);

  void synchronize();

protected:
  void createDataPaths();
  void loadFolderState();

  void addOutgoingPatch(const std::filesystem::path &patchFile,
                        const std::string &lastIncludedCommit);

  std::list<std::string> loadPatches(const std::filesystem::path &path);
  void parsePatchName(const std::string &patchName, int &patchIndex,
                      std::string &commit);

private:
  std::string id;
  std::filesystem::path repoPath;
  std::filesystem::path dataPath;
  std::filesystem::path incomingPatchesPath;
  std::filesystem::path outgoingPatchesPath;
  std::list<std::string> outgoingPatches;
  std::list<std::string> incomingPatches;
  int nextOutgoingPatchIndex = 1;
  std::string lastIncludedCommit;
  patch::IPatchProvider &patchProvider;
  IPatchStorage &patchStorage;
  corral::Nursery *nursery = nullptr;
  bool isSynchronizing = false;
};

typedef std::unique_ptr<FolderManager> FolderManagerPtr;

} // namespace synqueen
