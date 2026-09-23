cmake_minimum_required(VERSION 3.20)
project(TheAnswerProtocol LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Dépendances externes (compilées avec leurs propres règles, sans nos flags stricts)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v27.3
)
FetchContent_MakeAvailable(protobuf)

# FetchContent_Declare(
#   absl
#   GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
#   GIT_TAG 20240116.2
# )
# FetchContent_MakeAvailable(absl)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.9.0
)
FetchContent_MakeAvailable(yaml-cpp)

# 2. Génération du code C++ à partir de template.proto

set(PROTO_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/template.proto"
)

set(PROTO_GEN_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/common"
)

set(PROTO_SRCS
    "${PROTO_GEN_DIR}/template.pb.cc"
)

set(PROTO_HDRS
    "${PROTO_GEN_DIR}/template.pb.h"
)

# protoc ne crée pas forcément le répertoire de sortie racine,
# donc on le crée explicitement.
file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")

add_custom_command(
    OUTPUT
        ${PROTO_SRCS}
        ${PROTO_HDRS}

    COMMAND
        $<TARGET_FILE:protobuf::protoc>
        --cpp_out=${PROTO_GEN_DIR}
        -I${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTO_FILE}

    DEPENDS
        ${PROTO_FILE}
        protobuf::protoc

    COMMENT "Génération des sources C++ pour template.proto"

    VERBATIM
)

add_library(protocol_cpp STATIC
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_link_libraries(protocol_cpp
    PUBLIC
        protobuf::libprotobuf
)

target_include_directories(protocol_cpp
    PUBLIC
        ${PROTO_GEN_DIR}
)

# 3. Exécutables de votre projet
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(server server/server.cpp)
target_link_libraries(server PRIVATE
  protocol_cpp
  absl::log_internal_check_op
  yaml-cpp
)

add_executable(gui gui/gui.cpp)
target_link_libraries(gui PRIVATE
  protocol_cpp
  absl::log_internal_check_op
)

add_executable(cli cli/cli.cpp cli/handles.cpp)
target_link_libraries(cli PRIVATE
  protocol_cpp
  absl::log_internal_check_op
)

# 4. Vos flags stricts appliqués UNIQUEMENT sur vos binaires (server, gui, cli)
# On exclut volontairement protocol_cpp, protobuf, absl et yaml-cpp pour leur faire confiance.
set(MY_TARGETS server gui cli)

foreach(target ${MY_TARGETS})
  target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
endforeach()

option(ENABLE_SANITIZERS "Activer ASan et UBSan" OFF)
if (ENABLE_SANITIZERS)
  foreach(target ${MY_TARGETS})
    target_compile_options(${target} PRIVATE -ggdb3 -O0 -fsanitize=address,undefined)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endforeach()
endif()
