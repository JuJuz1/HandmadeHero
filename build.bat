@echo off

rem setup env for x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir build
pushd build

rem /TP /EHsc /O2 /Ob2 /Fe:test.exe
rem link the User32.lib to create UI
cl /Zi /W4 ../src/handmade.cpp User32.lib

rem not actually needed?
rem popd
