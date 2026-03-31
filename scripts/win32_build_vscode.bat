@echo off

rem this can be used to build the project using a shortcut in vscode
rem seems to only work on vscode which is fine

call "scripts\win32_setup_2022.bat"
call "scripts\win32_build.bat"
