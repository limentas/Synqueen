#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace synqueen {

void initialize(std::shared_ptr<spdlog::logger> logger = nullptr);

void deinitialize();

// Run the main loop of the service. This function will block until the service
// is stopped.
void run();

// Stop the service. This function waits for the service to stop and then
// returns. It can be called from any thread but it is not thread-safe.
void stop();

} // namespace synqueen
