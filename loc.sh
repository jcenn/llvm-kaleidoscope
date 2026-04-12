#! /usr/bin/env bash
cloc . --exclude-dir=cmake-build-debug,.git,.idea,.direnv --include-lang="C++,C/C++ Header"

