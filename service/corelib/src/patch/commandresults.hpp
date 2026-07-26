#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>

namespace synqueen {
namespace patch {

struct BaseResult {
  bool ok;
  std::string errorMessage;
};

struct LocalStateResult : public BaseResult {
  bool initialized;
  bool hasUncommittedChanges;
};

typedef std::function<void(const std::string &folderPath,
                           const LocalStateResult &result)>
    LocalStateCallback;
typedef std::shared_ptr<LocalStateCallback> LocalStateCallbackPtr;

struct PreparePatchResult : public BaseResult {
  std::list<std::string> patches;
};
typedef std::function<void(const std::string &folderPath,
                           const PreparePatchResult &result)>
    PreparePatchCallback;
typedef std::shared_ptr<PreparePatchCallback> PreparePatchCallbackPtr;

} // namespace patch
} // namespace synqueen
