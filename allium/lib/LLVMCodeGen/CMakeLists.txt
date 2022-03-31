cmake_minimum_required(VERSION 3.13.4)
project(AlliumLLVMCodegen)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(AlliumLLVMCodeGen SHARED
  CGPred.cpp
  CGType.cpp
  CodeGen.cpp
  LogInstrumentor.cpp)

llvm_map_components_to_libnames(llvm_libs core support passes analysis coroutines ipo mc ${LLVM_TARGETS_TO_BUILD})
message(STATUS "LLVM library names: ${llvm_libs}")
target_link_libraries(AlliumLLVMCodeGen AlliumSemAna)
target_link_libraries(AlliumLLVMCodeGen ${llvm_libs})

