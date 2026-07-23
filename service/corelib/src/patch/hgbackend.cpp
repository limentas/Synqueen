#include "hgbackend.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>

#include "hgprotocol.hpp"

namespace synqueen {

synqueen::HgBackend::HgBackend(uv_loop_t *l) : loop(l) {}

void HgBackend::checkLocalState(const std::string &folderPath,
                                const StateCallback &callback) {
  startHgServer();
  sendHgCommand("summary\n00000000");
}

void on_exit(uv_process_t *req, int64_t exit_status, int term_signal) {
  uv_close(reinterpret_cast<uv_handle_t *>(req), [](uv_handle_t *handle) {
    delete reinterpret_cast<uv_process_t *>(handle);
  });
}

void HgBackend::startHgServer() {
  if (hgProcess != nullptr)
    return; // Already started

  if (!pipesInitialized) {
    auto result = uv_pipe_init(loop, &hgStdin, 0);
    if (result < 0) {
      throw std::runtime_error(
          "Failed to initialize child stdin pipe. Error: " +
          std::string(uv_strerror(result)));
    }
    uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&hgStdin), this);

    result = uv_pipe_init(loop, &hgStdout, 0);
    if (result < 0) {
      throw std::runtime_error(
          "Failed to initialize child stdout pipe. Error: " +
          std::string(uv_strerror(result)));
    }
    uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&hgStdout), this);

    result = uv_pipe_init(loop, &hgStderr, 0);
    if (result < 0) {
      throw std::runtime_error(
          "Failed to initialize child stderr pipe. Error: " +
          std::string(uv_strerror(result)));
    }
    uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&hgStderr), this);

    pipesInitialized = true;
  }

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

  hgProcess = new uv_process_t();
  uv_process_options_t options = {0};
  options.exit_cb = on_exit;
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
      reinterpret_cast<uv_stream_t *>(&hgStdout),
      [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
      },
      [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread > 0) {
          try {
            HgProtocol::HgOutput output;
            HgProtocol::parseHgOutput(buf->base, nread, output);
          } catch (const std::exception &e) {
            spdlog::error("Error parsing hg output: {}", e.what());
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

std::string HgBackend::sendHgCommand(const std::string &command) {
  uv_buf_t buffer[] = {{.len = static_cast<unsigned long>(command.size()),
                        .base = const_cast<char *>(command.data())}};

  auto req = new uv_write_t();
  uv_write(req, reinterpret_cast<uv_stream_t *>(&hgStdin), buffer, 1,
           [](uv_write_t *req, int status) {
             if (status < 0) {
               spdlog::error("Failed to write to hg cmdserver. Error: {}",
                             uv_strerror(status));
             }
             uv_close(reinterpret_cast<uv_handle_t *>(req),
                      [](uv_handle_t *handle) {
                        delete reinterpret_cast<uv_write_t *>(handle);
                      });
           });
  return std::string();
}

} // namespace synqueen
