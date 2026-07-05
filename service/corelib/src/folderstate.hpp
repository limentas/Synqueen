#pragma once

#include <memory>
#include <string>

namespace synqueen {

class FolderState {
public:
  explicit FolderState(const std::string &path) : filesystemPath(path) {}
  ~FolderState() = default;

  void initialize();

private:
  void load();
  void ensureInternalFolderExists();
  void ensureVCSInitialized();

private:
  std::string filesystemPath;
};

typedef std::shared_ptr<FolderState> FolderStatePtr;

} // namespace synqueen
