# Orbbec Femto Mega Verification Project

This project helps you verify if your Orbbec Femto Mega cameras are working correctly (OK) or not (NG).

## Prerequisites

- Orbbec SDK v2 (already included in the `OrbbecSDK` folder)
- CMake 3.10+
- G++ compiler
- OpenCV (optional, for some SDK examples)

## How to Verify Cameras

1.  **Connect your camera(s)** via USB or Network.
2.  **Run the verification script**:
    ```bash
    ./verify.sh
    ```
    This will:
    - Enumerate all connected Orbbec devices.
    - Attempt to start Depth and Color streams for each device.
    - Receive 5-10 frames to confirm functionality.
    - Report `OK` or `NG` for each device.

## Orbbec Viewer

For a visual check of the camera streams, you can use the official Orbbec Viewer:
```bash
cd OrbbecSDK/bin
./OrbbecViewer
```

## Building the Tool

If you modify `verification_tool/verify_camera.cpp`, you can rebuild it using:
```bash
cd verification_tool
mkdir -p build
cd build
cmake ..
make
```

## Project Structure

- `OrbbecSDK/`: The Orbbec SDK v2 files.
- `verification_tool/`: Source code for the OK/NG verification tool.
- `webrtc_streamer/`: Real-time WebRTC streaming for WebVR/3D visualization.
- `verify.sh`: Convenience script to run the tool.
