#pragma once

#include <memory>
#include <string>

#include "patch/ipatchprovider.hpp"
#include "utils/corralheader.hpp"

namespace synqueen {

class FolderManager {
public:
  explicit FolderManager(const std::string &path, IPatchProvider &patchProvider,
                         corral::Nursery &nursery)
      : path(path), patchProvider(patchProvider), nursery(nursery) {}
  ~FolderManager() = default;

  void initialize();

  void check();

private:
  // TODO: Use std::filesystem::path for better path handling
  std::string path;
  IPatchProvider &patchProvider;
  corral::Nursery &nursery;
};

typedef std::shared_ptr<FolderManager> FolderManagerPtr;

} // namespace synqueen
