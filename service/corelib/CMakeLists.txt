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
  src/synchronizer.hpp
  src/synchronizer.cpp
  src/precompiled.hpp
  src/folderstate.hpp
  src/folderstate.cpp
  src/cloudgate.hpp
  src/cloudgate.cpp
  src/patch/patchexchange.hpp
  src/patch/patchexchange.cpp
  src/patch/patchbackend.hpp
  src/patch/hgbackend.hpp
  src/patch/hgbackend.cpp
  src/patch/hgprotocol.hpp
  src/patch/hgprotocol.cpp
  src/watchers/basewatcher.hpp
  src/watchers/basewatcher.cpp
  src/watchers/timerwatcher.hpp
  src/watchers/timerwatcher.cpp
  include/utils/platform.hpp
  include/utils/standardpaths.hpp
  include/utils/utils.hpp
  include/utils/uvutils.hpp
  src/utils/standardpaths.cpp
  src/utils/uvutils.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/precompiled.hpp)
