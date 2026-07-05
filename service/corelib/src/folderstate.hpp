#pragma once

#include <string>

namespace synqueen {

class FolderState {
public:
  FolderState() = default;
  ~FolderState() = default;

  void update();

private:
  void initialize();
  void load();
  void ensureInternalFolderExists();
  void ensureVCSInitialized();

private:
  std::string filesystemPath;
};

} // namespace synqueen
