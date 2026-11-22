#pragma once

#include "corralheader.hpp"
#include "exceptions.hpp"

namespace synqueen::utils {

template <typename ResultType> class Awaitable;

// This class contains common functionality for completers
class CompleterBase {
public:
  inline bool isReady() const { return completed; }
  void completeWithException(const SqException &e);
  void completeWithUnknownException(std::exception_ptr e);

protected:
  // Use templated version instead
  CompleterBase() = default;

  void completeCommand();
  void markCompleted();
  void resumeHandle();
  void rethrowIfFailed() const;

private:
  // For use by Awaitable
  template <typename ResultType> friend class Awaitable;
  void setAwaitableHandle(corral::Handle);

private:
  corral::Handle awaitableHandle;
  bool completed = false;
  mutable std::optional<SqException> pendingException;
  std::exception_ptr pendingUnknownException = nullptr;
};

template <typename ResultType> class Completer : public CompleterBase {
public:
  inline std::optional<ResultType> result() const {
    rethrowIfFailed();
    return pendingResult;
  }

  void completeCommand(ResultType result) {
    pendingResult = std::move(result);
    CompleterBase::completeCommand();
  }

private:
  std::optional<ResultType> pendingResult;
};

template <> class Completer<void> : public CompleterBase {
public:
  // We just make completeCommand public
  void completeCommand() { CompleterBase::completeCommand(); }
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

} // namespace synqueen::utils
