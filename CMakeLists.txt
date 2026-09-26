cmake_minimum_required(VERSION 3.20)
project(TheAnswerProtocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(FetchContent)
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.9.0
    )
    FetchContent_MakeAvailable(yaml-cpp)
endif()

find_package(Threads REQUIRED)

add_library(tap_common
    common/Logger.cpp
    common/Protocol.cpp
    common/Socket.cpp
)
target_include_directories(tap_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tap_common PUBLIC Threads::Threads)

add_library(tap_game
    server/Game.cpp
    server/World.cpp
)
target_include_directories(tap_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tap_game PUBLIC tap_common yaml-cpp)

add_executable(tap-server server/server.cpp)
target_link_libraries(tap-server PRIVATE tap_game)

add_executable(tap-cli cli/cli.cpp)
target_link_libraries(tap-cli PRIVATE tap_common)

set(TAP_TARGETS tap_common tap_game tap-server tap-cli)
foreach(target IN LISTS TAP_TARGETS)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endforeach()

option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
if(ENABLE_SANITIZERS)
    foreach(target IN LISTS TAP_TARGETS)
        target_compile_options(${target} PRIVATE -O0 -g3 -fsanitize=address,undefined)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(tap-tests tests/tests.cpp)
    target_link_libraries(tap-tests PRIVATE tap_game)
    target_compile_options(tap-tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_definitions(
        tap-tests PRIVATE TAP_WORLD_PATH="${CMAKE_CURRENT_SOURCE_DIR}/world.yaml"
    )
    if(ENABLE_SANITIZERS)
        target_compile_options(tap-tests PRIVATE -O0 -g3 -fsanitize=address,undefined)
        target_link_options(tap-tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME tap-tests COMMAND tap-tests)
endif()
