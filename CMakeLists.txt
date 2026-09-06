cmake_minimum_required(VERSION 3.20)
project(TheAnswerProtocol LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Werror)

option(ENABLE_SANITIZERS "Activer ASan et UBSan" OFF)
if (ENABLE_SANITIZERS)
  add_compile_options(-ggdb3 -O0 -fsanitize=address,undefined)
  add_link_options(-fsanitize=address,undefined)
endif()

# Dépendances protobuf et yaml-cpp
find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(NOTCURSES REQUIRED IMPORTED_TARGET notcurses)

add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)

# Génération du code c++ à partir du template proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS template.proto)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROTOBUF_INCLUDE_DIRS}
  vendor/yaml-cpp/include
)

# exécutables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(server
  server/server.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)
target_link_libraries(server PRIVATE
  protobuf::libprotobuf
  absl::log_internal_check_op
  yaml-cpp
)

add_executable(gui
  gui/gui.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)
target_link_libraries(gui PRIVATE
  protobuf::libprotobuf
  absl::log_internal_check_op
)

add_executable(cli
  cli/cli.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)
target_link_libraries(cli PRIVATE
  protobuf::libprotobuf
  absl::log_internal_check_op
  PkgConfig::NOTCURSES
)
