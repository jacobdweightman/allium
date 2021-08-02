cmake_minimum_required(VERSION 3.10)

project(allium VERSION 0.0.1)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
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

add_executable(allium lib/driver.cpp)

target_include_directories(allium PUBLIC include)
target_link_libraries(allium PUBLIC AlliumParse)
target_link_libraries(allium PUBLIC AlliumSemAna)
target_link_libraries(allium PUBLIC AlliumInterpreter)

#############################
# libraries
#############################

include(GenerateExportHeader)

add_library(AlliumParse SHARED
  lib/Parser/AST.cpp
  lib/Parser/Lexer.cpp
  lib/Parser/Parser.cpp)

target_include_directories(AlliumParse PUBLIC include)
GENERATE_EXPORT_HEADER(AlliumParse)


add_library(AlliumSemAna SHARED
  lib/SemAna/ASTPrinter.cpp
  lib/SemAna/Predicates.cpp
  lib/SemAna/StaticError.cpp
  lib/SemAna/TypedAST.cpp
  lib/SemAna/VariableAnalysis.cpp)

target_include_directories(AlliumSemAna PUBLIC include)
target_link_libraries(AlliumSemAna PUBLIC AlliumParse)
GENERATE_EXPORT_HEADER(AlliumSemAna)

add_library(AlliumInterpreter SHARED
  lib/Interpreter/ASTLower.cpp
  lib/Interpreter/Program.cpp
  lib/Interpreter/WitnessProducer.cpp)

target_include_directories(AlliumInterpreter PUBLIC include)
target_link_libraries(AlliumInterpreter PUBLIC AlliumSemAna)
GENERATE_EXPORT_HEADER(AlliumInterpreter)

#############################
# unit tests
#############################

add_executable(unittests
  unittests/TestASTLower.cpp
  unittests/TestASTRaise.cpp
  unittests/TestGenerator.cpp
  unittests/TestInterpreter.cpp
  unittests/TestOptional.cpp
  unittests/TestParse.cpp
  unittests/TestSema.cpp
  unittests/TestTaggedUnion.cpp)
add_test(unittests unittests)

set_target_properties(unittests PROPERTIES
  EXCLUDE_FROM_ALL TRUE)
target_include_directories(unittests PUBLIC include)
target_link_libraries(unittests gtest_main)
target_link_libraries(unittests gmock)
target_link_libraries(unittests AlliumParse)
target_link_libraries(unittests AlliumSemAna)
target_link_libraries(unittests AlliumInterpreter)

#############################
# functional tests
#############################

add_custom_target(functionaltests
  COMMAND python ${CMAKE_SOURCE_DIR}/tests/runner.py $<TARGET_FILE:allium>)
add_test(NAME functionaltests
  COMMAND python ${CMAKE_SOURCE_DIR}/tests/runner.py $<TARGET_FILE:allium>)

set_target_properties(functionaltests PROPERTIES
  EXCLUDE_FROM_ALL TRUE)

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
