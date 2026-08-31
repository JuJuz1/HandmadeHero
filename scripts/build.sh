#!/usr/bin/env bash
set -euo pipefail

# =============================================================================
# Handmade Hero Build
#
# Usage:
#   ./scripts/build.sh [platform] [compiler] [mode] [options]
#
# Platforms:
#   linux
#   mac
#   web
#
# Compilers:
#   clang
#   gcc       Linux only
#   emcc      Web only
#
# Modes:
#   debug
#   release
#   rel       same as release
#
# Options:
#   asan      Enable AddressSanitizer (native builds only)
#
# Examples:
#   ./scripts/build.sh linux
#   ./scripts/build.sh linux release
#   ./scripts/build.sh linux clang release
#   ./scripts/build.sh linux gcc rel
#
#   ./scripts/build.sh mac
#   ./scripts/build.sh mac release
#
#   ./scripts/build.sh web
#   ./scripts/build.sh web release
#
#   ./scripts/build.sh linux debug asan
#
# Arguments are order-independent:
#   ./scripts/build.sh release clang linux
#   ./scripts/build.sh asan linux debug
#
# Defaults:
#   linux -> clang debug
#   mac   -> clang debug
#   web   -> emcc  debug
#
# Doesn't fully handle all edge cases such as using asan with a web build
# =============================================================================

# --- Root Directory ----------------------------------------------------------

#SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
#ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

#cd "$ROOT_DIR"

# --- Arguments ---------------------------------------------------------------

platform=""
compiler="clang" # Default for Linux and mac
mode="debug"
useAsan=0

for arg in "$@"; do
    case "$arg" in
        linux)
            #if [[ -n "$platform" && "$platform" != "linux" ]]; then
            #    echo "ERROR: Multiple platforms specified."
            #    exit 1
            #fi
            platform="linux"
            ;;

        mac|macos)
            platform="mac"
            ;;

        web)
            platform="web"
            compiler="emcc"
            ;;

        clang)
            compiler="clang"
            ;;

        gcc)
            compiler="gcc"
            ;;

        emcc)
            compiler="emcc"
            ;;

        rel|release)
            mode="release"
            ;;

        asan)
            useAsan=1
            ;;

        *)
            #echo "ERROR: Unknown build argument: '$arg'"
            #echo
            #echo "Usage:"
            #echo "  ./scripts/build.sh [linux|mac|web] [clang|gcc|emcc] [debug|release] [asan]"
            #exit 1
            ;;
    esac
done

if [[ -z "$platform" ]]; then
    echo "ERROR: Specify the platform [linux|mac|web]"
    exit 1
fi

# --- Platform Default --------------------------------------------------------

#if [[ -z "$platform" ]]; then
#    case "$(uname -s)" in
#        Linux)
#            platform="linux"
#            ;;

#        Darwin)
#            platform="mac"
#            ;;

#        *)
#            echo "ERROR: Could not determine the native platform."
#            echo "Specify one explicitly: linux, mac, or web."
#            exit 1
#            ;;
#    esac
#fi

# --- Compiler Defaults -------------------------------------------------------

#if [[ -z "$compiler" ]]; then
#    case "$platform" in
#        linux|mac)
#            compiler="clang"
#            ;;

#        web)
#            compiler="emcc"
#            ;;
#    esac
#fi

# --- Validate Platform / Compiler --------------------------------------------

#case "$platform" in
#    linux)
#        if [[ "$compiler" != "clang" && "$compiler" != "gcc" ]]; then
#            echo "ERROR: Linux supports clang or gcc, not '$compiler'."
#            exit 1
#        fi
#        ;;

#    mac)
#        if [[ "$compiler" != "clang" ]]; then
#            echo "ERROR: macOS currently supports clang only."
#            exit 1
#        fi

#        if [[ "$(uname -s)" != "Darwin" ]]; then
#            echo "ERROR: macOS builds must be performed on macOS."
#            exit 1
#        fi
#        ;;

#    web)
#        if [[ "$compiler" != "emcc" ]]; then
#            echo "ERROR: Web builds require emcc."
#            exit 1
#        fi

#        if [[ "$useAsan" == "1" ]]; then
#            echo "ERROR: The 'asan' option is currently supported only for native builds."
#            exit 1
#        fi
#        ;;
#esac

echo "[PLATFORM: ${platform}]"
echo "[COMPILER: ${compiler}]"
echo "[CONFIG: ${mode}]"

if [[ "$useAsan" == "1" ]]; then
    if [[ "$platform" == "web" ]]; then
        # TODO: can it be?
        useAsan=0
        echo "ASAN not available on web build"
    else
        echo "[ASAN: ENABLED]"
    fi
fi

echo

# --- Build dir ----------------------------------------------------------------

mkdir -p build
pushd build > /dev/null

# TODO: runtime
# --- Assets Check -------------------------------------------------------------

useRealAssets=0
if [[ -d "../data/original" ]]; then
    useRealAssets=1
    echo "Using original assets"
else
    echo "Using default assets"
fi

# --- Git Commit Info ---------------------------------------------------------

