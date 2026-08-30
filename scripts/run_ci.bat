@echo off
setlocal enabledelayedexpansion

rem mac is not available on act
if "%~1"=="win32" (
  act push ^
      -j build-win32 ^
      -P windows-2025-vs2026=-self-hosted
  exit /b %ERRORLEVEL%
)

if "%~1"=="linux" (
  act push ^
    -j build-linux ^
    -P ubuntu-latest=catthehacker/ubuntu:act-latest
  exit /b %ERRORLEVEL%
)

if "%~1"=="web" (
  act push ^
    -j build-web ^
    -P ubuntu-latest=catthehacker/ubuntu:act-latest
  exit /b %ERRORLEVEL%
)

if "%~1"=="" (
  act push ^
    -P windows-2025-vs2026=-self-hosted ^
    -P ubuntu-latest=catthehacker/ubuntu:act-latest
  exit /b %ERRORLEVEL%
)

echo Usage: %~nx0 [win32^|linux^|web]
exit /b 1
