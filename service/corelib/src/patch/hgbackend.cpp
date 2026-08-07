#include "hgbackend.hpp"

#include <algorithm>
#include <chrono>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread>

#include "hgprotocol.hpp"
#include "utils/uvutils.hpp"

using namespace std;
using namespace chrono_literals;

namespace synqueen {

using namespace patch;

synqueen::HgBackend::HgBackend(uv_loop_t *l) : loop(l) {}

HgBackend::~HgBackend() { stopHgProcess(); }

void HgBackend::checkLocalState(const string &folderPath,
                                const LocalStateCallbackPtr &callback) {
  currentCommand = CommandType::CheckLocalState;
  repoFolder = folderPath;
  checkCallback = callback;
  startHgProcess();
  sendHgCommand(
      protocol.prepareCommand({"summary", "--repository", folderPath}));
}

void HgBackend::preparePatch(const string &folderPath,
                             const PreparePatchCallbackPtr &callback) {
  currentCommand = CommandType::PreparePatch;
  repoFolder = folderPath;
  prepareCallback = callback;
  startHgProcess();
  // TODO: Implement preparePatch logic
}

void HgBackend::startHgProcess() {
  if (process != nullptr)
    return; // Already started

  protocol.reset();
  processStarted = false;

  initPipe(&hgStdin, "stdin");
  initPipe(&hgStdout, "stdout");
  initPipe(&hgStderr, "stderr");

  uv_stdio_container_t stdio[3];
  stdio[0].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
  stdio[0].data.stream = reinterpret_cast<uv_stream_t *>(&hgStdin);
  stdio[1].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[1].data.stream = reinterpret_cast<uv_stream_t *>(&hgStdout);
  stdio[2].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[2].data.stream = reinterpret_cast<uv_stream_t *>(&hgStderr);

  process = new uv_process_t();
  process->data = this; // Store the HgBackend instance in the process data
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
    uv_close(reinterpret_cast<uv_handle_t *>(process), [](uv_handle_t *handle) {
      delete reinterpret_cast<uv_process_t *>(handle);
    });
    process = nullptr;
    throw runtime_error("Failed to start hg cmdserver. Error: " + errMsg);
  }

  err = uv_read_start(
      reinterpret_cast<uv_stream_t *>(&hgStdout),
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
      reinterpret_cast<uv_stream_t *>(&hgStderr),
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

  // Wait for the process to exit
  waitRunningLoop(loop, [this]() { return !processStarted; }, 2000);
}

void HgBackend::stopHgProcess() {
  if (process == nullptr)
    return; // Already stopped

  // Try to gracefully shutdown the process at first
  auto result = uv_process_kill(process, SIGINT);
  if (result < 0) {
    spdlog::warn("Failed to send SIGINT to hg process. Error: {}",
                 uv_strerror(result));
  } else {
    spdlog::debug("Sent SIGINT to hg process");
  }

  // Wait for the process to exit
  waitRunningLoop(loop, [this]() { return process != nullptr; }, 2000);

  if (process == nullptr)
    return;

  // Force kill if it doesn't finish
  result = uv_process_kill(process, SIGKILL);
  if (result < 0) {
    spdlog::warn("Failed to send SIGKILL to hg process. Error: {}",
                 uv_strerror(result));
  } else {
    spdlog::debug("Sent SIGKILL to hg process");
  }
}

string HgBackend::sendHgCommand(const string &command) {
  uv_buf_t buffer[] = {{.len = static_cast<unsigned long>(command.size()),
                        .base = const_cast<char *>(command.data())}};

  auto req = new uv_write_t();
  uv_write(req, reinterpret_cast<uv_stream_t *>(&hgStdin), buffer, 1,
           [](uv_write_t *req, int status) {
             if (status < 0) {
               spdlog::error("Failed to write to hg cmdserver. Error: {}",
                             uv_strerror(status));
             }
             delete req;
           });
  return string();
}

void HgBackend::onProcessStdoutRead(uv_stream_t *stream, ssize_t nread,
                                    const uv_buf_t *buf) {
  if (nread > 0) {
    try {
      auto backend = reinterpret_cast<HgBackend *>(stream->data);
      backend->onProcessStdoutRead(buf->base, nread);
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

void HgBackend::onProcessStdoutRead(const char *data, ssize_t nread) {
  auto result = protocol.feedStdOutput(data, nread);
  // Process started and sent Hello message
  if (protocol.getHelloMessage().pid != 0 && !processStarted) {
    processStarted = true;
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
    // TODO: implement
    throw runtime_error(
        "Hg cmdserver requires input, but this is not implemented yet");
  }

  switch (currentCommand) {
  case CommandType::CheckLocalState:
    if (!checkCallback)
      break;
    {
      LocalStateResult result;
      if (commandResult.resultCode == 255) {
        // This means the repository is not initialized or the folder not found
        result.ok = true;
        result.initialized = false;
        result.hasUncommittedChanges = false;
      } else {
        result.ok = (commandResult.resultCode == 0);
        result.initialized = result.ok;
        result.errorMessage = "";
      }
      (*checkCallback)(repoFolder, result);
    }
    break;
  case CommandType::PreparePatch:
    if (prepareCallback) {
      (*prepareCallback)(repoFolder, PreparePatchResult{});
    }
    break;
  }
}

void HgBackend::onProcessStderrRead(uv_stream_t *stream, ssize_t nread,
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

void HgBackend::onProcessExit(uv_process_t *req, int64_t exit_status,
                              int term_signal) {
  auto backend = reinterpret_cast<HgBackend *>(req->data);
  backend->onProcessExit(exit_status, term_signal);
}

void HgBackend::onProcessExit(int64_t exit_status, int term_signal) {
  if (exit_status != 0) {
    spdlog::error("hg cmdserver exited with status {} and signal {}",
                  exit_status, term_signal);
  } else {
    spdlog::info("hg cmdserver exited successfully");
  }

  uv_read_stop(reinterpret_cast<uv_stream_t *>(&hgStdout));
  uv_read_stop(reinterpret_cast<uv_stream_t *>(&hgStderr));

  closePipeHandle(&hgStdin);
  closePipeHandle(&hgStdout);
  closePipeHandle(&hgStderr);

  uv_close(reinterpret_cast<uv_handle_t *>(process), [](uv_handle_t *handle) {
    delete reinterpret_cast<uv_process_t *>(handle);
  });
  process = nullptr;
}

void HgBackend::initPipe(uv_pipe_t *pipe, const char *name) {
  int result = uv_pipe_init(loop, pipe, 0);
  if (result < 0) {
    throw runtime_error("Failed to initialize child pipe " + string(name) +
                        ". Error: " + string(uv_strerror(result)));
  }
  uv_handle_set_data(reinterpret_cast<uv_handle_t *>(pipe), this);
}

void HgBackend::waitRunningLoop(uv_loop_t *loop,
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

} // namespace synqueen
