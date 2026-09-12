#pragma once

#include "utils/corralheader.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace synqueen {

class IUiGateProvider {
public:
  struct Folder {
    std::filesystem::path path;
  };
  typedef std::vector<Folder> ListFolders;

  virtual ~IUiGateProvider() = default;

  virtual ListFolders listFolders() const = 0;
  virtual corral::Task<void> addFolder(const Folder &folder) = 0;
  virtual corral::Task<void>
  removeFolder(const std::filesystem::path &path) = 0;
};

} // namespace synqueen
