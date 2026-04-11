#!/usr/bin/env bash
set -euo pipefail

# MUST BE RUN FROM THE ROOT

mkdir -p build
pushd build

# TODO: take a look at the optimization flags later
# -O3 or 4? fpfastmath or similar, etc...

commonCompilerWarnings="-Wall -Wextra -Wpedantic -Wno-unused-function -Wno-missing-braces -Wno-unused-variable -Wno-unused-parameter -Wno-null-dereference -Wno-missing-field-initializers -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-sign-compare"
commonCompilerDefines="-DHANDMADE_LINUX=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1"
commonCompilerFlags="$commonCompilerDefines $commonCompilerWarnings -g -O0 -fno-exceptions -fno-rtti -std=c++20"

sdl2=$(sdl2-config --cflags --libs)

echo WAITING FOR PDB > lock.tmp

# Build game
clang++ $commonCompilerFlags ../src/game/handmade.cpp -I ../src -I ../src/game -shared -o handmade.so

rm lock.tmp

# Build platform
clang++ $commonCompilerFlags ../src/platform/linux/linux_handmade.cpp -I ../src -o linux_handmade $sdl2

popd
