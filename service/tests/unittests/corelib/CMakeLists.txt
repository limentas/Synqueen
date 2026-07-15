cmake_minimum_required(VERSION 4.3)

project(CoreLibTest LANGUAGES CXX)

add_executable(
    ${PROJECT_NAME}
    patchexchange_test.cpp
    timerwatcher_test.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gmock_main synqueen-corelib)

# Discover GoogleTest tests and make them known to ctest
include(GoogleTest)
gtest_discover_tests(
    ${PROJECT_NAME} DISCOVERY_MODE PRE_TEST
    PROPERTIES TIMEOUT 10)