@echo off

rem mac is not available on act
act push ^
  -P windows-2025-vs2026=-self-hosted ^
  -P ubuntu-latest=catthehacker/ubuntu:act-latest
