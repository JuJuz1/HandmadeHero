#!/usr/bin/env bash
set -euo pipefail

# Hosts the web build to be playable locally
# Without this we run into CORS issues and such bs...

cd build/web
python -m http.server 8000
