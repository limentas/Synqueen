#include "signalshandler.hpp"

#include <spdlog/spdlog.h>

#include <cassert>
#include <csignal>
#include <thread>

using namespace std;

namespace synqueen {

SignalsHandler::SignalsHandler(StopCallback callback)
    : stopCallback(callback), sigint(nullptr, deleteSignal),
      loop(nullptr, deleteLoop), sigbreak(nullptr, deleteSignal),
      sighup(nullptr, deleteSignal), stopEvent(nullptr, deleteAsync) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  if (result < 0) {
    spdlog::critical("Failed to initialize signal handler loop. Error: {}",
                     uv_strerror(result));
    delete l;
    throw std::runtime_error(
        "Failed to initialize signal handler loop. Error: " +
        std::string(uv_strerror(result)));
  }
  loop = LoopPtr(l, deleteLoop);

  sigint = initSignal(SIGINT, loop.get(), stopCallback);
  sigbreak = initSignal(SIGBREAK, loop.get(), stopCallback);
#ifdef SIGHUP
  sighup = initSignal(SIGHUP, loop.get(), stopCallback);
#endif

  // We have to stop and close all handles in the event loop
  auto se = new uv_async_t();
  se->data = this;
  result = uv_async_init(loop.get(), se, [](uv_async_t *handle) {
    auto handler = reinterpret_cast<SignalsHandler *>(handle->data);
    handler->stopAsync();
  });
  if (result < 0) {
    spdlog::error("Failed to initialize stop event for signal loop. Error: {}",
                  uv_strerror(result));
    delete se;
    throw std::runtime_error(
        "Failed to initialize stop event for signal loop. Error: " +
        std::string(uv_strerror(result)));
  }
  stopEvent = AsyncPtr(se, deleteAsync);

  eventLoopThread = make_unique<jthread>([this]() {
    uv_run(loop.get(), UV_RUN_DEFAULT);
    spdlog::debug("Signal handler loop exited");
  });
}

SignalsHandler::~SignalsHandler() {
  spdlog::debug("Stopping signal handlers");
  auto result = uv_async_send(stopEvent.get());
  if (result < 0) {
    spdlog::error("Failed to send stop event to signal handler loop. Error: {}",
                  uv_strerror(result));
  }

  // Wait for the event loop thread to finish
  if (eventLoopThread) {
    eventLoopThread->join();
    eventLoopThread = nullptr;
  }
  loop.reset();
}

void SignalsHandler::stopAsync() {
  // Stop all events and signals to exit the event loop
  sigint.reset();
  sigbreak.reset();
  sighup.reset();
  stopEvent.reset();
}

SignalPtr SignalsHandler::initSignal(int signum, uv_loop_t *loop,
                                     StopCallback callback) {
  auto signal = new uv_signal_t();
  int result = uv_signal_init(loop, signal);
  if (result < 0) {
    spdlog::warn("Failed to initialize signal handler for signum {}. Error: {}",
                 signum, uv_strerror(result));
    delete signal;
    return SignalPtr(nullptr, deleteSignal);
  }
  signal->data = callback;
  result = uv_signal_start(
      signal,
      [](uv_signal_t *handle, int signum) {
        auto callback = reinterpret_cast<StopCallback>(handle->data);
        assert(callback != nullptr);
        callback();
      },
      signum);
  if (result < 0) {
    spdlog::warn("Failed to start signal handler for signum {}. Error: {}",
                 signum, uv_strerror(result));
    uv_close(reinterpret_cast<uv_handle_t *>(signal), [](uv_handle_t *handle) {
      delete reinterpret_cast<uv_signal_t *>(handle);
    });
    return SignalPtr(nullptr, deleteSignal);
  }
  return SignalPtr(signal, deleteSignal);
}

} // namespace synqueen
