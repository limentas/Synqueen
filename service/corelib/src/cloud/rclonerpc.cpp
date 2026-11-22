#include "rclonerpc.hpp"

#include "utils/exceptions.hpp"
#include "utils/platform.hpp"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <filesystem>

namespace fs = std::filesystem;
namespace json = rapidjson;
using namespace synqueen::utils;

namespace synqueen::cloud {

const char *RcloneRpc::jobsGroup("default");

RcloneRpc::RcloneRpc(uv_loop_t &loop)
    : loop(loop), commandFinishedAsync(nullptr, deleteHandle<uv_async_t>) {
  auto path = fs::path("librclone") / "shared" / RcloneRpc::libraryName();
  auto res = uv_dlopen(path.string().c_str(), &rcloneLib);
  if (res != 0) {
    SPDLOG_CRITICAL("Failed to load {}. Error: {}", path.string(),
                    uv_dlerror(&rcloneLib));
    throw synqueen::SqException("Failed to load " + path.string());
  }

  void *symbol;
  resolveFunction("RcloneInitialize", &symbol);
  rcloneInitialize = reinterpret_cast<void (*)()>(symbol);

  resolveFunction("RcloneFinalize", &symbol);
  rcloneFinalize = reinterpret_cast<void (*)()>(symbol);

  // Strictly speaking, the RcloneRPC function arguments declared as char *.
  // But it should be safe to cast to const char * to make our life easier.
  resolveFunction("RcloneRPC", &symbol);
  rcloneRPC =
      reinterpret_cast<RcloneRPCResult (*)(const char *, const char *)>(symbol);

  resolveFunction("RcloneFreeString", &symbol);
  rcloneFreeString = reinterpret_cast<void (*)(char *)>(symbol);

  commandFinishedAsync = AsyncPtr(new uv_async_t(), deleteHandle<uv_async_t>);
  commandFinishedAsync->data = this;
  uv_async_init(&loop, commandFinishedAsync.get(), [](uv_async_t *handle) {
    static_cast<RcloneRpc *>(handle->data)->onCommandFinished();
  });

  json::Document d;
  d.AddMember("path", path.string(), d.GetAllocator());
  runCommand("config/setpath", std::move(d));
}

RcloneRpc::~RcloneRpc() {
  // IMPORTANT: librclone docs explicitly state that we must not unload the
  // library. But let's try.
  //// uv_dlclose(&rcloneLib);
}

corral::Task<void> RcloneRpc::shutdown() {
  commandsThread.request_stop();
  co_await freeRclone();
  if (commandsThread.joinable())
    commandsThread.join();
  co_return;
}

corral::Task<RcloneRpc::CommandResult>
RcloneRpc::runCommand(const char *method,
                      json::GenericDocument<json::UTF8<>> &&input) {
  co_await tryLoadRclone();
  tryStartCommandsThread();
  auto completer = std::make_shared<Completer<CommandResult>>();
  auto modifiedInput =
      std::make_shared<json::GenericDocument<json::UTF8<>>>(std::move(input));
  modifiedInput->AddMember("_async", json::Value().SetBool(true),
                           modifiedInput->GetAllocator());
  {
    const std::lock_guard<std::mutex> lock(pendingCommandsMutex);
    pendingCommands.push_back(CommandStruct{.method = method,
                                            .input = modifiedInput,
                                            .completer = completer,
                                            .jobId = std::nullopt});
  }
  pendingCommandsCv.notify_all();
  co_return co_await Awaitable<CommandResult>(completer);
}

void RcloneRpc::resolveFunction(const char *name, void **symbol) {
  auto res = uv_dlsym(&rcloneLib, name, symbol);
  if (res != 0) {
    SPDLOG_CRITICAL("Failed to load symbol {}. Error: {}", name,
                    uv_dlerror(&rcloneLib));
    throw synqueen::SqException("Failed to load symbol " + std::string(name));
  }
}

corral::Task<void> RcloneRpc::tryLoadRclone() {
  if (rcloneLoaded)
    co_return;
  co_await loadRclone();
}

corral::Task<void> RcloneRpc::loadRclone() {
  SPDLOG_DEBUG("Loading rclone");
  try {
    this->rcloneInitialize();
    rcloneLoaded = true;
    co_return;
  } catch (...) {
    SPDLOG_CRITICAL("Failed to load rclone");
    throw;
  }
}

corral::Task<void> RcloneRpc::freeRclone() {
  if (!rcloneLoaded)
    co_return;

  SPDLOG_DEBUG("Freeing rclone");
  try {
    this->rcloneFinalize();
    rcloneLoaded = false;
  } catch (...) {
    SPDLOG_ERROR("Failed to free rclone");
    throw;
  }
  co_return;
}

void RcloneRpc::tryStartCommandsThread() {
  if (commandsThread.joinable())
    return;

  startCommandsThread();
}

void RcloneRpc::startCommandsThread() {
  commandsThread = std::jthread(
      [this](std::stop_token stoken) { this->runCommandsThread(stoken); });
}

void RcloneRpc::runCommandsThread(std::stop_token stoken) {
  while (!stoken.stop_requested()) {
    CommandStruct *command;
    CommandQueue::iterator commandIt = pendingCommands.end();
    {
      std::unique_lock<std::mutex> lock(pendingCommandsMutex);
      pendingCommandsCv.wait_for(lock, std::chrono::milliseconds(100),
                                 [this] { return !pendingCommands.empty(); });
      if (pendingCommands.empty())
        continue;
      if (commandIt == pendingCommands.end())
        commandIt = pendingCommands.begin();
      else
        ++commandIt;
      command = &(*commandIt);
    }

    if (stoken.stop_requested())
      break;

    if (!command->jobId.has_value()) {
      // The command was not yet started
      try {
        auto jobid = doCall(command->method, command->input);
        command->jobId = jobid;
      } catch (...) {
        SPDLOG_ERROR("Failed to execute command {}", command->method);
        std::unique_lock<std::mutex> lock(pendingCommandsMutex);
        commandIt = pendingCommands.erase(commandIt);
        if (commandIt != pendingCommands.begin() &&
            commandIt != pendingCommands.end())
          --commandIt;
        else
          commandIt = pendingCommands.end(); // To start over
        continue;
      }
    } else {
      // Command started => we need to check its status or wait for completion
      try {
        auto result = checkCommandStatus(command->jobId.value());
        if (result.has_value()) {
          command->result = *result;
          {
            std::unique_lock<std::mutex> lock(finishedCommandsMutex);
            finishedCommands.push_back(*command);
          }
          {
            std::unique_lock<std::mutex> lock(pendingCommandsMutex);
            commandIt = pendingCommands.erase(commandIt);
            if (commandIt != pendingCommands.begin() &&
                commandIt != pendingCommands.end())
              --commandIt;
            else
              commandIt = pendingCommands.end(); // To start over
          }
          uv_async_send(commandFinishedAsync.get());
        }
      } catch (...) {
        SPDLOG_ERROR("Failed to check command status for {}", command->method);
      }
    }
  }
}

long long RcloneRpc::doCall(const char *method, JsonDocumentDefaultPtr input) {
  json::StringBuffer buffer;
  json::Writer<json::StringBuffer> writer(buffer);
  input->Accept(writer);
  auto res = rcloneRPC(method, buffer.GetString());
  SPDLOG_DEBUG("rcloneRPC result for method {}: status={}", method, res.Status);
  if (!isStatusOk(res.Status)) {
    SPDLOG_ERROR("rcloneRPC call for method {} failed with status {}", method,
                 res.Status);
    throw SqException("rcloneRPC call for method " + std::string(method) +
                      " failed with status " + std::to_string(res.Status));
  }

  json::Document document;
  document.Parse(res.Output);

  if (document.HasParseError()) {
    throw SqException("Failed to parse rclone RPC output: " +
                      std::string(res.Output));
  }

  if (!document.HasMember("jobid")) {
    throw SqException("rclone RPC output missing 'jobid' field");
  }

  return document["jobid"].GetInt64();
}

std::optional<RcloneRpc::CommandResult>
RcloneRpc::checkCommandStatus(long long jobId) {
  json::StringBuffer buffer;
  json::Writer<json::StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key("jobid");
  writer.Int64(jobId);
  writer.EndObject();

  auto res = rcloneRPC("job/status", buffer.GetString());
  SPDLOG_TRACE("rcloneRPC result for check command: status={}", res.Status);
  if (!isStatusOk(res.Status)) {
    SPDLOG_ERROR("rcloneRPC check command failed with status {}", res.Status);
    throw SqException("rcloneRPC check command failed with status " +
                      std::to_string(res.Status));
  }

  json::Document document;
  document.Parse(res.Output);

  rcloneFreeString(res.Output);

  if (document.HasParseError()) {
    throw SqException("Failed to parse rclone RPC output: " +
                      std::string(res.Output));
  }

  if (!document.HasMember("finished")) {
    throw SqException("rclone RPC output missing 'finished' field");
  }

  auto finished = document["finished"].GetBool();
  if (finished) {
    if (!document.HasMember("output")) {
      throw SqException("rclone RPC output missing 'output' field");
    }
    CommandResult result;
    // We need to copy the output to not get it destroyed when the document goes
    // out of scope
    result.output = std::make_shared<
        json::GenericValue<json::UTF8<>, json::CrtAllocator>>();
    json::CrtAllocator crt;
    result.output->CopyFrom(document["output"], crt);
    result.error = document.HasMember("error")
                       ? std::string(document["error"].GetString(),
                                     document["error"].GetStringLength())
                       : "";

    if (document.HasMember("duration")) {
      auto duration = document["duration"].GetDouble();
      result.duration = std::chrono::duration<double>(duration);
    }

    return result;
  }

  return std::nullopt;
}

void RcloneRpc::onCommandFinished() {
  std::unique_lock<std::mutex> lock(finishedCommandsMutex);
  for (auto &command : finishedCommands) {
    if (command.completer) {
      command.completer->completeCommand(command.result);
    }
  }
  finishedCommands.clear();
}

std::string RcloneRpc::libraryName() {
#ifdef SQ_OS_WINDOWS
  return "librclone.dll";
#else
  return "librclone.so";
#endif
}

bool RcloneRpc::isStatusOk(long long status) {
  return status >= 200 && status < 300;
}

} // namespace synqueen::cloud
