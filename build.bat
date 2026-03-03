@echo off

rem setup env for x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

IF NOT EXIST build mkdir build
pushd build
rem We are now inside build

rem /TP /EHsc- /GR- /O2 /Ob2 /Fe:test.exe
rem /TP compile all files as c++
rem /EHsc- disable exception handling for c++ and extern "C"
rem /GR- disable RTTI
rem /Bt better output info

rem link the User32.lib, Gdi32.lib to create UI
cl -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1 /Zi /FC /W4 /std:c++20 /I ../src ../src/win32/win32_handmade.cpp User32.lib Gdi32.lib

rem not actually needed, to pop the build directory?
rem popd

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