#gitHash="unknown"
#gitHashFull="unknown"

#if command -v git >/dev/null 2>&1; then
#    gitHash="$(git describe --always --dirty 2>/dev/null || echo unknown)"
#    gitHashFull="$(git rev-parse HEAD 2>/dev/null || echo unknown)"
#fi

# --- Common Defines ----------------------------------------------------------

commonDefines=(
    "-DHANDMADE_USE_REAL_ASSETS=$useRealAssets"
    #"-DBUILD_GIT_HASH=\"$gitHash\""
    #"-DBUILD_GIT_HASH_FULL=\"$gitHashFull\""
    "-DHANDMADE_INTERNAL=1"
    "-DHANDMADE_DEBUG=1"
)

case "$platform" in
    linux)
        commonDefines+=("-DHANDMADE_LINUX=1")
        ;;

    mac)
        commonDefines+=("-DHANDMADE_MAC=1")
        ;;

    web)
        commonDefines+=("-DHANDMADE_WEB=1")
        ;;
esac

# --- Build Mode --------------------------------------------------------------

modeFlags=(
    -O0
    -g
)

if [[ "$mode" == "release" ]]; then
    modeFlags=(
        -O3
    )
fi

# --- Common Warnings ---------------------------------------------------------

commonWarnings=(
    -Wall
    -Wextra
    -Wpedantic

    -Wno-unused-function
    -Wno-missing-braces
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-null-dereference
    -Wno-missing-field-initializers
    -Wno-gnu-anonymous-struct
    -Wno-nested-anon-types
    -Wno-sign-compare
    -Wno-gnu-zero-variadic-macro-arguments
)

# =============================================================================
# Linux and Mac
# =============================================================================

if [[ "$platform" == "linux" || "$platform" == "mac" ]]; then
    CXX=clang++
    if [[ "$compiler" == "gcc" ]]; then
        # TODO: MAKE WORK AND don't allow for mac
        #CXX=g++
        echo "GCC NOT SUPPORTED YET"
        exit 1
    fi

    nativeFlags=(
        "${commonDefines[@]}"
        "${modeFlags[@]}"

        -fno-exceptions
        -fno-rtti
        #-fPIC

        -std=c++20

        "${commonWarnings[@]}"
    )

    outDll="handmade.so"
    outExe="linux_handmade"
    if [[ "$platform" == "linux" ]]; then
        nativeFlags+=(-fPIC)
    else
        outDll="handmade.dylib"
        outExe="macos_handmade"
    fi

    sanitizeFlags=()

    if [[ "$useAsan" == "1" ]]; then
        sanitizeFlags+=(
            -fsanitize=address
            -fno-omit-frame-pointer
        )

        nativeFlags+=("${sanitizeFlags[@]}")
    fi

    sdl2=$(sdl2-config --cflags --libs)

    echo
    echo "WAITING FOR PDB" > lock.tmp

    "$CXX" \
        "${nativeFlags[@]}" \
        ../src/game/handmade.cpp \
        -I ../src \
        -shared \
        -o "$outDll"

    rm -f lock.tmp

    echo

    "$CXX" \
        "${nativeFlags[@]}" \
        ../src/platform/sdl/sdl_handmade.cpp \
        -I ../src \
        -o "$outExe" \
        $sdl2
# Web
elif [[ "$platform" == "web" ]]; then
    mkdir -p web
    pushd web > /dev/null

    webFlags=(
        "${commonDefines[@]}"
        "${modeFlags[@]}"

        -std=c++20

        "${commonWarnings[@]}"

        -sUSE_SDL=2
        -sINITIAL_MEMORY=134217728

        # -sASSERTIONS=1 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=1 -sALLOW_MEMORY_GROWTH=1

        # -sEXPORTED_FUNCTIONS=_main,_ToggleFullscreen -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

        -gsource-map
        --source-map-base
        "http://localhost:8000/"
    )

    dataPath="../../data/original@/original"
    if [ "$useRealAssets" = "0" ]; then
        dataPath="../../data/handmade@/handmade"
    fi

    #if [[ "$mode" == "debug" ]]; then
    #    webFlags+=(
    #        -gsource-map
    #        --source-map-base
    #        "http://localhost:8000/"
    #    )
    #fi

    echo "web_handmade.cpp"
    echo emcc \
        "${webFlags[@]}" \
        ../../src/platform/web/web_handmade.cpp \
        -I ../../src \
        --preload-file "$dataPath" \
        -o web_handmade.html

    emcc \
        "${webFlags[@]}" \
        ../../src/platform/web/web_handmade.cpp \
        -I ../../src \
        --preload-file "$dataPath" \
        -o web_handmade.html

    # Copy source files for easier debugging
    # https://wiki.libsdl.org/SDL2/README-emscripten
    cp ../../src/platform/web/web_handmade.cpp web_handmade.cpp
    cp ../../src/platform/web/web_handmade.h web_handmade.h
fi

popd >/dev/null

# --- Result ------------------------------------------------------------------

#echo
#echo "Build succeeded $(date '+%Y-%m-%d %H:%M:%S')"
