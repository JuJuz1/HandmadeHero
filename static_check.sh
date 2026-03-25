#!/usr/bin/env bash
set -euo pipefail

# This script searches for the handmade defined keywords for static
# LOCAL_PERSIST, INTERNAL and GLOBAL

printf "\nStatics found:\n"
grep -nHriw "static" src
# We should only see the #define macros here!

printf "\nLOCAL_PERSIST:\n"
grep -nHr "LOCAL_PERSIST" src

# Not really concerned about these for now (probably ever as we use unity builds)
#printf "\nINTERNAL:\n"
#grep -nHr "INTERNAL" src

printf "\nGLOBAL:\n"
grep -nHr "GLOBAL" src
