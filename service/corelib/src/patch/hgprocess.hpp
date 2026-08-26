#pragma once

#include "hgprotocol.hpp"
#include "utils/corralheader.hpp"
#include "utils/corralutils.hpp"
#include "utils/uvutils.hpp"

#include <functional>
#include <list>
#include <optional>
#include <string>
#include <uv.h>

namespace synqueen {

class HgProcess {
public:
  HgProcess(uv_loop_t *l);
  ~HgProcess();

  corral::Task<HgProtocol::CommandResult>
  runCommand(const std::list<std::string> &args);

  // Shutdown the hg process gracefully. It is required to call this before
  // destroying the HgProcess instance.
  corral::Task<void> shutdown();

private:
  corral::Task<void> tryStartHgProcess();
  corral::Task<void> startHgProcess();
  corral::Task<void> stopHgProcess();

  void initPipe(PipePtr &pipe, const char *name);

  void waitRunningLoop(uv_loop_t *loop, std::function<bool()> predicate,
                       int timeoutMs = 5000);

  static void onProcessStdoutRead(uv_stream_t *stream, ssize_t nread,
                                  const uv_buf_t *buf);
  void onProcessStdoutRead(const char *data, ssize_t nread);
  static void onProcessStderrRead(uv_stream_t *stream, ssize_t nread,
                                  const uv_buf_t *buf);
  static void onProcessExit(uv_process_t *req, int64_t exit_status,
                            int term_signal);
  void onProcessExit(int64_t exit_status, int term_signal);

private:
  HgProtocol protocol;

  uv_loop_t *loop = nullptr;

  uv_process_t *process = nullptr;
  bool processStarted = false;
  bool commandInFlight = false;

  Completer<void> processStartCompleter;
  Completer<void> processStopCompleter;
  Completer<HgProtocol::CommandResult> commandCompleter;

  PipePtr hgStdin;
  PipePtr hgStdout;
  PipePtr hgStderr;

  corral::Handle awaitableHandle;
};

} // namespace synqueen
