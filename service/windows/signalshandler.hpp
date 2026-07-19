#pragma once

#include "utils/uvutils.hpp"

#include <memory>
#include <thread>
#include <uv.h>

namespace synqueen {

class SignalsHandler {
public:
  typedef void (*StopCallback)();

  SignalsHandler(StopCallback callback);
  ~SignalsHandler();

  SignalsHandler(const SignalsHandler &) = delete;
  SignalsHandler &operator=(const SignalsHandler &) = delete;

private:
  void stopAsync();

  static SignalPtr initSignal(int signum, uv_loop_t *loop,
                              StopCallback callback);

private:
  StopCallback stopCallback = nullptr;
  LoopPtr loop;
  SignalPtr sigint, sigbreak, sighup;
  std::unique_ptr<std::jthread> eventLoopThread;
  AsyncPtr stopEvent;
};

} // namespace synqueen
