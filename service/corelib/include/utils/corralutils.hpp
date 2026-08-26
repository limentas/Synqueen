#pragma once

#include "corralheader.hpp"

namespace synqueen {

template <typename ResultType> class Awaitable;

template <typename ResultType> class Completer {
public:
  inline bool isReady() const { return pendingResult.has_value(); }
  inline std::optional<ResultType> result() const { return pendingResult; }

  void reset() {
    pendingResult.reset();
    awaitableHandle = corral::Handle();
  }

  void completeCommand(ResultType result) {
    pendingResult = std::move(result);

    if (awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  friend class Awaitable<ResultType>;

  void setAwaitableHandle(corral::Handle handle) {
    awaitableHandle = handle;
    if (pendingResult.has_value() && awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  corral::Handle awaitableHandle;
  std::optional<ResultType> pendingResult;
};

template <> class Completer<void> {
public:
  inline bool isReady() const { return completed; }

  void reset() {
    completed = false;
    awaitableHandle = corral::Handle();
  }

  void completeCommand() {
    completed = true;

    if (awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  friend class Awaitable<void>;

  void setAwaitableHandle(corral::Handle handle) {
    awaitableHandle = handle;
    if (completed && awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  corral::Handle awaitableHandle;
  bool completed = false;
};

template <typename ResultType> class Awaitable {
public:
  Awaitable(Completer<ResultType> &c) : completer(c) {}

  bool await_ready() const noexcept { return completer.isReady(); }
  void await_suspend(corral::Handle h) { completer.setAwaitableHandle(h); }
  ResultType await_resume() { return completer.result().value(); }

private:
  Completer<ResultType> &completer;
};

template <> class Awaitable<void> {
public:
  Awaitable(Completer<void> &c) : completer(c) {}

  bool await_ready() const noexcept { return completer.isReady(); }
  void await_suspend(corral::Handle h) { completer.setAwaitableHandle(h); }
  void await_resume() {}

private:
  Completer<void> &completer;
};

} // namespace synqueen
