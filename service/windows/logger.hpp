#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace synqueen {

std::shared_ptr<spdlog::logger> createLogger();

} // namespace synqueen