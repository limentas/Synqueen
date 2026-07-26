#include "patchexchange.hpp"

#include "patchbackend.hpp"

#include <cctype>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <uv.h>

namespace synqueen {

using namespace patch;

PatchExchange::PatchExchange(std::unique_ptr<PatchBackend> backendPtr)
    : backend(std::move(backendPtr)) {}

void PatchExchange::checkLocalState(const std::string &folderPath,
                                    const LocalStateCallbackPtr &callback) {
  assert(callback != nullptr);
  auto request = std::make_shared<PatchExchange::Request>();
  request->type = PatchExchange::Request::Type::CheckLocalState;
  request->folderPath = folderPath;
  request->localStateCallback = callback;
  queueRequest(request);
}

void PatchExchange::preparePatch(
    const std::string &folderPath,
    const patch::PreparePatchCallbackPtr &callback) {
  assert(callback != nullptr);
  auto request = std::make_shared<PatchExchange::Request>();
  request->type = PatchExchange::Request::Type::PreparePatch;
  request->folderPath = folderPath;
  request->preparePatchCallback = callback;
  queueRequest(request);
}

void PatchExchange::ensureFolderInitialized(const std::string &folderPath) {}

void PatchExchange::queueRequest(const RequestPtr &request) {
  requestQueue.push(request);
  tryProcessNextRequest();
}

void PatchExchange::tryProcessNextRequest() {
  if (currentRequest) {
    return;
  }
  if (requestQueue.empty()) {
    return;
  }

  currentRequest = requestQueue.front();
  requestQueue.pop();

  switch (currentRequest->type) {
  case Request::Type::CheckLocalState:
    backend->checkLocalState(
        currentRequest->folderPath,
        LocalStateCallbackPtr(
            new LocalStateCallback([this](const std::string &folderPath,
                                          const LocalStateResult &result) {
              auto callback = this->currentRequest->localStateCallback;
              currentRequest = nullptr;
              tryProcessNextRequest();
              try {
                (*callback)(folderPath, result);
              } catch (const std::exception &e) {
                spdlog::error("Exception in checkLocalState callback: {}",
                              e.what());
              } catch (...) {
                spdlog::error("Unknown exception in checkLocalState "
                              "callback");
              }
            })));
    break;
  case Request::Type::PreparePatch:
    backend->preparePatch(
        currentRequest->folderPath,
        PreparePatchCallbackPtr(
            new PreparePatchCallback([this](const std::string &folderPath,
                                            const PreparePatchResult &result) {
              auto callback = this->currentRequest->preparePatchCallback;
              currentRequest = nullptr;
              tryProcessNextRequest();
              try {
                (*callback)(folderPath, result);
              } catch (const std::exception &e) {
                spdlog::error("Exception in preparePatch callback: {}",
                              e.what());
              } catch (...) {
                spdlog::error("Unknown exception in preparePatch "
                              "callback");
              }
            })));
    break;
  }
}

} // namespace synqueen
