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

if(NOT("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
  set(VCPKG_BUILD_TYPE "release") # Install only release for release builds
endif()

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

project(
  Synqueen
  LANGUAGES CXX
  VERSION 0.0.1)

message(
  STATUS "Cmaking project ${PROJECT_NAME} of version ${PROJECT_VERSION}")
message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(corelib)
add_subdirectory(windows)
