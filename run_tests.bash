#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j --target chip8_tests
./build/chip8_tests "$@"
