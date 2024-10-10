#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Get the current working directory
CURRENT_DIR="$(pwd)"

# Check if the script is being run one directory above its location
if [[ "$CURRENT_DIR" != "$(dirname "$SCRIPT_DIR")" ]]; then
    echo "Please run this script from the repo's root directory."
    exit 1
fi

mkdir -p build
cd build
cmake ..
make -j$(nproc)