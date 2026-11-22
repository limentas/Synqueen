#include "utils/corralutils.hpp"

#include "utils/continuationscheduler.hpp"

namespace synqueen::utils {

void CompleterBase::completeWithException(const SqException &e) {
  markCompleted();
  pendingException = e;
  resumeHandle();
}

void CompleterBase::completeWithUnknownException(std::exception_ptr e) {
  markCompleted();
  pendingUnknownException = e;
  resumeHandle();
}

void CompleterBase::completeCommand() {
  markCompleted();
  resumeHandle();
}

void CompleterBase::markCompleted() {
  assert(!completed);
  completed = true;
}

void CompleterBase::resumeHandle() {
  assert(awaitableHandle);

  // handle.resume immediately executes a code block after co_await, so it may
  // happen that this code will destroy the object that calls the resume and
  // this maybe undesired in certain scenarios. This is common for unit tests.
  // In real app destruction usually happens by an external event.
  if (ContinuationScheduler::isInitialized()) {
    // Schedule the continuation to event loop
    ContinuationScheduler::schedule(awaitableHandle);
    return;
  }

  // Call resume immediately
  auto handleToResume = awaitableHandle;
  awaitableHandle = corral::Handle();
  handleToResume.resume();
}

void CompleterBase::rethrowIfFailed() const {
  if (pendingException) {
    pendingException->addRethrowContext("Exception rethrown");
    std::rethrow_exception(std::make_exception_ptr(*pendingException));
  }
  if (pendingUnknownException) {
    std::rethrow_exception(pendingUnknownException);
  }
}

void CompleterBase::setAwaitableHandle(corral::Handle handle) {
  awaitableHandle = handle;

  // Already completed
  if (completed || pendingException.has_value() ||
      pendingUnknownException != nullptr) {
    resumeHandle();
  }
}

} // namespace synqueen::utils
