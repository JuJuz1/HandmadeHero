#!/usr/bin/env bash
set -euo pipefail

# MUST BE RUN FROM THE ROOT

mkdir -p build
pushd build

#commonCompilerWarnings="-Wall -Wextra -Wpedantic"
commonCompilerWarnings="-Wall -Wextra -Wno-unused-function -Wno-missing-braces -Wno-unused-variable -Wno-unused-parameter -Wno-null-dereference"
commonCompilerDefines="-DHANDMADE_LINUX=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1"
commonCompilerFlags="$commonCompilerDefines $commonCompilerWarnings -g -std=c++20"
#commonLinkerFlags=/OPT:REF /OPT:NOICF /INCREMENTAL:NO

#linuxLibraries??? =User32.lib Gdi32.lib Winmm.lib
#gameExportedFunctions=/EXPORT:UpdateAndRender /EXPORT:GetSoundSamples

sdl2=$(sdl2-config --cflags --libs)

# Build
clang++ $commonCompilerFlags ../src/handmade.cpp -I ../src -shared -o handmade.so

clang++ $commonCompilerFlags ../src/linux/linux_handmade.cpp -I ../src -o linux_handmade $sdl2


popd
