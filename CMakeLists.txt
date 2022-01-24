cmake_minimum_required(VERSION 3.12)

project(allium VERSION 0.0.1)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options("-fcoroutines-ts")
  add_compile_options("-stdlib=libc++")
  add_link_options("-stdlib=libc++")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options("-fcoroutines")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_package(Python3 COMPONENTS Interpreter)
message("python: ${Python3_EXECUTABLE}")

find_program(FILE_CHECK NAMES FileCheck FileCheck-12)
message("file check: ${FILE_CHECK}")

add_subdirectory(allium)
add_subdirectory(allium-ls)

#############################
# googletest
#############################
# Source: https://github.com/google/googletest/blob/master/googletest/README.md

configure_file(CMakeLists-googletest.txt googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)
