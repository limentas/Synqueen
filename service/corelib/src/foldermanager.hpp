#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "patch/ipatchprovider.hpp"
#include "utils/corralheader.hpp"
#include "utils/standardpaths.hpp"

namespace synqueen {

class FolderManager {
public:
  explicit FolderManager(const std::string &id,
                         const std::filesystem::path &repoPath,
                         IPatchProvider &patchProvider)
      : id(id), repoPath(repoPath), patchProvider(patchProvider),
        dataPath(StandardPaths::getDataPath() / "folders" / id),
        incomingPatchesPath(dataPath / "incoming"),
        outgoingPatchesPath(dataPath / "outgoing") {}
  ~FolderManager() = default;

  void initialize(corral::Nursery &nursery);

  void createDataPaths();

  void addOutgoingPatch(const std::filesystem::path &patchFile,
                        const std::string &lastIncludedCommit);

  void checkForChanges();

protected:
  void loadFolderState();

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
  IPatchProvider &patchProvider;
  corral::Nursery *nursery = nullptr;
};

typedef std::shared_ptr<FolderManager> FolderManagerPtr;

} // namespace synqueen
