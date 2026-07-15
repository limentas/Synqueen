cmake_minimum_required(VERSION 4.3)

project(synqueen-corelib)

add_library(
  ${PROJECT_NAME}
  STATIC
  include/corelib.hpp
  src/corelib.cpp
  src/settings.hpp
  src/settings.cpp
  src/settingsjsonschema.cpp
  ${CMAKE_SOURCE_DIR}/common/src/standardpaths.hpp
  ${CMAKE_SOURCE_DIR}/common/src/standardpaths.cpp
  ${CMAKE_SOURCE_DIR}/common/src/utils.hpp
  src/synchronizer.hpp
  src/synchronizer.cpp
  src/precompiled.hpp
  src/folderstate.hpp
  src/folderstate.cpp
  src/cloudgate.hpp
  src/cloudgate.cpp
  src/patchexchange.hpp
  src/patchexchange.cpp
  src/watchers/basewatcher.hpp
  src/watchers/basewatcher.cpp
  src/watchers/timerwatcher.hpp
  src/watchers/timerwatcher.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/precompiled.hpp)
