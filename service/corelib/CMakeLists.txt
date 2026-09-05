cmake_minimum_required(VERSION 4.3)

project(synqueen-corelib)

find_package(gRPC CONFIG REQUIRED)

set(PROTO_FILES
  ../proto/service.proto
)

add_library(
  ${PROJECT_NAME}
  STATIC
  ${PROTO_FILES}
  include/corelib.hpp
  src/corelib.cpp
  src/settings.hpp
  src/settings.cpp
  src/settingsjsonschema.cpp
  src/synchronizer.hpp
  src/synchronizer.cpp
  src/precompiled.hpp
  src/foldermanager.hpp
  src/foldermanager.cpp
  src/cloudgate.hpp
  src/cloudgate.cpp
  src/const.hpp
  src/patch/patchbackend.hpp
  src/patch/patchbackend.cpp
  src/patch/hgbackend.hpp
  src/patch/hgbackend.cpp
  src/patch/hgprotocol.hpp
  src/patch/hgprotocol.cpp
  src/patch/hgprocess.hpp
  src/patch/hgprocess.cpp
  src/watchers/basewatcher.hpp
  src/watchers/basewatcher.cpp
  src/watchers/timerwatcher.hpp
  src/watchers/timerwatcher.cpp
  include/utils/platform.hpp
  include/utils/compilerwarnings.hpp
  include/utils/standardpaths.hpp
  include/utils/utils.hpp
  include/utils/uvutils.hpp
  include/utils/corralheader.hpp
  include/utils/corralutils.hpp
  include/utils/corraleventlooptraits.hpp
  src/utils/standardpaths.cpp
  src/utils/uvutils.cpp
  src/utils/utils.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/precompiled.hpp)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  protobuf::libprotobuf gRPC::grpc++
)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_include_directories(${PROJECT_NAME} PUBLIC "${PROTO_BINARY_DIR}")

protobuf_generate(
  TARGET ${PROJECT_NAME}
  IMPORT_DIRS ../proto
  PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")
