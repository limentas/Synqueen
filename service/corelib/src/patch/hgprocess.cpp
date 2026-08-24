#include "hgprocess.hpp"

#include "utils/compilerwarnings.hpp"
#include "utils/uvutils.hpp"

#include <chrono>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread>

using namespace std;
using namespace chrono_literals;

namespace synqueen {

HgProcess::HgProcess(uv_loop_t *l)
    : loop(l), hgStdin(nullptr, deletePipe), hgStdout(nullptr, deletePipe),
      hgStderr(nullptr, deletePipe) {}

HgProcess::~HgProcess() {
  // There is no way to close the process synchronously, so shutdown is
  // essential. Here we can only make sure that it was called.
  // We cannot just free libuv handles because they have to be closed before,
  // otherwise libuv references them and may use them after free.
  assert(process == nullptr && "HgProcess must be shutdown before destruction");
  assert(!commandInFlight &&
         "There must be no command in flight when destroying HgProcess");
}

corral::Task<HgProtocol::CommandResult>
synqueen::HgProcess::runCommand(const std::list<std::string> &args) {
  if (commandInFlight) {
    throw runtime_error("Another hg command is already in flight");
  }

  commandInFlight = true;
  commandCompleter.reset();

  auto cmd = protocol.prepareCommand(args);
  try {
    co_await tryStartHgProcess();

    uv_buf_t buffer[] = {{.len = static_cast<unsigned long>(cmd.size()),
                          .base = const_cast<char *>(cmd.data())}};

    auto req = new uv_write_t();
    auto writeStatus =
        uv_write(req, reinterpret_cast<uv_stream_t *>(hgStdin.get()), buffer, 1,
                 [](uv_write_t *req, int status) {
                   if (status < 0) {
                     spdlog::error("Failed to write to hg cmdserver. Error: {}",
                                   uv_strerror(status));
                   }
                   delete req;
                 });

    if (writeStatus < 0) {
      throw runtime_error("Failed to write command to hg cmdserver. Error: " +
                          string(uv_strerror(writeStatus)));
    }
  } catch (...) {
    commandInFlight = false;
    throw;
  }

  co_return co_await Awaitable<HgProtocol::CommandResult>(commandCompleter);
}

corral::Task<void> HgProcess::shutdown() { co_await stopHgProcess(); }

corral::Task<void> HgProcess::tryStartHgProcess() {
  if (process != nullptr)
    co_return; // Already started

  co_await startHgProcess();
}

corral::Task<void> HgProcess::startHgProcess() {
  protocol.reset();
  processStartCompleter.reset();

  initPipe(hgStdin, "stdin");
  initPipe(hgStdout, "stdout");
  initPipe(hgStderr, "stderr");

  uv_stdio_container_t stdio[3];
  stdio[0].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
  stdio[0].data.stream = reinterpret_cast<uv_stream_t *>(hgStdin.get());
  stdio[1].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[1].data.stream = reinterpret_cast<uv_stream_t *>(hgStdout.get());
  stdio[2].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[2].data.stream = reinterpret_cast<uv_stream_t *>(hgStderr.get());

  process = new uv_process_t();
  process->data = this; // Store the HgProcess instance in the process data
  uv_process_options_t options = {0};
  options.exit_cb = onProcessExit;
  options.file =
      "C:\\Program Files\\Mercurial\\hg"; // TODO: Make this configurable or
                                          // find hg in PATH
  const char *args[] = {"hg",          "--config", "ui.interactive=True",
                        "--encoding",  "UTF-8",    "serve",
                        "--cmdserver", "pipe",     nullptr};
  options.args = const_cast<char **>(args);
  options.env = nullptr;
  options.cwd = nullptr;
  options.flags = UV_PROCESS_WINDOWS_HIDE | UV_PROCESS_WINDOWS_HIDE_CONSOLE |
                  UV_PROCESS_WINDOWS_HIDE_GUI;
  options.stdio_count = 3;
  options.stdio = stdio;

  auto err = uv_spawn(loop, process, &options);
  if (err < 0) {
    auto errMsg = string(uv_strerror(err));
    delete process;
    process = nullptr;
    throw runtime_error("Failed to start hg cmdserver. Error: " + errMsg);
  }

  err = uv_read_start(
      reinterpret_cast<uv_stream_t *>(hgStdout.get()),
      [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
      },
      onProcessStdoutRead);

  if (err < 0) {
    auto errMsg = string(uv_strerror(err));
    throw runtime_error("Failed to start reading from hg cmdserver. Error: " +
                        errMsg);
  }

  err = uv_read_start(
      reinterpret_cast<uv_stream_t *>(hgStderr.get()),
      [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
      },
      onProcessStderrRead);
  if (err < 0) {
    auto errMsg = string(uv_strerror(err));
    throw runtime_error(
        "Failed to start reading from hg cmdserver stderr. Error: " + errMsg);
  }

  // Wait for the process to start
  // waitRunningLoop(loop, [this]() { return !processStarted; }, 2000);
  co_await Awaitable<void>(processStartCompleter);
  co_return;
}

corral::Task<void> HgProcess::stopHgProcess() {
  if (process == nullptr)
    co_return; // Already stopped

  // Try to gracefully shutdown the process at first
  auto result = uv_process_kill(process, SIGINT);
  if (result < 0) {
    spdlog::warn("Failed to send SIGINT to hg process. Error: {}",
                 uv_strerror(result));
  } else {
    spdlog::debug("Sent SIGINT to hg process");
  }

  processStopCompleter.reset();

  TimerPtr timerPtr(new uv_timer_t(), deleteHandle<uv_timer_t>);
  timerPtr->data = this;
  uv_timer_init(loop, timerPtr.get());
  uv_timer_start(
      timerPtr.get(),
      [](uv_timer_t *handle) {
        auto instance = reinterpret_cast<HgProcess *>(handle->data);
        instance->processStopCompleter.completeCommand();
      },
      2000, 0);
  co_await Awaitable<void>(processStopCompleter);
  uv_timer_stop(timerPtr.get());

  if (process == nullptr)
    co_return;

  result = uv_process_kill(process, SIGKILL);
  if (result < 0) {
    spdlog::warn("Failed to send SIGKILL to hg process. Error: {}",
                 uv_strerror(result));
    co_return;
  } else {
    spdlog::debug("Sent SIGKILL to hg process");
  }

  processStopCompleter.reset();
  uv_timer_set_repeat(timerPtr.get(), 2000);
  uv_timer_again(timerPtr.get());
  co_await Awaitable<void>(processStopCompleter);
}

void HgProcess::initPipe(PipePtr &pipe, const char *name) {
  pipe = PipePtr(new uv_pipe_t(), deletePipe);
  pipe->data = this;
  int result = uv_pipe_init(loop, pipe.get(), 0);
  if (result < 0) {
    throw runtime_error("Failed to initialize child pipe " + string(name) +
                        ". Error: " + string(uv_strerror(result)));
  }
  uv_handle_set_data(reinterpret_cast<uv_handle_t *>(pipe.get()), this);
}

void HgProcess::waitRunningLoop(uv_loop_t *loop,
                                std::function<bool()> predicate,
                                int timeoutMs) {
  const auto step = 100ms;
  auto iterations = max(timeoutMs / step.count(), 1);
  while (predicate() && iterations-- > 0) {
    uv_run(loop, UV_RUN_NOWAIT); // Process any pending events
    if (!predicate())
      break;
    this_thread::sleep_for(step);
  }
}

void HgProcess::onProcessStdoutRead(uv_stream_t *stream, ssize_t nread,
                                    const uv_buf_t *buf) {
  if (nread > 0) {
    try {
      auto instance = reinterpret_cast<HgProcess *>(stream->data);
      instance->onProcessStdoutRead(buf->base, nread);
    } catch (const exception &e) {
      spdlog::error("Error parsing hg output: {}", e.what());
    }
  } else if (nread < 0) {
    if (nread != UV_EOF) {
      // Handle read error
    }
    uv_close((uv_handle_t *)stream, NULL);
  }
  delete[] buf->base;
}

void HgProcess::onProcessStdoutRead(const char *data, ssize_t nread) {
  auto result = protocol.feedStdOutput(data, nread);
  // Process started and sent Hello message
  if (protocol.getHelloMessage().pid != 0 && !processStartCompleter.isReady()) {
    processStartCompleter.completeCommand();
  }
  if (!result.has_value()) // Result not ready yet, continue reading
    return;
  auto commandResult = result.value();
  spdlog::info("Received command result: output='{}', error='{}', debug='{}', "
               "resultCode={}, requiresInput={}, requiredInputSize={}",
               commandResult.output, commandResult.error, commandResult.debug,
               commandResult.resultCode, commandResult.requiresInput,
               commandResult.requiredInputSize);

  if (commandResult.requiresInput) {
    commandResult.error +=
        "Hg cmdserver requested interactive input, but this is not "
        "implemented.";
    commandResult.resultCode = -1;
  }

  commandInFlight = false;
  commandCompleter.completeCommand(std::move(commandResult));
}

void HgProcess::onProcessStderrRead(uv_stream_t *stream, ssize_t nread,
                                    const uv_buf_t *buf) {
  if (nread > 0) {
    spdlog::debug("hg cmdserver stderr: {}", string(buf->base, nread));
  } else if (nread < 0) {
    if (nread != UV_EOF) {
      // Handle read error
    }
    uv_close((uv_handle_t *)stream, NULL);
  }
  delete[] buf->base;
}

void HgProcess::onProcessExit(uv_process_t *req, int64_t exit_status,
                              int term_signal) {
  auto instance = reinterpret_cast<HgProcess *>(req->data);
  instance->onProcessExit(exit_status, term_signal);
}

void HgProcess::onProcessExit(int64_t exit_status, int term_signal) {
  if (exit_status != 0) {
    spdlog::error("hg cmdserver exited with status {} and signal {}",
                  exit_status, term_signal);
  } else {
    spdlog::info("hg cmdserver exited successfully");
  }

  if (commandInFlight && !commandCompleter.isReady()) {
    HgProtocol::CommandResult commandResult;
    commandResult.resultCode = static_cast<int>(exit_status);
    commandResult.error = "hg cmdserver exited before returning command "
                          "result";
    commandInFlight = false;
    commandCompleter.completeCommand(std::move(commandResult));
  }

  uv_read_stop(reinterpret_cast<uv_stream_t *>(hgStdout.get()));
  uv_read_stop(reinterpret_cast<uv_stream_t *>(hgStderr.get()));

  hgStdin.reset();
  hgStdout.reset();
  hgStderr.reset();

  uv_close(reinterpret_cast<uv_handle_t *>(process), [](uv_handle_t *h) {
    auto instance = reinterpret_cast<HgProcess *>(h->data);
    delete instance->process;
    instance->process = nullptr;
    if (!instance->processStopCompleter.isReady()) {
      instance->processStopCompleter.completeCommand();
    }
  });
}

} // namespace synqueen
