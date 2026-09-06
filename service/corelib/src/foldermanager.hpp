#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "patch/ipatchprovider.hpp"
#include "utils/corralheader.hpp"

namespace synqueen {

class FolderManager {
public:
  explicit FolderManager(const std::filesystem::path &path,
                         IPatchProvider &patchProvider)
      : path(path), patchProvider(patchProvider) {}
  ~FolderManager() = default;

  void initialize(corral::Nursery &nursery);

  void check();

private:
  std::filesystem::path path;
  IPatchProvider &patchProvider;
  corral::Nursery *nursery = nullptr;
};

typedef std::shared_ptr<FolderManager> FolderManagerPtr;

} // namespace synqueen
