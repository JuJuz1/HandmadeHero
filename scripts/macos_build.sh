#!/usr/bin/env bash
set -euo pipefail

# MUST BE RUN FROM THE ROOT

mkdir -p build
pushd build

useRealAssets=0

if [ -d "../data/original" ]; then
    useRealAssets=1
    echo "Using original assets"
else
    echo "Using default assets"
fi

commonCompilerDefines="-DHANDMADE_MACOS=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1 -DHANDMADE_USE_REAL_ASSETS=$useRealAssets"

commonCompilerWarnings="-Wall -Wextra -Wpedantic -Wno-unused-function -Wno-missing-braces -Wno-unused-variable -Wno-unused-parameter -Wno-null-dereference -Wno-missing-field-initializers -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-sign-compare"
# -O3 when testing on the VM as it lags quite hard (-O0 debug)
commonCompilerFlags="$commonCompilerDefines $commonCompilerWarnings -g -O3 -fno-exceptions -fno-rtti -std=c++20"

sdl2=$(sdl2-config --cflags --libs)

echo WAITING FOR PDB > lock.tmp

# Build game
clang++ $commonCompilerFlags ../src/game/handmade.cpp -I ../src -shared -o handmade.dylib

rm lock.tmp

# Build platform
clang++ $commonCompilerFlags ../src/platform/sdl/sdl_handmade.cpp -I ../src -o sdl_handmade $sdl2

popd
