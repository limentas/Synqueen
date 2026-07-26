#pragma once

#include "patchbackend.hpp"

#include "commandresults.hpp"

#include <uv.h>

namespace synqueen {

class HgBackend : public PatchBackend {
public:
  HgBackend(uv_loop_t *l);
  virtual ~HgBackend() override = default;

  virtual void
  checkLocalState(const std::string &folderPath,
                  const patch::LocalStateCallbackPtr &callback) override;

  virtual void
  preparePatch(const std::string &folderPath,
               const patch::PreparePatchCallbackPtr &callback) override;

protected:
  void startHgServer();
  std::string sendHgCommand(const std::string &command);

  void onProcessExit(int64_t exit_status, int term_signal);
  friend void on_exit(uv_process_t *req, int64_t exit_status, int term_signal);

private:
  uv_loop_t *loop = nullptr;
  uv_process_t *hgProcess = nullptr;
  uv_pipe_t hgStdin{};
  uv_pipe_t hgStdout{};
  uv_pipe_t hgStderr{};
  bool pipesInitialized = false;
};

} // namespace synqueen
