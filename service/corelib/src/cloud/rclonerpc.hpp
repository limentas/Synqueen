#pragma once

#include "cloud/cloudtypes.hpp"
#include "utils/corralheader.hpp"
#include "utils/corralutils.hpp"
#include "utils/jsontypes.hpp"
#include "utils/uvheader.hpp"
#include "utils/uvutils.hpp"

#include <librclone/librclone.h>
#include <rapidjson/document.h>

#include <list>
#include <mutex>
#include <optional>

namespace json = rapidjson;

namespace synqueen::cloud {

class RcloneRpc {
public:
  struct CommandResult {
    synqueen::utils::JsonValueCrtPtr output;
    std::string error;
    std::chrono::duration<double> duration;
  };

  RcloneRpc(uv_loop_t &loop);
  ~RcloneRpc();

  corral::Task<void> shutdown();

  corral::Task<CommandResult>
  runCommand(const char *method, json::GenericDocument<json::UTF8<>> &&input);

private:
  void resolveFunction(const char *name, void **symbol);

  corral::Task<void> tryLoadRclone();
  corral::Task<void> loadRclone();
  corral::Task<void> freeRclone();
  void tryStartCommandsThread();
  void startCommandsThread();

  void runCommandsThread(std::stop_token stoken);
  long long doCall(const char *method,
                   synqueen::utils::JsonDocumentDefaultPtr input);
  std::optional<CommandResult> checkCommandStatus(long long jobId);
  void onCommandFinished();

  static std::string libraryName();
  static bool isStatusOk(long long status);

private:
  static const char *jobsGroup;

  struct CommandStruct {
    const char *method;
    synqueen::utils::JsonDocumentDefaultPtr input;
    CommandResult result;
    synqueen::utils::CompleterPtr<CommandResult> completer;
    std::optional<long long> jobId;
  };
  typedef std::list<CommandStruct> CommandQueue;

  uv_loop_t &loop;
  bool rcloneLoaded = false;
  uv_lib_t rcloneLib;

  std::function<void()> rcloneInitialize;
  std::function<void()> rcloneFinalize;
  std::function<RcloneRPCResult(const char *method, const char *input)>
      rcloneRPC;
  std::function<void(char *str)> rcloneFreeString;

  CommandQueue pendingCommands;
  std::mutex pendingCommandsMutex;
  std::condition_variable pendingCommandsCv;

  CommandQueue finishedCommands;
  std::mutex finishedCommandsMutex;
  AsyncPtr commandFinishedAsync;

  std::jthread commandsThread;
};

} // namespace synqueen::cloud
