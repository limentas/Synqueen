#pragma once

#include "utils/notifier.hpp"
#include <filesystem>
#include <list>
#include <map>
#include <string>

namespace synqueen {

class IPatchStorage {
public:
  virtual ~IPatchStorage() = default;

  // There are globally two sorts of patches by origin:
  // - Incoming: patches that are received from a remote
  // - Outgoing: patches that are created locally
  // And Incoming patches can be just downloaded and not yet applied
  // and already applied.
  struct PatchFile {
    int index;
    std::string lastCommit;
    std::filesystem::path path;
  };

  typedef std::map<std::string, PatchFile> PatchesByFileName;
  typedef std::map<std::string, std::filesystem::path> PatchFilesByFileName;

  // Stores the patches as to be applied
  virtual void
  movePatchesToApply(const std::list<std::filesystem::path> &patchPaths) = 0;
  // Marks the patch as applied
  virtual void stagePatchAsApplied(const std::filesystem::path &patchPath) = 0;
  // Stores the patch as outgoing (means that it is created locally and should
  // be uploaded)
  virtual void movePatchToOutgoing(const std::filesystem::path &patchPath,
                                   int index,
                                   const std::string &lastCommit) = 0;

  virtual const PatchesByFileName &listOutgoingPatches() const = 0;
  virtual const PatchFilesByFileName &listPatchesToApply() const = 0;
  virtual const PatchFilesByFileName &listAppliedPatches() const = 0;

  // Notifies when an incoming patch added or applied
  virtual utils::ISubscribable &getIncomingNotifier() = 0;
  // Notifies when an outgoing patch added
  virtual utils::ISubscribable &getOutgoingNotifier() = 0;
};

} // namespace synqueen
