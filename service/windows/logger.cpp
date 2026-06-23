#include "logger.hpp"

#include <filesystem>
#include <iostream>

#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/win_eventlog_sink.h>

#include <platform.hpp>
#include <standardpaths.hpp>

namespace fs = std::filesystem;

namespace synqueen {

std::shared_ptr<spdlog::logger> createLogger() {
  auto logsPath = (fs::path(StandardPaths::getDataPath()) / "logs").string();

  std::cout << "Creating logger with logs path: " << logsPath << std::endl;

  try {
    std::vector<spdlog::sink_ptr> sinksList;
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logsPath + "/service_log.txt", 2 * 1024 * 1024, 3);
    fileSink->set_level(spdlog::level::trace);
#ifdef NDEBUG
    fileSink->set_pattern("%T.%e [%t] %L  %v");
#else
    fileSink->set_pattern("%T.%e [%t] %L <%s:%#>  %v");
#endif
    sinksList.push_back(fileSink);

#ifdef SQ_OS_WINDOWS
    // Printing the message to a debugger on Windows
    auto debugSink = std::make_shared<spdlog::sinks::msvc_sink_mt>(false);
    debugSink->set_level(spdlog::level::debug);
    debugSink->set_pattern("%T.%e  %v");
    sinksList.push_back(debugSink);
    auto eventSink =
        std::make_shared<spdlog::sinks::win_eventlog_sink_mt>("Synqueen");
    eventSink->set_level(spdlog::level::info);
    sinksList.push_back(eventSink);
#elif defined(SQ_OS_ANDROID)
    // Printing the message to the Android logcat on Android
    auto androidSink =
        std::make_shared<spdlog::sinks::android_sink_mt>("Synqueen");
    androidSink->set_level(spdlog::level::debug);
    androidSink->set_pattern("%v");
    sinksList.push_back(androidSink);
#else
    // And to standard output otherwise
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::debug);
    consoleSink->set_pattern("%T.%e  %v");
    sinksList.push_back(consoleSink);
#endif

    auto spdLogger = std::make_shared<spdlog::logger>(
        "MainLogger", sinksList.begin(), sinksList.end());
    spdLogger->set_level(spdlog::level::debug);
    // Store the latest 32 trace messages in a ring buffer
    spdlog::enable_backtrace(32);
    spdlog::flush_every(std::chrono::seconds(5));
    return spdLogger;
  } catch (const spdlog::spdlog_ex &ex) {
    std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    return nullptr;
  }
}

} // namespace synqueen
