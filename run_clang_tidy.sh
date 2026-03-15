#!/usr/bin/env bash
set -euxo pipefail

clang-tidy src/handmade.cpp src/win32/win32_handmade.cpp
