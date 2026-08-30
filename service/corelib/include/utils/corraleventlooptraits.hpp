#pragma once

#include <uv.h>

#include <spdlog/spdlog.h>

// Corral library EventLoopTraits for libuv event loop
namespace corral {

template <> struct EventLoopTraits<uv_loop_t> {
  static EventLoopID eventLoopID(uv_loop_t &loop) { return EventLoopID(&loop); }
  static void run(uv_loop_t &loop) {
    SPDLOG_TRACE("Starting libuv event loop");
    uv_run(&loop, UV_RUN_DEFAULT);
    SPDLOG_TRACE("libuv event loop exited");
  }
  static void stop(uv_loop_t &loop) {
    // The loop should close itself when there are no more active handles
    // uv_stop(&loop);
  }
};

} // namespace corral
