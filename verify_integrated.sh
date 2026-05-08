#!/bin/bash
set -e

echo "=== Integrated Verification Script ==="

echo "1. Building Verification Tools..."
cd verification_tool && mkdir -p build && cd build
cmake .. > /dev/null && make > /dev/null
echo "   [OK] Verification tools built."

echo "2. Building WebRTC Streamer & Signaling Server..."
cd ../../webrtc_streamer && mkdir -p build && cd build
cmake .. > /dev/null && make > /dev/null
echo "   [OK] WebRTC components built."

echo "3. Running Automated Health Check..."
cd ../..
./verify.sh

echo "4. Checking Library Dependencies (Container Simulation)..."
ldd ./verification_tool/build/verify_camera | grep "not found" && (echo "   [FAIL] Missing libraries found!"; exit 1) || echo "   [OK] Library dependencies resolved."

echo ""
echo "=== Automated Tests Passed ==="
echo "Please proceed to manual GUI tests as outlined in TEST_PLAN.md:"
echo " - view_sbs (D2C Alignment)"
echo " - WebRTC browser stream"
echo " - Container X11 forwarding"
