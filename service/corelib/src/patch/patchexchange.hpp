#pragma once

#include <list>
#include <queue>
#include <spdlog/spdlog.h>
#include <string>
#include <uv.h>

#include "commandresults.hpp"

namespace synqueen {

class PatchBackend;

// This is a wrapper around the PatchBackend that queues async requests and do
// calls one by one
class PatchExchange {
public:
  explicit PatchExchange(std::unique_ptr<PatchBackend> backendPtr);
  ~PatchExchange() = default;

  void checkLocalState(const std::string &folderPath,
                       const patch::LocalStateCallbackPtr &callback);
  void preparePatch(const std::string &folderPath,
                    const patch::PreparePatchCallbackPtr &callback);

  void ensureFolderInitialized(const std::string &folderPath);

private:
  struct Request {
    enum class Type { CheckLocalState, PreparePatch };
    Type type;
    std::string folderPath;

    patch::LocalStateCallbackPtr localStateCallback;
    patch::PreparePatchCallbackPtr preparePatchCallback;
  };
  typedef std::shared_ptr<Request> RequestPtr;

  void queueRequest(const RequestPtr &request);
  void tryProcessNextRequest();

private:
  std::unique_ptr<PatchBackend> backend;
  std::queue<RequestPtr> requestQueue;
  RequestPtr currentRequest;
};

} // namespace synqueen
