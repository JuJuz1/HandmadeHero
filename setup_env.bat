@echo off

rem setup env for x64, currently done in build.bat, do before build_cli.bat
rem couldn't get to work in VSCode so that it would not spawn a new shell every time
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
