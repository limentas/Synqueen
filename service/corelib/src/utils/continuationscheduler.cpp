#include "utils/continuationscheduler.hpp"

namespace synqueen::utils {

ContinuationScheduler *ContinuationScheduler::self = nullptr;
bool ContinuationScheduler::destroyed = false;

void ContinuationScheduler::initialize(uv_loop_t &loop) {
  if (self != nullptr)
    return;

  assert(destroyed != true &&
         "ContinuationScheduler instance was already destroyed");

  static std::once_flag flag;
  std::call_once(flag, [&loop]() { self = new ContinuationScheduler(loop); });
}

void ContinuationScheduler::schedule(corral::Handle handle) {
  auto *scheduler = ContinuationScheduler::getInstance();
  scheduler->scheduleImpl(handle);
}

bool ContinuationScheduler::isInitialized() { return self != nullptr; }

ContinuationScheduler::ContinuationScheduler(uv_loop_t &loop)
    : taskScheduler(loop) {
  mainThread = uv_thread_self();
}

ContinuationScheduler::~ContinuationScheduler() {
  destroyed = true;
  self = nullptr;
}

ContinuationScheduler *ContinuationScheduler::getInstance() {
  assert(self != nullptr &&
         "ContinuationScheduler instance is not initialized");
  return self;
}

void ContinuationScheduler::scheduleImpl(corral::Handle handle) {
  auto currentThread = uv_thread_self();
  assert(uv_thread_equal(&mainThread, &currentThread) != 0 &&
         "Continuation must be scheduled from the main thread only");
  taskScheduler.runInEventLoop([handle]() { handle.resume(); });
}

} // namespace synqueen::utils
