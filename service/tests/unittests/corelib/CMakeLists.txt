cmake_minimum_required(VERSION 4.3)

project(CoreLibTest LANGUAGES CXX)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    timerwatcher_test.cpp
    hgprotocol_test.cpp
    hgbackend_test.cpp
    hgprocess_test.cpp
    uvutils_test.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${corral_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gmock_main synqueen-corelib)

# Discover GoogleTest tests and make them known to ctest
include(GoogleTest)
gtest_discover_tests(
    ${PROJECT_NAME} DISCOVERY_MODE PRE_TEST
    PROPERTIES TIMEOUT 10)
