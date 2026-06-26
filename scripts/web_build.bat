@echo off

IF NOT EXIST build\web mkdir build\web
pushd build\web

set useRealAssets=0

if EXIST ..\..\data/original (
    set useRealAssets=1
    echo Using original assets
) else (
    echo Using default assets
)

set useCTime=1
if %useCTime% == 1 (
    if NOT EXIST ctime.exe (
        if EXIST ..\..\misc\ctime.exe (
            copy ..\..\misc\ctime.exe ctime.exe >nul
            echo Copied ctime.exe to build
            echo.
        ) else (
            echo ctime.exe not found in misc, disabling ctime
            echo.
            set useCTime=0
        )
    )
)

rem TODO: emcc defines and flags
set commonCompilerDefines=-DHANDMADE_WEB=1 -DHANDMADE_USE_REAL_ASSETS=%useRealAssets%
set commonCompilerWarnings=-Wall -Wextra -Wpedantic -Wno-unused-function -Wno-missing-braces -Wno-unused-variable -Wno-unused-parameter -Wno-null-dereference -Wno-missing-field-initializers -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-sign-compare
set commonCompilerFlags=-O0 -g2 -gsource-map --source-map-base http://localhost:8000/
rem -sASSERTIONS=1 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=1 -sALLOW_MEMORY_GROWTH=1

if %useRealAssets% == 1 (
    set commonCompilerFlags=%commonCompilerFlags% --preload-file ../../data/original@/original
) else (
    set commonCompilerFlags=%commonCompilerFlags% --preload-file ../../data/handmade@/handmade
)

if "%1" == "rel" (
    echo config: RELEASE
    set commonCompilerFlags=-O3
) else if "%1" == "release" (
    echo config: RELEASE
    set commonCompilerFlags=-O3
) else (
    echo config: DEBUG
    set commonCompilerDefines=%commonCompilerDefines% -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1
)

set commonCompilerFlags=%commonCompilerDefines% %commonCompilerFlags% %commonCompilerWarnings%
rem 32 MB
set initialMemory=33554432

echo.

set buildFailed=0

if %useCTime% == 1 (
    ctime.exe -begin web_handmade.ctm
)

echo web_handmade.cpp
echo emcc %commonCompilerFlags% ../../src/platform/web/web_handmade.cpp -I ../../src -sUSE_SDL=2 -sINITIAL_MEMORY=%initialMemory% -o web_handmade.html
emcc %commonCompilerFlags% ../../src/platform/web/web_handmade.cpp -I ../../src -sUSE_SDL=2 -sINITIAL_MEMORY=%initialMemory% -o web_handmade.html
if ERRORLEVEL 1 (
    set buildFailed=1
    echo [31m[1mweb_handmade.cpp failed[0m[1m
)

if %useCTime% == 1 (
    ctime.exe -end web_handmade.ctm
)

set NOW=%TIME:~0,8%

echo.
if %buildFailed% NEQ 0 (
    echo [31m[1mBuild failed[0m[1m %DATE% %NOW%
) else (
    echo [32m[1mBuild succeeded[0m[1m %DATE% %NOW%
    rem Copy source files for easier debugging
    rem https://wiki.libsdl.org/SDL2/README-emscripten
    copy ..\..\src\platform\web\web_handmade.cpp web_handmade.cpp >nul
    copy ..\..\src\platform\web\web_handmade.h web_handmade.h >nul
)

popd
