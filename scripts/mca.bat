@echo off
setlocal enabledelayedexpansion

IF NOT EXIST build mkdir build
pushd build

rem LLVM MCA
rem https://llvm.org/docs/CommandGuide/llvm-mca.html
rem specify the argument "all" to get a lots of additional statistics
rem This script assumes you have llvm-mca.exe binary inside .\misc

set "allStats=0"

for %%a in (%*) do set "%%~a=1"
if "%all%" == "1" set "allStats=1"

set "commonDefines=-DHANDMADE_WIN32=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_MCA=1"
set "commonFlags=-fno-exceptions -fno-rtti -std=c++20"
set "commonWarnings=-Wall -Wextra -Wpedantic -Wno-unused-function -Wno-missing-braces -Wno-unused-variable -Wno-unused-parameter -Wno-null-dereference -Wno-missing-field-initializers -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-sign-compare -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-but-set-variable -Wno-unused-value"
rem Compile with optimizations on of course
set "modeFlags=-O2"

set "commonFlags=%commonDefines% %modeFlags% %commonFlags% %commonWarnings%"

rem output as assembly and supply that to llvm-mca
clang++ ..\src\game\handmade.cpp -I ..\src %commonFlags% -S -o handmade.s

rem TODO: Figure out why clang can optimize the loop much better!
rem This works and produces the asm file!
rem set "modeFlags=-Zc:__cplusplus -FC -Oi -EHa- -GR- -nologo -std:c++20 -MT -O2"
rem set "commonFlags=%commonDefines% %modeFlags%"
rem cl ..\src\game\handmade.cpp -I ..\src %commonFlags% -c -FAs -Fahandmade_msvc.asm

rem Here one can specify the cpu arch if not autodetected
set "cpu=znver4"
set "options="

if "%allStats%" == "1" (
    set "options=-bottleneck-analysis -timeline -timeline-max-iterations=2 -all-stats"
)

..\misc\llvm-mca -mcpu=%cpu% %options% handmade.s -o mca.txt

popd
