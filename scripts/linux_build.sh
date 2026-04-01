#!/usr/bin/env bash
set -euo pipefail

# MUST BE RUN FROM THE ROOT

mkdir -p build
pushd build

commonCompilerWarnings="-Wall -Wextra -Wpedantic"
commonCompilerDefines="-DHANDMADE_LINUX=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1"
commonCompilerFlags="$commonCompilerDefines $commonCompilerWarnings -std=c++20"
#commonLinkerFlags=/OPT:REF /OPT:NOICF /INCENTAL:NO

#linuxLibraries??? =User32.lib Gdi32.lib Winmm.lib
#gameExportedFunctions=/EXPORT:UpdateAndRender /EXPORT:GetSoundSamples

# Build
clang++ "$commonCompilerFlags" -c ../src/linux/linux_handmade.cpp -I ../src -o linux_handmade.o
# Link
clang++ linux_handmade.o -o linux_handmade

popd
