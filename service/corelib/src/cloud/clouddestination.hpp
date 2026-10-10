#pragma once

#include "cloudtypes.hpp"
#include "icloudgate.hpp"
#include "ipatchstorage.hpp"

#include <list>
#include <memory>

namespace synqueen::cloud {

// This class monitors changes in Patch storage and synchronizes them with the
// cloud destination. One Folder can have multiple destinations.
class CloudDestination {
public:
  CloudDestination(ICloudGate &cloudGate, const CloudDestinationConfig &config,
                   IPatchStorage &patchStorage);
  virtual ~CloudDestination() = default;

  void initialize(corral::Nursery &nursery);

  void synchronize();

private:
  corral::Task<void> pullPatches(const std::list<std::string> &patches);

private:
  ICloudGate &cloudGate;
  const CloudDestinationConfig &config;
  IPatchStorage &patchStorage;
  corral::Nursery *nursery = nullptr;
};

typedef std::unique_ptr<CloudDestination> CloudDestinationPtr;

} // namespace synqueen::cloud
