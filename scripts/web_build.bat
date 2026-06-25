@echo off

IF NOT EXIST build\web mkdir build\web
pushd build\web

set useRealAssets=0

if EXIST ../../data/original (
    set useRealAssets=1
    echo Using original assets
) else (
    echo Using default assets
)

set useCTime=1
if %useCTime% == 1 (
    if not exist ctime.exe (
        if exist ..\..\misc\ctime.exe (
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

set commonCompilerDefines=-DHANDMADE_WEB=1 -DHANDMADE_USE_REAL_ASSETS=%useRealAssets%
set commonCompilerFlags=-O0

if "%1" == "rel" (
    echo config: RELEASE
    set commonCompilerFlags=-O2
) else if "%1" == "release" (
    echo config: RELEASE
    set commonCompilerFlags=-O2
) else (
    echo config: DEBUG
    set commonCompilerDefines=%commonCompilerDefines% -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1
)

set commonCompilerFlags=%commonCompilerDefines% %commonCompilerFlags%

echo.

set buildFailed=0

if %useCTime% == 1 (
    ctime.exe -begin web_handmade.ctm
)

echo web_handmade.cpp
echo emcc %commonCompilerFlags% ../../src/platform/web/web_handmade.cpp -o web_handmade.html
emcc %commonCompilerFlags% ../../src/platform/web/web_handmade.cpp -o web_handmade.html
if ERRORLEVEL 1 (
    set buildFailed=1
    echo [31m[1mweb_handmade.cpp failed[0m[1m
)

if %useCTime% == 1 (
    ctime.exe -end web_handmade.ctm
)

popd

set NOW=%TIME:~0,8%

echo.
if %buildFailed% NEQ 0 (
    echo [31m[1mBuild failed[0m[1m %DATE% %NOW%
) else (
    echo [32m[1mBuild succeeded[0m[1m %DATE% %NOW%
)
