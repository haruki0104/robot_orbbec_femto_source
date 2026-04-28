# Orbbec Femto Mega SDK & Verification Project

This project provides the Orbbec SDK v2 and a suite of tools for verifying, streaming, and integrating Orbbec Femto Mega cameras into various environments, including ROS 2 and WebRTC.

## Core Components

- **Orbbec SDK v2**: Core libraries and headers (`OrbbecSDK/`).
- **Verification Tool**: C++ utility to perform automated OK/NG hardware validation (`verification_tool/`).
- **ROS 2 Bridge**: High-performance ROS 2 node for Color, Depth, IR, IMU, and Point Cloud streaming (`ros2_bridge/`).
- **WebRTC Streamer**: Low-latency streaming solution for WebVR and browser-based 3D visualization (`webrtc_streamer/`).

## Getting Started

### 1. Hardware Verification
Connect your camera and run the automated check:
```bash
./verify.sh
```

### 2. ROS 2 Integration (Jazzy/Humble)
Build and launch the Femto Mega node to publish topics to your ROS 2 environment:
```bash
cd ros2_bridge
colcon build --packages-select femto_mega_bridge
source install/setup.bash
ros2 launch femto_mega_bridge femto_mega.launch.py
```
**Supported Topics:**
- `/camera/color/image_raw` (RGB)
- `/camera/depth/image_raw` (16-bit, D2C Aligned)
- `/camera/ir/image_raw` (Infrared)
- `/camera/imu/data_raw` (Accel + Gyro)
- `/camera/depth/points` (RGB PointCloud2)

### 3. WebRTC Streaming
To stream camera data to a web-based VR or 3D viewer:
```bash
cd webrtc_streamer
# Refer to webrtc_streamer/README.md for setup instructions
```

## Project Structure
- `OrbbecSDK/`: The Orbbec SDK v2 files.
- `verification_tool/`: Source code for the OK/NG verification tool.
- `webrtc_streamer/`: WebRTC streaming logic and web assets.
- `ros2_bridge/`: ROS 2 package for camera integration.
- `verify.sh`: Convenience script to run the hardware validation.

## License
Apache-2.0
