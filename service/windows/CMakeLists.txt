cmake_minimum_required(VERSION 4.3)

project(synqueen-service)

add_executable(
    synqueen-service
    WIN32
    main.cpp
)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog_header_only)

include_directories(${CMAKE_SOURCE_DIR}/include)
