#pragma once

#include "hgprotocol.hpp"
#include "utils/corralhelpers.hpp"
#include "utils/uvutils.hpp"

#include <functional>
#include <list>
#include <optional>
#include <string>
#include <uv.h>

namespace synqueen {

template <typename ResultType> class Awaitable;

template <typename ResultType> class Completer {
public:
  inline bool isReady() const { return pendingResult.has_value(); }
  inline std::optional<ResultType> result() const { return pendingResult; }

  void reset() {
    pendingResult.reset();
    awaitableHandle = corral::Handle();
  }

  void completeCommand(ResultType result) {
    pendingResult = std::move(result);

    if (awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  friend class Awaitable<ResultType>;

  void setAwaitableHandle(corral::Handle handle) {
    awaitableHandle = handle;
    if (pendingResult.has_value() && awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  corral::Handle awaitableHandle;
  std::optional<ResultType> pendingResult;
};

template <> class Completer<void> {
public:
  inline bool isReady() const { return completed; }

  void reset() {
    completed = false;
    awaitableHandle = corral::Handle();
  }

  void completeCommand() {
    completed = true;

    if (awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  friend class Awaitable<void>;

  void setAwaitableHandle(corral::Handle handle) {
    awaitableHandle = handle;
    if (completed && awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  corral::Handle awaitableHandle;
  bool completed = false;
};

template <typename ResultType> class Awaitable {
public:
  Awaitable(Completer<ResultType> &c) : completer(c) {}

  bool await_ready() const noexcept { return completer.isReady(); }
  void await_suspend(corral::Handle h) { completer.setAwaitableHandle(h); }
  ResultType await_resume() { return completer.result().value(); }

private:
  Completer<ResultType> &completer;
};

template <> class Awaitable<void> {
public:
  Awaitable(Completer<void> &c) : completer(c) {}

  bool await_ready() const noexcept { return completer.isReady(); }
  void await_suspend(corral::Handle h) { completer.setAwaitableHandle(h); }
  void await_resume() {}

private:
  Completer<void> &completer;
};

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
