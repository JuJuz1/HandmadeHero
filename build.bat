@echo off

mkdir build
pushd build

rem /TP /EHsc /O2 /Ob2 /Fe:test.exe
rem link the User32.lib to create a message box
cl /Zi ../src/handmade.cpp User32.lib

rem not actually needed?
rem popd
