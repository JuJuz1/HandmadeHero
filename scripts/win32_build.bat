@echo off

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
set commonCompilerWarnings=/W4 /wd4201 /wd4505 /wd4100 /wd4189
set commonCompilerDefines=-DHANDMADE_WIN32=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1 -DHANDMADE_USE_REAL_ASSETS=1
set commonCompilerFlags=%commonCompilerDefines% /MTd /Zi /Zc:__cplusplus /FC /Fm /Od /Oi /EHa- /GR- /std:c++20 /nologo %commonCompilerWarnings%
set commonLinkerFlags=/OPT:REF /OPT:NOICF /INCREMENTAL:NO

set win32Libraries=User32.lib Gdi32.lib Winmm.lib
set gameExportedFunctions=/EXPORT:UpdateAndRender /EXPORT:GetSoundSamples

set buildFailed=0

rem delete all .pdb files
rem replace the game's one with a new timestamped version to enable instantenous updating
del *.pdb >nul 2>nul

rem wait for pdb to be generated before building platform
rem sometimes we couldn't set breakpoints in visual studio
rem as the pdb was not loaded correctly when hot loading
echo WAITING FOR PDB > lock.tmp

rem compile the platform and the game as seperate to allow DLL tricks
rem insert a random number to avoid name conflict when rebuilding
cl %commonCompilerFlags% ../src/game/handmade.cpp /I ../src /LD /link /PDB:handmade_%random%.pdb %gameExportedFunctions% %commonLinkerFlags%
if ERRORLEVEL 1 (
    set buildFailed=1
    echo [31m[1mhandmade.cpp failed[0m[1m
)

del lock.tmp

cl %commonCompilerFlags% ../src/platform/win32/win32_handmade.cpp /I ../src /link %win32Libraries% %commonLinkerFlags%
if ERRORLEVEL 1 (
    set buildFailed=1
    echo [31m[1mwin32_handmade.cpp failed[0m[1m
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
