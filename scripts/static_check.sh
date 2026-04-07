#!/usr/bin/env bash
set -euo pipefail

# MUST BE RUN FROM THE ROOT

SRC="src"

# This script searches for the handmade defined keywords for static
# LOCAL_PERSIST, INTERNAL and GLOBAL

printf "\nStatics found:\n"
grep -nHriw "static" "$SRC"
# We should only see the #define macros here!

printf "\nLOCAL_PERSIST:\n"
grep -nHr "LOCAL_PERSIST" "$SRC"

# Not really concerned about these for now (probably ever as we use unity builds)
#printf "\nINTERNAL:\n"
#grep -nHr "INTERNAL" "$SRC"

printf "\nGLOBAL:\n"
grep -nHr "GLOBAL" "$SRC"
