#pragma once

#include "folderstate.hpp"
#include "patchexchange.hpp"
#include "settings.hpp"

#include <vector>

namespace synqueen {

class Synchronizer {
public:
  Synchronizer(uv_loop_t *loop);
  ~Synchronizer() = default;

  void loadSettings(const Settings &settings);
  void sync();

private:
  PatchExchange patchExchange;
  std::vector<FolderStatePtr> folderStates;
};

} // namespace synqueen
