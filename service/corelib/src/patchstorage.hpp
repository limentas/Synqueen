#pragma once

#include <filesystem>
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
  // Stores the patch as to be applied
  virtual void
  movePatchToApply(const std::filesystem::path &patchPath) override;
  // Marks the patch as applied
  virtual void
  stagePatchAsApplied(const std::filesystem::path &patchPath) override;
  // Stores the patch as outgoing (means that it is created locally and should
  // be uploaded)
  virtual void
  movePatchToOutgoing(const std::filesystem::path &patchPath) override;

  virtual const std::map<int, PatchFile> &listPatches() const override;

  // Notifies when an incoming patch added or applied
  virtual utils::ISubscribable &getIncomingNotifier() override;
  // Notifies when an outgoing patch added
  virtual utils::ISubscribable &getOutgoingNotifier() override;

private:
  void createStorageDirectories();
  void parsePatchName(const std::filesystem::path &patchPath, int &index,
                      std::string &lastCommit);
  void moveFile(const std::filesystem::path &src,
                const std::filesystem::path &dst);
  void cleanAllInterimFiles();
  void cleanInterimFiles(const std::filesystem::path &directory);

private:
  static constexpr const char *toApplyDir = "to_apply";
  static constexpr const char *appliedDir = "applied";
  static constexpr const char *outgoingDir = "outgoing";

  static constexpr const char *interimSuffix = ".interim";

  std::filesystem::path basePath;
  utils::Notifier incomingNotifier;
  utils::Notifier outgoingNotifier;

  std::map<int, PatchFile> patches;
};

typedef std::unique_ptr<PatchStorage> PatchStoragePtr;

} // namespace synqueen