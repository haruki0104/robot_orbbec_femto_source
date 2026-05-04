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

## Developer Guide

### Project Architecture
The project is divided into three main components, all sharing the same **OrbbecSDK v2** core:
1.  **Verification Tool**: Lightweight C++ tools for hardware validation.
2.  **ROS 2 Bridge**: Integration with ROS 2 (Jazzy/Humble) for robotics applications.
3.  **WebRTC Streamer**: A high-performance, bi-directional streaming solution for WebVR and remote monitoring.

### Building the Project
The build system is designed to work both inside a container (target path `/workspace`) and in local development environments. It automatically detects the OrbbecSDK location.

#### Building All Components
```bash
# Verification Tool
cd verification_tool && mkdir -p build && cd build && cmake .. && make

# WebRTC Streamer
cd webrtc_streamer && mkdir -p build && cd build && cmake .. && make

# ROS 2 Bridge
cd ros2_bridge && colcon build
```

### WebRTC Remote Control System
The WebRTC streamer uses a **Data Channel** named `control` to receive hardware commands from the browser.

#### Communication Protocol
Messages are sent as simple JSON strings from the browser:
*   `{"type": "laser", "value": true/false}`: Toggles the IR laser.
*   `{"type": "viz_mode", "value": 0/1/2}`: Switches between SBS (3D), Color, and Depth views.
*   `{"type": "exposure", "value": <int>}`: Manually sets color exposure.

#### Extending Remote Control
To add a new control:
1.  **C++**: Add a case in `WebRtcSbsStreamer::handleControlMessage` in `webrtc_streamer/src/main.cpp`. Use `device->setIntProperty` or `device->setBoolProperty` with a property ID from `OrbbecSDK/include/libobsensor/h/Property.h`.
2.  **Web**: Add a UI element in `webrtc_streamer/web/index.html` that calls `sendControl('your_property', value)`.

### Visualization Compositor
The `SbsCompositor` class (`webrtc_streamer/src/sbs_compositor.hpp`) handles the layout of frames. It supports:
*   **Depth Heatmaps**: Toggled via `useColormap`.
*   **Text Overlays**: Configurable via `showOverlay`.
*   **Mode Switching**: Dynamic layout changes without re-initializing the video encoder.

