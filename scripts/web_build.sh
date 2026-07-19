#!/usr/bin/env bash
set -euo pipefail

# Basically a one-to-one copy of web_build.bat

mkdir -p build/web
pushd build/web

useRealAssets=0

if [ -d ../../data/original ]; then
    useRealAssets=1
    echo "Using original assets"
else
    echo "Using default assets"
fi

useCTime=1
if [ "$useCTime" = "1" ]; then
    if [ ! -f ctime.exe ]; then
        if [ -f ../../misc/ctime.exe ]; then
            cp ../../misc/ctime.exe ctime.exe
            echo "Copied ctime.exe to build"
            echo
        else
            echo "ctime.exe not found in misc, disabling ctime"
            echo
            useCTime=0
        fi
    fi
fi

commonCompilerDefines="-DHANDMADE_WEB=1 -DHANDMADE_USE_REAL_ASSETS=$useRealAssets"
commonCompilerWarnings="-Wall -Wextra -Wpedantic -Wno-unused-function -Wno-missing-braces -Wno-unused-variable -Wno-unused-parameter -Wno-null-dereference -Wno-missing-field-initializers -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-sign-compare"
commonCompilerFlags="-gsource-map --source-map-base http://localhost:8000/"
# -sASSERTIONS=1 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=1 -sALLOW_MEMORY_GROWTH=1

# -sEXPORTED_FUNCTIONS=_main,_ToggleFullscreen -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

if [ "$useRealAssets" = "1" ]; then
    commonCompilerFlags="$commonCompilerFlags --preload-file ../../data/original@/original"
else
    commonCompilerFlags="$commonCompilerFlags --preload-file ../../data/handmade@/handmade"
fi

if [ "${1:-}" = "rel" ]; then
    echo "config: RELEASE"
    commonCompilerFlags="$commonCompilerFlags -O3"
elif [ "${1:-}" = "release" ]; then
    echo "config: RELEASE"
    commonCompilerFlags="$commonCompilerFlags -O3"
else
    echo "config: DEBUG"
    commonCompilerFlags="$commonCompilerFlags -O0 -g2"
    commonCompilerDefines="$commonCompilerDefines -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1"
fi

commonCompilerFlags="$commonCompilerDefines $commonCompilerFlags $commonCompilerWarnings"
# 32 MB
initialMemory=33554432

echo

buildFailed=0

if [ "$useCTime" = "1" ]; then
    ./ctime.exe -begin web_handmade_sh.ctm
fi

echo "web_handmade.cpp"
echo emcc $commonCompilerFlags ../../src/platform/web/web_handmade.cpp -I ../../src -sUSE_SDL=2 -sINITIAL_MEMORY=$initialMemory -o web_handmade.html

if ! emcc $commonCompilerFlags ../../src/platform/web/web_handmade.cpp -I ../../src -sUSE_SDL=2 -sINITIAL_MEMORY=$initialMemory -o web_handmade.html; then
    buildFailed=1
    printf "\033[31m\033[1mweb_handmade.cpp failed\033[0m\033[1m\n"
fi

if [ "$useCTime" = "1" ]; then
    ./ctime.exe -end web_handmade_sh.ctm
fi

NOW=$(date +"%H:%M:%S")

echo
if [ "$buildFailed" -ne 0 ]; then
    printf "\033[31m\033[1mBuild failed\033[0m\033[1m %s %s\n" "$(date +"%Y-%m-%d")" "$NOW"
else
    printf "\033[32m\033[1mBuild succeeded\033[0m\033[1m %s %s\n" "$(date +"%Y-%m-%d")" "$NOW"
    # Copy source files for easier debugging
    # https://wiki.libsdl.org/SDL2/README-emscripten
    cp ../../src/platform/web/web_handmade.cpp web_handmade.cpp
    cp ../../src/platform/web/web_handmade.h web_handmade.h
fi

popd
