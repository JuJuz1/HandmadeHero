@echo off
setlocal enabledelayedexpansion

rem A faster alternative to build.bat from VSCode which does the same but doesn't initialize the
rem environment for no reason if it was already initialized in the shell

IF NOT EXIST build mkdir build
pushd build
rem We are now inside build

rem /TP /EHsc- /GR- /O2 /Ob2 /Fe:test.exe
rem /TP compile all files as c++
rem /EHa- disable exception handling for asynchronous and synchronous
rem /GR- disable RTTI
rem /Bt better output info

rem /Zc:__cplusplus make __cplusplus reflect true version of the standard used https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-170

rem /MT vs /MD https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-170
rem by default the cli uses /MT
rem we use /MTd to add extra checks

rem https://learn.microsoft.com/en-us/cpp/build/reference/o-options-optimize-code?view=msvc-170
rem /Oi generate intrinsic functions for appropriate function calls
rem /fp:fast to optimize e.g. CRT floorf calls

rem /nologo don't show compiler version and info

rem /Fm map file https://learn.microsoft.com/fi-fi/cpp/build/reference/fm-name-mapfile?view=msvc-150

rem pass to linker:
rem /OPT https://learn.microsoft.com/en-us/cpp/build/reference/opt-optimizations?view=msvc-170
rem /OPT:REF disable functions and data which are not referenced
rem recommended by docs: /OPT:NOICF to preserve identical functions in debug builds
rem link the User32.lib, Gdi32.lib to create UI

rem /LD to tell linker its going to be a dll
rem https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-170
rem /EXPORT to specify which functions to export with the dll
rem could also specify in the source and is most recommended
rem this way we get the maximum flexibility though and we don't boilerplate the source

rem /wd4201 nonstandard extension used: nameless struct/union
rem /wd4127 conditional expression is constant NOT USED
rem TODO: enable /WX back, remove /wd4505 /wd4100 /wd4189

set "platform=win32"
set "compiler=msvc"
set "mode=debug"
set "useAsan=0"

rem Additional arguments:
rem clang, release, rel, asan

for %%a in (%*) do set "%%~a=1"
if "%clang%" == "1" set "compiler=clang"
if "%rel%" == "1" set "mode=release"
if "%release%" == "1" set "mode=release"
if "%asan%" == "1" set "useAsan=1"

echo [PLATFORM: %platform%]
echo [COMPILER: %compiler%]
echo [CONFIG: %mode%]

if "%useAsan%" == "1" (
    echo [ASAN: enabled]
)

echo.

rem search if the preordered data assets exist
set useRealAssets=0

if EXIST ..\data\original (
    set useRealAssets=1
    echo Using original assets
) else (
    echo Using default assets
)

set useCTime=1
if %useCTime% == 1 (
    if NOT EXIST ctime.exe (
        if EXIST ..\misc\ctime.exe (
            copy ..\misc\ctime.exe ctime.exe >nul
            echo Copied ctime.exe to build
            echo.
        ) else (
            echo ctime.exe not found in misc, disabling ctime
            echo.
            set useCTime=0
        )
    )
)


rem --- Git Commit Info ---------------------------------------------------------
rem set "gitHash=unknown"
rem set "gitHashFull=unknown"
rem for /f %%i in ('git describe --always --dirty >nul') do set "gitHash=%%i"
rem for /f %%i in ('git rev-parse HEAD >nul') do set "gitHashFull=%%i"


rem TODO: HANDMADE_INTERNAL=1 for release mode also for now
set "commonDefines=-DHANDMADE_WIN32=1 -DHANDMADE_USE_REAL_ASSETS=%useRealAssets% -DHANDMADE_INTERNAL=1"

rem TODO: make ASAN work, seems to not work if we do DirectSound initialization stuff...
rem pretty weird but disabling any dsound related stuff makes it work
rem Also using it even on /O2 is absurdly slow...
rem /fsanitize=address

rem --- Compiler options --------------------------------------------------------

set "cxx=cl"
set "modeFlags=-MTd -Od -Zi"
set "commonFlags=-Zc:__cplusplus -FC -Oi -EHa- -GR- -nologo -std:c++20"
rem /wd4100 unreferenced param /wd4189 local variable init but not referenced
rem /wd4189 /wd4100
set "commonWarnings=-W4 -wd4201 -wd4505 -wd4189 -wd4100"
set "dllFlags=-LDd"
rem Combine linkerFlags with clang version?
set "linkerFlags=-link -OPT:REF -OPT:NOICF -INCREMENTAL:NO"
rem Visual studio broke with this one :D 4.9.2026, couldn't load symbols for the dll
rem -PDB:handmade_%random%.pdb
set "outDll=-Fe:handmade.dll"
set "outExe=-Fe:win32_handmade.exe"

if "%mode%" == "release" (
    set "modeFlags=-MT -O2"
    set "dllFlags=-LD"
)

