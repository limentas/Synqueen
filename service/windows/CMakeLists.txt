cmake_minimum_required(VERSION 4.3)

project(synqueen-service)

add_executable(
    synqueen-service
    main.cpp
    logger.hpp
    logger.cpp
    ${CMAKE_SOURCE_DIR}/common/src/standardpaths.hpp
    ${CMAKE_SOURCE_DIR}/common/src/standardpaths.cpp
)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog_header_only)
find_package(libuv 1.52.1 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
target_link_libraries(${PROJECT_NAME} PRIVATE synqueen-corelib)

include_directories(${CMAKE_SOURCE_DIR}/corelib/include)
include_directories(${CMAKE_SOURCE_DIR}/common/src)
