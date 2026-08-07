#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>

namespace synqueen {
namespace patch {

enum class CommandType { CheckLocalState, PreparePatch };

struct BaseResult {
  bool ok = false;
  std::string errorMessage;
};

struct LocalStateResult : public BaseResult {
  bool initialized = false;
  bool hasUncommittedChanges = false;
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
