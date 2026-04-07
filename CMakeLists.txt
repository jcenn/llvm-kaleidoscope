cmake_minimum_required(VERSION 4.1)
project(llvm_kaleidoscope)

set(CMAKE_CXX_STANDARD 26)

# To avoid debugging issues
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")



add_executable(llvm_kaleidoscope main.cpp
        Lexer.cpp
        Lexer.h
        Token.h
        Parser.cpp
        Parser.h
        ast.cpp
        ast.h
        TestRunner.cpp
        TestRunner.h
)

# ... rest of your setup ...
include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})
# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core)

# Link against LLVM libraries
target_link_libraries(llvm_kaleidoscope ${llvm_libs})