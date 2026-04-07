#! /usr/bin/env bash

# Pass the program path (without extension as the first argument
echo "Running $1..."
cmake-build-debug/llvm_kaleidoscope $1 && lli $1.ll ; echo "Program $1 returned $?"

