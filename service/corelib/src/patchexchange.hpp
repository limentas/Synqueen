#pragma once

#include <string>
#include <uv.h>

namespace synqueen {

class PatchExchange {
public:
  explicit PatchExchange(uv_loop_t *l);
  ~PatchExchange() = default;

  void ensureFolderInitialized(const std::string &folderPath);

private:
  void startHgServer();

private:
  uv_loop_t *loop = nullptr;
  uv_process_t *hgProcess = nullptr;
};

} // namespace synqueen
