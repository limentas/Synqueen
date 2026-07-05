#include "patchexchange.hpp"

#include <stdexcept>

synqueen::PatchExchange::PatchExchange(uv_loop_t *l) : loop(l) {
  startHgServer();
}

void synqueen::PatchExchange::ensureFolderInitialized(
    const std::string &folderPath) {}

void on_exit(uv_process_t *req, int64_t exit_status, int term_signal) {
  uv_close((uv_handle_t *)req, NULL);
}

void synqueen::PatchExchange::startHgServer() {
  if (loop == nullptr) {
    throw std::runtime_error("UV loop is not initialized");
  }

  if (hgProcess != nullptr)
    return; // Already started

  uv_pipe_t childStdin;
  uv_pipe_t childStdout;
  uv_pipe_init(loop, &childStdin, 0);
  uv_pipe_init(loop, &childStdout, 0);

  uv_stdio_container_t stdio[3];
  stdio[0].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
  stdio[0].data.stream = (uv_stream_t *)&childStdin;
  stdio[1].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[1].data.stream = (uv_stream_t *)&childStdout;
  stdio[2].flags = UV_INHERIT_FD;
  stdio[2].data.fd = 2; // inherit stderr

  hgProcess = new uv_process_t();
  uv_process_options_t options = {0};
  options.exit_cb = on_exit;
  options.file = "C:\\Program Files\\Mercurial\\hg";
  const char *args[] = {"--config", "ui.interactive=True",
                        "serve",    "--cmdserver",
                        "pipe",     nullptr};
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
    throw std::runtime_error("Failed to start hg cmdserver. Error: " + errMsg);
  }
}
