set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

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
  lib/Parser/Builtins.cpp
  lib/Parser/Lexer.cpp
  lib/Parser/Parser.cpp)

include_directories(include)
GENERATE_EXPORT_HEADER(AlliumParse)


add_library(AlliumSemAna SHARED
  lib/SemAna/ASTPrinter.cpp
  lib/SemAna/Builtins.cpp
  lib/SemAna/Predicates.cpp
  lib/SemAna/StaticError.cpp
  lib/SemAna/TypedAST.cpp
  lib/SemAna/VariableAnalysis.cpp)

target_link_libraries(AlliumSemAna PUBLIC AlliumParse)
GENERATE_EXPORT_HEADER(AlliumSemAna)

add_library(AlliumInterpreter SHARED
  lib/Interpreter/ASTLower.cpp
  lib/Interpreter/BuiltinEffects.cpp
  lib/Interpreter/Program.cpp
  lib/Interpreter/WitnessProducer.cpp)

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

#add_custom_target(functionaltests
#  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/tests/runner.py $<TARGET_FILE:allium>)
add_test(NAME functionaltests
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/tests/runner.py $<TARGET_FILE:allium>)

# set_target_properties(functionaltests PROPERTIES
#   EXCLUDE_FROM_ALL TRUE)
