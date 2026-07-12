#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace synqueen {

void initialize(std::shared_ptr<spdlog::logger> logger = nullptr);

void deinitialize();

// Run the main loop of the service. This function will block until the service
// is stopped.
void run();

void stop();

} // namespace synqueen
