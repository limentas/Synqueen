#pragma once

#include <spdlog/spdlog.h>
#include <string>
#include <uv.h>

namespace synqueen {

class PatchExchange {
public:
  explicit PatchExchange(uv_loop_t *l);
  ~PatchExchange() = default;

  void ensureFolderInitialized(const std::string &folderPath);

  enum class HgChannel { Output, Error, Result, Debug, Input, Line };
  struct HgOutput {
    bool hasData;
    HgChannel channel;
    std::string data;
  };
  static void parseHgOutput(const char *buffer, size_t length,
                            HgOutput &hgOutput);

private:
  void startHgServer();

private:
  uv_loop_t *loop = nullptr;
  uv_process_t *hgProcess = nullptr;
  uv_pipe_t childStdin{};
  uv_pipe_t childStdout{};
  uv_pipe_t childStderr{};
  bool pipesInitialized = false;
};

} // namespace synqueen
