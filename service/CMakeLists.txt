cmake_minimum_required(VERSION 4.3)

message(STATUS "CMake v${CMAKE_VERSION}")

include(FetchContent)
FetchContent_Declare(
  vcpkg
  GIT_REPOSITORY https://github.com/microsoft/vcpkg
  GIT_TAG 2026.06.01
  GIT_SHALLOW OFF # vcpkg needs full history
)

set(VCPKG_BOOTSTRAP_OPTIONS "-disableMetrics")
set(VCPKG_INSTALL_OPTIONS "--no-print-usage")

if(NOT("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
  set(VCPKG_BUILD_TYPE "release") # Install only release for release builds
endif()

FetchContent_Declare(
  corral
  GIT_REPOSITORY https://github.com/hudson-trading/corral.git
  GIT_TAG 5d1153705395640c9f9f8c91e242c9932cd0bd0e
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(vcpkg)

# ~~~
# For Windows we don't use a toolchain file and set everything via command line arguments
# For Android we use toolchain file and also chain vcpkg toolchain afterwards
# Changing toolchain file only if it is not changed yet
# NOTE: This must be defined before the first project call
set(GENERATED_TOOLCHAIN "${CMAKE_BINARY_DIR}/synqueen-toolchain.cmake")

if(NOT EXISTS ${GENERATED_TOOLCHAIN})
  message(STATUS "Creating custom toolchain file ${GENERATED_TOOLCHAIN}")

  if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(
      STATUS
      "CMAKE_TOOLCHAIN_FILE is defined and equals to \"${CMAKE_TOOLCHAIN_FILE}\""
    )
  endif()

  if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT("${CMAKE_TOOLCHAIN_FILE}" STREQUAL
    "${GENERATED_TOOLCHAIN}"))
    file(WRITE ${GENERATED_TOOLCHAIN} "include(\"${CMAKE_TOOLCHAIN_FILE}\")\n")
  endif()

  file(APPEND ${GENERATED_TOOLCHAIN}
    "include(\"${CMAKE_SOURCE_DIR}/cmake/DetectPlatform.cmake\")\n")
  file(APPEND ${GENERATED_TOOLCHAIN}
    "include(\"${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake\")\n")
endif()

set(CMAKE_TOOLCHAIN_FILE ${GENERATED_TOOLCHAIN})

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

project(
  Synqueen
  LANGUAGES CXX
  VERSION 0.0.1)

message(
  STATUS "Cmaking project ${PROJECT_NAME} of version ${PROJECT_VERSION}")
message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/common/src)

# Common libraries used by all subprojects
find_package(libuv 1.52.1 CONFIG REQUIRED)
link_libraries($<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
find_package(RapidJSON CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
link_libraries(spdlog::spdlog_header_only)
FetchContent_MakeAvailable(corral)

include_directories(${corral_SOURCE_DIR})

add_subdirectory(corelib)
add_subdirectory(windows)

include(CTest) # sets BUILD_TESTING by default to ON

if(BUILD_TESTING)
  add_subdirectory(tests/unittests)
endif()
