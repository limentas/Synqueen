#pragma once

#include <filesystem>
#include <list>
#include <map>

#include "ipatchstorage.hpp"
#include "utils/notifier.hpp"

namespace synqueen {

// This implementation just stores patch files in different folders according to
// their kind (to apply, applied, outgoing). It doesn't have any other
// persistent state other than patch files.
class PatchStorage : public IPatchStorage {
public:
  PatchStorage(const std::filesystem::path &basePath);
  virtual ~PatchStorage() = default;

public:
  virtual void movePatchesToApply(
      const std::list<std::filesystem::path> &patchPaths) override;
  virtual void
  stagePatchAsApplied(const std::filesystem::path &patchPath) override;
  virtual void movePatchToOutgoing(const std::filesystem::path &patchPath,
                                   int index,
                                   const std::string &lastCommit) override;

  virtual const PatchesByFileName &listOutgoingPatches() const override;
  virtual const PatchFilesByFileName &listPatchesToApply() const override;
  virtual const PatchFilesByFileName &listAppliedPatches() const override;

  virtual utils::ISubscribable &getIncomingNotifier() override;
  virtual utils::ISubscribable &getOutgoingNotifier() override;

private:
  void createStorageDirectories();
  void loadAllPatches();
  bool moveFile(const std::filesystem::path &src,
                const std::filesystem::path &dst);
  void cleanAllInterimFiles();

  static std::string formatPatchName(int index, const std::string &lastCommit);
  static void parsePatchName(const std::filesystem::path &patchPath, int &index,
                             std::string &lastCommit);
  static void loadPatches(const std::filesystem::path &folder,
                          PatchesByFileName &patches);
  static void loadPatchFiles(const std::filesystem::path &folder,
                             PatchFilesByFileName &patchFiles);
  static void cleanInterimFiles(const std::filesystem::path &directory);

private:
  static constexpr const char *toApplyDir = "to_apply";
  static constexpr const char *appliedDir = "applied";
  static constexpr const char *outgoingDir = "outgoing";

  static constexpr const char *interimSuffix = ".interim";

  std::filesystem::path basePath;
  utils::Notifier incomingNotifier;
  utils::Notifier outgoingNotifier;

  PatchesByFileName outgoingPatches;
  PatchFilesByFileName patchesToApply;
  PatchFilesByFileName patchesApplied;
};

typedef std::unique_ptr<PatchStorage> PatchStoragePtr;

} // namespace synqueen