cmake_minimum_required(VERSION 3.14)
project(MarketDataStreamer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Proto files. 
set(PROTO_DIR ${CMAKE_SOURCE_DIR}/proto)
set(PROTO_SRC
    ${PROTO_DIR}/market_data.pb.cc
    ${PROTO_DIR}/market_data.grpc.pb.cc
)

# Find dependencies. 
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

# Include directories. 
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROTO_DIR}
  ${GRPC_INCLUDE_DIRS}
  ${PROTOBUF_INCLUDE_DIRS}
  /usr/local/include
  /opt/homebrew/include
)

# Link directories. 
link_directories(
  /usr/local/lib
  /opt/homebrew/lib
)

# -----------------------------
# Server Target. 
add_executable(server
  server/server.cpp
  ${PROTO_SRC}
)

target_link_libraries(server
  ${GRPC_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
)
