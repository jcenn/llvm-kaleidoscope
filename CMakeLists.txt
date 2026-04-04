cmake_minimum_required(VERSION 4.2)
project(llvm_kaleidoscope)

set(CMAKE_CXX_STANDARD 26)

add_executable(llvm_kaleidoscope main.cpp
        Lexer.cpp
        Lexer.h
        Token.h
        Parser.cpp
        Parser.h
)
