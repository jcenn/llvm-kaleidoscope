#! /usr/bin/env bash

# Pass the program path (without extension as the first argument
# echo $1
while sleep 1;
do cmake-build-debug/llvm_kaleidoscope $1 > /dev/null && lli $1.ll ; echo "Program $1 returned $?";
done
