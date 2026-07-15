#include "patchexchange.hpp"

#include <cctype>
#include <iostream>
#include <spdlog/fmt/bin_to_hex.h>
#include <stdexcept>
#include <uv.h>

namespace synqueen {

PatchExchange::PatchExchange(uv_loop_t *l) : loop(l) {}

void PatchExchange::ensureFolderInitialized(const std::string &folderPath) {}

void on_exit(uv_process_t *req, int64_t exit_status, int term_signal) {
  uv_close(reinterpret_cast<uv_handle_t *>(req), [](uv_handle_t *handle) {
    delete reinterpret_cast<uv_process_t *>(handle);
  });
}

void PatchExchange::parseHgOutput(const char *buffer, size_t length,
                                  HgOutput &hgOutput) {
  SPDLOG_INFO("Received from hg cmdserver: {}",
              spdlog::to_hex(buffer, buffer + length));

  if (length < 1) {
    throw std::runtime_error("Invalid hg output: empty buffer");
  }

  char channelChar = buffer[0];
  if (std::isupper(static_cast<unsigned char>(channelChar))) {
    // The channels can be required and their identifiers are uppercase letters
    switch (channelChar) {
    case 'I':
      // Input channel is for hg to request exact size bytes input from the user
      hgOutput.channel = HgChannel::Input;
      break;
    case 'L':
      // Line based input channel is for hg to request one single line input
      // from the user
      hgOutput.channel = HgChannel::Line;
      break;
    default:
      // If the channel is not recognized, we have to abort execution
      throw std::runtime_error(
          std::string("Invalid hg output: unknown required channel '") +
          channelChar + "'");
    }
    hgOutput.hasData = false;
    return;
  }

  // The channels can be optional and their identifiers are lowercase letters
  if (length < 5) {
    throw std::runtime_error("Invalid hg output: too short");
  }

  switch (channelChar) {
  case 'o':
    hgOutput.channel = HgChannel::Output;
    break;
  case 'e':
    hgOutput.channel = HgChannel::Error;
    break;
  case 'r':
    hgOutput.channel = HgChannel::Result;
    break;
  case 'd':
    hgOutput.channel = HgChannel::Debug;
    break;
  default:
    // Unknown optional channels can be ignored
    hgOutput.hasData = false;
    return;
  }
  auto dataLength = *reinterpret_cast<const uint32_t *>(buffer + 1);
  if (length < 5 + dataLength) {
    throw std::runtime_error("Invalid hg output: data length mismatch");
  }
  hgOutput.data.assign(buffer + 5, dataLength);
  hgOutput.hasData = true;
}

void PatchExchange::startHgServer() {
  if (loop == nullptr) {
    throw std::runtime_error("UV loop is not initialized");
  }

  if (hgProcess != nullptr)
    return; // Already started

  if (!pipesInitialized) {
    int pipeErr = uv_pipe_init(loop, &childStdin, 0);
    if (pipeErr < 0) {
      throw std::runtime_error(
          "Failed to initialize child stdin pipe. Error: " +
          std::string(uv_strerror(pipeErr)));
    }
    uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&childStdin), this);

    pipeErr = uv_pipe_init(loop, &childStdout, 0);
    if (pipeErr < 0) {
      throw std::runtime_error(
          "Failed to initialize child stdout pipe. Error: " +
          std::string(uv_strerror(pipeErr)));
    }
    uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&childStdout), this);

    pipeErr = uv_pipe_init(loop, &childStderr, 0);
    if (pipeErr < 0) {
      throw std::runtime_error(
          "Failed to initialize child stderr pipe. Error: " +
          std::string(uv_strerror(pipeErr)));
    }
    uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&childStderr), this);

    pipesInitialized = true;
  }

  uv_stdio_container_t stdio[3];
  stdio[0].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
  stdio[0].data.stream = reinterpret_cast<uv_stream_t *>(&childStdin);
  stdio[1].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[1].data.stream = reinterpret_cast<uv_stream_t *>(&childStdout);
  stdio[2].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[2].data.stream = reinterpret_cast<uv_stream_t *>(&childStderr);

  hgProcess = new uv_process_t();
  uv_process_options_t options = {0};
  options.exit_cb = on_exit;
  options.file = "C:\\Program Files\\Mercurial\\hg";
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

  auto err = uv_spawn(loop, hgProcess, &options);
  if (err < 0) {
    auto errMsg = std::string(uv_strerror(err));
    uv_close(reinterpret_cast<uv_handle_t *>(hgProcess),
             [](uv_handle_t *handle) {
               delete reinterpret_cast<uv_process_t *>(handle);
             });
    throw std::runtime_error("Failed to start hg cmdserver. Error: " + errMsg);
  }

  err = uv_read_start(
      reinterpret_cast<uv_stream_t *>(&childStdout),
      [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
      },
      [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread > 0) {
          try {
            HgOutput output;
            PatchExchange::parseHgOutput(buf->base, nread, output);
          } catch (const std::exception &e) {
            SPDLOG_ERROR("Error parsing hg output: {}", e.what());
          }
        } else if (nread < 0) {
          if (nread != UV_EOF) {
            // Handle read error
          }
          uv_close((uv_handle_t *)stream, NULL);
        }
        delete[] buf->base;
      });

  if (err < 0) {
    auto errMsg = std::string(uv_strerror(err));
    throw std::runtime_error(
        "Failed to start reading from hg cmdserver. Error: " + errMsg);
  }
}

} // namespace synqueen
