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

# Function to check if a specific port is in use
check_port_in_use() {
    PORT=$1
    PROCESS_INFO=$(sudo lsof -i :"$PORT" -sTCP:LISTEN)

    if [[ -n "$PROCESS_INFO" ]]; then
        echo "Port $PORT is being used by the following process:"
        echo "$PROCESS_INFO"
        return 0 # Port is in use
    else
        echo "Port $PORT is not in use."
        return 1 # Port is not in use
    fi
}

# Check ports 8080 and 8081
check_port_in_use 8080
IN_USE_8080=$?

check_port_in_use 8081
IN_USE_8081=$?

# If either port is in use, do not run the api-server
if [[ $IN_USE_8080 -eq 0 || $IN_USE_8081 -eq 0 ]]; then
    echo "One or more ports are already in use. The api-server will not be started."
    exit 1
fi

# Run the api-server
echo "Starting api-server..."
./build/api-server