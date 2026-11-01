#pragma once

#include "corralheader.hpp"
#include "exceptions.hpp"

namespace synqueen {

template <typename ResultType> class Awaitable;

template <typename ResultType> class Completer {
public:
  inline bool isReady() const {
    return pendingResult.has_value() || pendingException.has_value() ||
           pendingUnknownException != nullptr;
  }
  inline std::optional<ResultType> result() const {
    if (pendingException) {
      pendingException->addRethrowContext("Exception rethrown");
      std::rethrow_exception(std::make_exception_ptr(*pendingException));
    }
    if (pendingUnknownException) {
      std::rethrow_exception(pendingUnknownException);
    }
    return pendingResult;
  }

  void completeCommand(ResultType result) {
    pendingResult = std::move(result);

    if (awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

  void completeWithException(const SqException &e) {
    pendingResult.reset();
    pendingUnknownException = nullptr;
    pendingException = e;

    if (awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

  void completeWithUnknownException(std::exception_ptr e) {
    pendingResult.reset();
    pendingException.reset();
    pendingUnknownException = e;

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
    if ((pendingResult.has_value() || pendingException.has_value() ||
         pendingUnknownException != nullptr) &&
        awaitableHandle) {
      auto handleToResume = awaitableHandle;
      awaitableHandle = corral::Handle();
      handleToResume.resume();
    }
  }

private:
  corral::Handle awaitableHandle;
  std::optional<ResultType> pendingResult;
  mutable std::optional<SqException> pendingException;
  std::exception_ptr pendingUnknownException = nullptr;
};

template <> class Completer<void> {
public:
  inline bool isReady() const { return completed; }

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

template <typename T> using CompleterPtr = std::shared_ptr<Completer<T>>;

template <typename ResultType> class Awaitable {
public:
  Awaitable(CompleterPtr<ResultType> c) : completer(c) {}

  bool await_ready() const noexcept { return completer->isReady(); }
  void await_suspend(corral::Handle h) { completer->setAwaitableHandle(h); }
  ResultType await_resume() { return completer->result().value(); }

private:
  CompleterPtr<ResultType> completer;
};

template <> class Awaitable<void> {
public:
  Awaitable(CompleterPtr<void> c) : completer(c) {}

  bool await_ready() const noexcept { return completer->isReady(); }
  void await_suspend(corral::Handle h) { completer->setAwaitableHandle(h); }
  void await_resume() {}

private:
  CompleterPtr<void> completer;
};

} // namespace synqueen
