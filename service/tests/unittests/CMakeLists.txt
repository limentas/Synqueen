cmake_minimum_required(VERSION 4.3)

project(UnitTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

# To put unittests binaries into subfolders of main build folder
unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)

# Google Test framework
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 4b21f1abddaf7d28388d7430bab69a81eccb1909)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add compiler flags for coverage
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL
    "Clang")
    # For multi-configuration builds we don't know build type on configuration
    # step, we know this only on build step. Here we add compiler flags only for
    # DEBUG builds. These commands add compiler and linker flags for all
    # subfolders.
    add_compile_options($<$<CONFIG:DEBUG>:--coverage>)
    add_link_options($<$<CONFIG:DEBUG>:--coverage>)
endif()

if(MSVC)
    # add_compile_options(/fsanitize=address)
    # add_link_options(/fsanitize=address)
endif()

include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

add_subdirectory(corelib)
