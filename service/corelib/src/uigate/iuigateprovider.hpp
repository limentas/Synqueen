#pragma once

#include "cloud/cloudtypes.hpp"
#include "utils/corralheader.hpp"

#include <filesystem>
#include <list>
#include <string>

namespace synqueen {

class IUiGateProvider {
public:
  struct Folder {
    std::filesystem::path path;
    std::list<cloud::CloudDestinationConfigPtr> cloudDestinations;
  };
  typedef std::list<Folder> ListFolders;

  virtual ~IUiGateProvider() = default;

  virtual ListFolders listFolders() const = 0;
  virtual corral::Task<void> addFolder(const Folder &folder) = 0;
  virtual corral::Task<void>
  removeFolder(const std::filesystem::path &path) = 0;
};

} // namespace synqueen
