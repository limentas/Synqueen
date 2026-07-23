#pragma once

#include "patchexchange.hpp"

#include <functional>

namespace synqueen {

class PatchBackend {
public:
  virtual ~PatchBackend() = default;

  typedef std::function<void(const PatchExchange::LocalStateResult &)>
      StateCallback;
  virtual void checkLocalState(const std::string &folderPath,
                               const StateCallback &callback) = 0;

  typedef std::function<void(const PatchExchange::PreparePatchResult &)>
      PreparePatchCallback;
  virtual void preparePatch(const std::string &folderPath,
                            const PreparePatchCallback &callback) = 0;

protected:
  enum class Command { CheckLocalState, PreparePatch };
};

} // namespace synqueen