set "win32Libraries=User32.lib Gdi32.lib Winmm.lib"
set "gameExportedFunctions=-EXPORT:UpdateAndRender -EXPORT:GetSoundSamples"

if "%compiler%" == "clang" (
    rem TODO: why not just use clang-cl?
    rem would still have to specify warnings the "clang" way but other stuff would work nicely
    set "cxx=clang++"
    set "modeFlags=-O0 -g"
    set "commonFlags=-fno-exceptions -fno-rtti -std=c++20"
    rem Remove last two disabling warnings
    set "commonWarnings=-Wall -Wextra -Wpedantic -Wno-unused-function -Wno-missing-braces -Wno-unused-variable -Wno-unused-parameter -Wno-null-dereference -Wno-missing-field-initializers -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-sign-compare -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-but-set-variable -Wno-unused-value"
    set "dllFlags=-shared"
    set "linkerFlags=-Wl,-opt:ref,-opt:noicf,-incremental:no"
    set "outDll=-o handmade.dll"
    set "outExe=-o win32_handmade.exe"

    if "%mode%" == "release" (
        set "modeFlags=-O3"
    )

    set "win32Libraries=-Wl,User32.lib,Gdi32.lib,Winmm.lib"
    set "gameExportedFunctions=-Wl,-EXPORT:UpdateAndRender,-EXPORT:GetSoundSamples"
)

set "sanitizeFlags="
if "%useAsan%" == "1" (
    rem TODO: sanitize=undefined?
    set "sanitizeFlags=-fsanitize=address"
)

set "modeFlags=%modeFlags% %sanitizeFlags%"

set "commonFlags=%commonDefines% %modeFlags% %commonFlags% %commonWarnings%"

echo %commonFlags%
echo.

rem delete all .pdb files
rem replace the game's one with a new timestamped version to enable instantenous updating
del *.pdb >nul 2>nul

rem wait for pdb to be generated before building platform
rem sometimes we couldn't set breakpoints in visual studio
rem as the pdb was not loaded correctly when hot loading
echo WAITING FOR PDB > lock.tmp

rem compile the platform and the game as seperate to allow DLL tricks
rem insert a random number to avoid name conflict when rebuilding

set "cTimeNameGame=win32_handmade_msvc.ctm"
set "cTimeNamePlatform=win32_platform_msvc.ctm"
if %useCTime% == 1 (
    if "%mode%" == "release" (
        set "cTimeNameGame=win32_handmade_msvc_rel.ctm"
        set "cTimeNamePlatform=win32_platform_msvc_rel.ctm"
    )

    if "%compiler%" == "clang" (
        if "%mode%" == "debug" (
            set "cTimeNameGame=win32_handmade_clang.ctm"
            set "cTimeNamePlatform=win32_platform_clang.ctm"
        ) else (
            set "cTimeNameGame=win32_handmade_clang_rel.ctm"
            set "cTimeNamePlatform=win32_platform_clang_rel.ctm"
        )
    )

    rem forced delayed expansion...
    ctime.exe -begin "!cTimeNameGame!"
)

set buildFailed=0

if "%compiler%" == "clang" (
    echo handmade.cpp
)

%cxx% %commonFlags% ../src/game/handmade.cpp -I ../src %outDll% %dllFlags% %linkerFlags% %gameExportedFunctions%
if ERRORLEVEL 1 (
    set buildFailed=1
    echo [31m[1mhandmade.cpp failed[0m[1m
)

if %useCTime% == 1 (
    ctime.exe -end "%cTimeNameGame%" %buildFailed%
)

set buildFailed=0

del lock.tmp

if %useCTime% == 1 (
    ctime.exe -begin "%cTimeNamePlatform%"
)

if "%compiler%" == "clang" (
    echo win32_handmade.cpp
)

%cxx% %commonFlags% ../src/platform/win32/win32_handmade.cpp -I ../src %outExe% %linkerFlags% %win32Libraries%
if ERRORLEVEL 1 (
    set buildFailed=1
    echo [31m[1mwin32_handmade.cpp failed[0m[1m
)

if %useCTime% == 1 (
    ctime.exe -end "%cTimeNamePlatform%" %buildFailed%
)

rem needed if building from command line and not vscode
popd

set NOW=%TIME:~0,8%

echo.
rem print out build status with colors just for fun :)
rem doesn't work for every MSVC error... maybe?
if %buildFailed% NEQ 0 (
    rem ANSI escape sequences
    rem https://gist.githubusercontent.com/mlocati/fdabcaeb8071d5c75a2d51712db24011/raw/b710612d6320df7e146508094e84b92b34c77d48/win10colors.cmd
    echo [31m[1mBuild failed[0m[1m %DATE% %NOW%
) else (
    echo [32m[1mBuild succeeded[0m[1m %DATE% %NOW%
)
