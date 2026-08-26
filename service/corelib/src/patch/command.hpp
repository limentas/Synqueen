#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>

namespace synqueen {
namespace patch {

struct BaseResult {
  bool ok = false;
  std::string errorMessage;
};

struct LocalStateResult : public BaseResult {
  bool initialized = false;
  bool hasUncommittedChanges = false;
  bool hasConflicts = false;
};

struct PreparePatchResult : public BaseResult {
  std::list<std::string> patches;
};

} // namespace patch
} // namespace synqueen
