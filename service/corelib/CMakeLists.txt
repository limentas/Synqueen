cmake_minimum_required(VERSION 4.3)

project(synqueen-corelib)

add_library(
  synqueen-corelib
  SHARED
  include/corelib.hpp
  src/corelib.cpp
  src/settings.hpp
  ${CMAKE_SOURCE_DIR}/common/src/standardpaths.hpp
  ${CMAKE_SOURCE_DIR}/common/src/standardpaths.cpp
  src/synchronizer.hpp
  src/synchronizer.cpp
  src/pch.h
)

find_package(libuv 1.52.1 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
find_package(RapidJSON CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog_header_only)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(${PROJECT_NAME} PRIVATE SYNQUEEN_EXPORTS)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
