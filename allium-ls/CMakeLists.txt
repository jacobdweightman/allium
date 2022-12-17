add_executable(allium-ls lib/main.cpp)

include_directories(include)

target_link_libraries(allium-ls PUBLIC AlliumParse)
target_link_libraries(allium-ls PUBLIC AlliumSemAna)
target_link_libraries(allium-ls PUBLIC AlliumLS)

#############################
# libraries
#############################

include(GenerateExportHeader)

add_library(AlliumLS SHARED
  lib/JSON.cpp
  lib/LSPServer.cpp
  lib/LSPTypes.cpp)

GENERATE_EXPORT_HEADER(AlliumLS)

target_include_directories(AlliumLS PUBLIC include)
include_directories(../allium/include)

#############################
# unit tests
#############################

add_executable(allium-ls-unittests
  unittests/TestDecoding.cpp)
add_test(NAME allium-ls-unittests COMMAND allium-ls-unittests)

set_target_properties(allium-ls-unittests PROPERTIES
  EXCLUDE_FROM_ALL TRUE)
target_link_libraries(allium-ls-unittests AlliumLS)
target_link_libraries(allium-ls-unittests gtest_main)

#############################
# functional tests
#############################

add_test(NAME language-server-functional-tests
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/languageserver.py $<TARGET_FILE:allium-ls>)
