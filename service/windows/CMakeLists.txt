cmake_minimum_required(VERSION 4.3)

project(synqueen-service)

add_executable(
    synqueen-service
    main.cpp
    logger.hpp
    logger.cpp
    signalshandler.hpp
    signalshandler.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE synqueen-corelib)

include_directories(${CMAKE_SOURCE_DIR}/corelib/include)
include_directories(${CMAKE_SOURCE_DIR}/common/src)
