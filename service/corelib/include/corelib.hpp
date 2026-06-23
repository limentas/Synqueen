#pragma once

#include <memory>
#include <spdlog/spdlog.h>

#ifdef SYNQUEEN_EXPORTS
#define SYNQUEEN_API __declspec(dllexport)
#else
#define SYNQUEEN_API __declspec(dllimport)
#endif

namespace synqueen {

SYNQUEEN_API void initialize(std::shared_ptr<spdlog::logger> logger = nullptr);

// Run the main loop of the service. This function will block until the service
// is stopped.
SYNQUEEN_API void run();

SYNQUEEN_API void stop();

} // namespace synqueen
