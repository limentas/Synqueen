#pragma once

#include "patchbackend.hpp"

#include "command.hpp"
#include "hgprotocol.hpp"

#include <functional>
#include <uv.h>

namespace synqueen {

class HgBackend : public PatchBackend {
public:
  HgBackend(uv_loop_t *l);
  virtual ~HgBackend() override;

  virtual void
  checkLocalState(const std::string &folderPath,
                  const patch::LocalStateCallbackPtr &callback) override;

  virtual void
  preparePatch(const std::string &folderPath,
               const patch::PreparePatchCallbackPtr &callback) override;

protected:
  void startHgProcess();
  void stopHgProcess();
  std::string sendHgCommand(const std::string &command);

  static void onProcessStdoutRead(uv_stream_t *stream, ssize_t nread,
                                  const uv_buf_t *buf);
  void onProcessStdoutRead(const char *data, ssize_t nread);
  static void onProcessStderrRead(uv_stream_t *stream, ssize_t nread,
                                  const uv_buf_t *buf);
  static void onProcessExit(uv_process_t *req, int64_t exit_status,
                            int term_signal);
  void onProcessExit(int64_t exit_status, int term_signal);

  void initPipe(uv_pipe_t *pipe, const char *name);

  void waitRunningLoop(uv_loop_t *loop, std::function<bool()> predicate,
                       int timeoutMs = 5000);

private:
  HgProtocol protocol;
  uv_loop_t *loop = nullptr;
  uv_process_t *process = nullptr;
  bool processStarted = false;
  uv_pipe_t hgStdin{};
  uv_pipe_t hgStdout{};
  uv_pipe_t hgStderr{};

  std::string repoFolder;
  patch::CommandType currentCommand;
  patch::LocalStateCallbackPtr checkCallback;
  patch::PreparePatchCallbackPtr prepareCallback;
};

} // namespace synqueen
