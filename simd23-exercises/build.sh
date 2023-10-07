#!/bin/bash

mkdir -p build_x64_opt
mkdir -p build_x64_debug

warnings="-Wall -Werror -Wuninitialized -Wno-missing-braces -Wno-unused-variable -Wno-unused-function -Wno-error=deprecated-declarations -Wno-sign-compare -Wno-write-strings -Wno-unused-local-typedefs -Wno-unused-value -Wno-unused-but-set-variable"

echo "[build.sh] main.cpp debug build"
flags="-lm -pthread -ldl -mavx2 -fno-exceptions -g3 $warnings -I../"
g++ main.cpp -o build_x64_debug/exercises $flags -O0
if [[ ! $? -eq 0 ]]; then
  echo "compilation of main.cpp failed (debug)"
  exit 1
fi

echo "[build.sh] main.cpp optimized build"
g++ main.cpp -o build_x64_opt/exercises $flags -O2
if [[ ! $? -eq 0 ]]; then
  echo "compilation of main.cpp failed (optimized)"
  exit 1
fi

echo "[build.sh] running tests"
build_x64_opt/exercises
if [[ ! $? -eq 0 ]]; then
  echo "Profiling of the program crashed :-("
  exit 1
fi

exit 0
