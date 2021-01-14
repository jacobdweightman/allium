cmake_minimum_required(VERSION 3.10)

project(allium VERSION 0.0.1)

enable_testing()

add_executable(allium
  lib/driver.cpp
  lib/Interpreter/ASTLower.cpp
  lib/Interpreter/program.cpp)

target_include_directories(allium PUBLIC include)
target_link_libraries(allium PUBLIC AlliumParse)
target_link_libraries(allium PUBLIC AlliumSemAna)

set_property(TARGET allium PROPERTY CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#############################
# libraries
#############################

add_library(AlliumParse SHARED
  lib/parse/AST.cpp
  lib/parse/lexer.cpp
  lib/parse/parser.cpp)

target_include_directories(AlliumParse PUBLIC include)
set_property(TARGET AlliumParse PROPERTY CXX_STANDARD 17)


add_library(AlliumSemAna SHARED
  lib/SemAna/Predicates.cpp
  lib/SemAna/StaticError.cpp)

target_include_directories(AlliumSemAna PUBLIC include)
target_link_libraries(AlliumSemAna PUBLIC AlliumParse)
set_property(TARGET AlliumSemAna PROPERTY CXX_STANDARD 17)

#############################
# unit tests
#############################

add_executable(unittests
  unittests/TestOptional.cpp
  unittests/TestTaggedUnion.cpp
  unittests/TestParse.cpp
  unittests/TestSema.cpp)
add_test(unittests unittests)

set_target_properties(unittests PROPERTIES
  CXX_STANDARD 17
  EXCLUDE_FROM_ALL TRUE)
target_include_directories(unittests PUBLIC include)
target_link_libraries(unittests gtest_main)
target_link_libraries(unittests gmock)
target_link_libraries(unittests AlliumParse)
target_link_libraries(unittests AlliumSemAna)

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
