#!/usr/bin/env bash
set -euo pipefail

# This script can be used to run clang-tidy on the unity build files
# It constructs the compile_commands.json automatically which clang-tidy uses
# No absolute paths have to be set in the compile_commands.json manually
# This script has to be run from the project root

# Get absolute path of the root directory (Linux (and macOS?))
ROOT="$(pwd)"

# Add files here (platform files in addition to the game)
SRC_FILES=(
    "src/game/handmade.cpp"
    "src/platform/linux/linux_handmade.cpp"
)

COMPILER_COMMAND="clang++ -DHANDMADE_LINUX=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_DEBUG=1 -I src -Wall -Wextra -Wpedantic -std=c++20"

# Start building the JSON, overwriting if already existed
echo "[" > "$ROOT/compile_commands.json"

for i in "${!SRC_FILES[@]}"; do
    FILE="${SRC_FILES[i]}"

    COMMA=","
    if [[ $i -eq $((${#SRC_FILES[@]} - 1)) ]]; then
        COMMA=""
    fi

    # Append the text between EOFs
    cat >> "$ROOT/compile_commands.json" <<EOF
    {
        "directory": "$ROOT",
        "command": "$COMPILER_COMMAND $FILE",
        "file": "$FILE"
    }$COMMA
EOF

done

echo "]" >> "$ROOT/compile_commands.json"

set -x

clang-tidy -p . "${SRC_FILES[@]}"
