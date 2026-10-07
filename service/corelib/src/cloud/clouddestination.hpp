#pragma once

#include <memory>

#include "ipatchstorage.hpp"

namespace synqueen::cloud {

// This class monitors changes in Patch storage and synchronizes them with the
// cloud destination. One Folder can have multiple destinations.
class CloudDestination {
public:
  CloudDestination(IPatchStorage &patchStorage);
  virtual ~CloudDestination() = default;

  void synchronize();

private:
  IPatchStorage &patchStorage;
};

typedef std::unique_ptr<CloudDestination> CloudDestinationPtr;

} // namespace synqueen::cloud