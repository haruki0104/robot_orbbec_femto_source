#!/bin/bash

# Ensure build directory exists
mkdir -p verification_tool/build
cd verification_tool/build

# Build the tool if not already built
if [ ! -f ./verify_camera ]; then
    cmake ..
    make
fi

# Run the verification
./verify_camera
