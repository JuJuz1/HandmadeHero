@echo off

rem setup env for x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

IF NOT EXIST build mkdir build
pushd build
rem We are now inside build

rem /TP /EHsc- /GR- /O2 /Ob2 /Fe:test.exe
rem /TP compile all files as c++
rem /EHa- disable exception handling for asynchronous and synchronous
rem /GR- disable RTTI
rem /Bt better output info

rem https://learn.microsoft.com/en-us/cpp/build/reference/o-options-optimize-code?view=msvc-170
rem /Oi generate intrinsic functions for appropriate function calls

rem /wd4201 nonstandard extension used: nameless struct/union
rem /wd4127 conditional expression is constant
rem TODO: this is raised from ASSERT(false...) -> fix ASSERT

rem /nologo don't show compiler version and info

rem link the User32.lib, Gdi32.lib to create UI
cl -DHANDMADE_WIN32=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1 /Zi /FC /W4 /wd4201 /wd4127 /Oi /EHa- /GR- /std:c++20 /nologo /I ../src ../src/win32/win32_handmade.cpp User32.lib Gdi32.lib

rem not actually needed, to pop the build directory?
rem needed if building from command line and not vscode
popd

set NOW=%TIME:~0,8%

echo.
rem print out build status with colors just for fun :)
if %errorlevel% NEQ 0 (
    rem ANSI escape sequences
    rem https://gist.githubusercontent.com/mlocati/fdabcaeb8071d5c75a2d51712db24011/raw/b710612d6320df7e146508094e84b92b34c77d48/win10colors.cmd
    echo [31m[1mBuild failed[0m[1m %DATE% %NOW%
) else (
    echo [32m[1mBuild succeeded[0m[1m %DATE% %NOW%
)

echo.
