
# Add a flag to control whether Allium should be built as a compiler or an
# interpreter. Setting this to OFF (default) will build just the interpreter,
# with the old CLI. This makes it possible to build Allium without LLVM, and
# is a stopgap while the new LLVM-based code generation capabilities catch up with
# the old interpreter.
option(BUILD_COMPILER "Whether or not to build Allium as a compiler (rather than an interpreter)" OFF)

#############################
# Allium
#############################

add_executable(allium lib/driver.cpp)

include_directories(include)

target_link_libraries(allium PUBLIC AlliumParse)
target_link_libraries(allium PUBLIC AlliumSemAna)
target_link_libraries(allium PUBLIC AlliumInterpreter)

if(BUILD_COMPILER)
  add_compile_definitions("ENABLE_COMPILER")
  target_link_libraries(allium PUBLIC AlliumLLVMCodeGen)
endif()

#############################
# libraries
#############################

include(GenerateExportHeader)

add_library(AlliumParse SHARED
  lib/Parser/AST.cpp
  lib/Parser/Builtins.cpp
  lib/Parser/Lexer.cpp
  lib/Parser/Parser.cpp)

GENERATE_EXPORT_HEADER(AlliumParse)


add_library(AlliumSemAna SHARED
  lib/SemAna/ASTPrinter.cpp
  lib/SemAna/Builtins.cpp
  lib/SemAna/GroundAnalysis.cpp
  lib/SemAna/InhabitableAnalysis.cpp
  lib/SemAna/Predicates.cpp
  lib/SemAna/PredRecursionAnalysis.cpp
  lib/SemAna/StaticError.cpp
  lib/SemAna/TypedAST.cpp
  lib/SemAna/TypeRecursionAnalysis.cpp
  lib/SemAna/VariableAnalysis.cpp)

target_link_libraries(AlliumSemAna PUBLIC AlliumParse)
GENERATE_EXPORT_HEADER(AlliumSemAna)


add_library(AlliumInterpreter SHARED
  lib/Interpreter/ASTLower.cpp
  lib/Interpreter/BuiltinEffects.cpp
  lib/Interpreter/BuiltinPredicates.cpp
  lib/Interpreter/Program.cpp
  lib/Interpreter/WitnessProducer.cpp)

target_link_libraries(AlliumInterpreter PUBLIC AlliumSemAna)
GENERATE_EXPORT_HEADER(AlliumInterpreter)

if(BUILD_COMPILER)
  add_subdirectory(lib/LLVMCodeGen)
endif()

add_library(AlliumRuntime SHARED
  lib/LibAllium/libAllium.c)
set_target_properties(AlliumRuntime PROPERTIES
  OUTPUT_NAME Allium)

#############################
# unit tests
#############################

add_executable(unittests
  unittests/TestASTLower.cpp
  unittests/TestASTRaise.cpp
  unittests/TestGenerator.cpp
  unittests/TestInterpreter.cpp
  unittests/TestInterpreterBuiltins.cpp
  unittests/TestOptional.cpp
  unittests/TestParse.cpp
  unittests/TestSema.cpp
  unittests/TestTaggedUnion.cpp)
add_test(NAME unittests COMMAND unittests)

set_target_properties(unittests PROPERTIES
  EXCLUDE_FROM_ALL TRUE)
target_link_libraries(unittests gtest_main)
target_link_libraries(unittests gmock)
target_link_libraries(unittests AlliumParse)
target_link_libraries(unittests AlliumSemAna)
target_link_libraries(unittests AlliumInterpreter)

#############################
# functional tests
#############################

add_test(NAME functionaltests
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/runner.py $<TARGET_FILE:allium> ${FILE_CHECK})
