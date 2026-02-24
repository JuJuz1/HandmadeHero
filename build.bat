@echo off

rem setup env for x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir build
pushd build

rem /TP /EHsc /O2 /Ob2 /Fe:test.exe
rem link the User32.lib, Gdi32.lib to create UI
cl /Zi /FC /W4 /Bt /std:c++20 ../src/handmade.cpp User32.lib Gdi32.lib

rem not actually needed?
rem popd

set NOW=%TIME:~0,8%

rem print out build status with colors just for fun :)
if %errorlevel% NEQ 0 (
    rem ANSI escape sequences
    rem https://gist.githubusercontent.com/mlocati/fdabcaeb8071d5c75a2d51712db24011/raw/b710612d6320df7e146508094e84b92b34c77d48/win10colors.cmd
    echo [31mBuild failed[0m at %NOW%
) else (
    echo [32mBuild succeeded[0m at %NOW%
)
