# AGENTS.md

## Project structure

Four components sharing `OrbbecSDK/` (`libobsensor` v2.7.6, pre-built under `OrbbecSDK/lib/`):

| Component | Language | Build system | Entry point |
|-----------|----------|-------------|-------------|
| `verification_tool/` | C++11 | CMake 3.10+ | `verify_camera.cpp`, `view_camera.cpp`, `view_sbs.cpp` |
| `webrtc_streamer/` | C++17 | CMake 3.10+ | `src/main.cpp` (+ `signaling_server.cpp`) |
| `ros2_bridge/` | C++17 | colcon (ament_cmake) | `src/femto_mega_node.cpp` |
| `OrbbecSDK/` | C (SDK) | CMake | `include/libobsensor/ObSensor.hpp` |

GTest-based unit tests in `tests/` (C++17, uses system-installed GoogleTest 1.14.0).
No typechecker, no CI workflows exist in this repo. Static analysis via `cppcheck` and `clang-tidy`.

## Setup

```bash
# Install udev rules for USB camera access (requires sudo)
./OrbbecSDK/setup.sh

# Or manually:
sudo OrbbecSDK/shared/install_udev_rules.sh
```

Dependencies: `build-essential cmake libopencv-dev libusb-1.0-0-dev libavcodec-dev libavutil-dev libswscale-dev libdatachannel-dev`

## Build

**Verification tool** (C++11):
```bash
cd verification_tool && mkdir -p build && cd build
cmake .. && make
```

**WebRTC streamer** (C++17, needs libdatachannel + FFmpeg):
```bash
cd webrtc_streamer && mkdir -p build && cd build
cmake .. && make
```
Outputs: `webrtc_streamer` + `signaling_server`.

**ROS 2 bridge** (requires ROS 2 Humble/Jazzy sourced):
```bash
cd ros2_bridge && colcon build --packages-select femto_mega_bridge
source install/setup.bash
```

**OrbbecSDK examples** (optional):
```bash
./OrbbecSDK/build_examples.sh
# Output in OrbbecSDK/bin/
```

## Verification flow

`TEST_PLAN.md` covers the full manual test suite. The only automated check is `./verify.sh` (frame counting over timeout). For integration confidence: `verify_integrated.sh` builds everything and runs the library dependency check.

## Tests

```bash
# Build and run all tests
mkdir -p tests/build && cd tests/build
cmake .. && make
ctest --output-on-failure

# Or run individual test binaries directly:
./test_argparse
./test_sbs_compositor
./test_h264_encoder
```

Covers: argument parsing, SBS compositor (synthetic OpenCV frames), H.264 encoder (synthetic frames via FFmpeg). Tests use system-installed GoogleTest (`find_package(GTest)`). Run `cd tests/build && ctest` as a focused verification step.

## Static analysis

```bash
# cppcheck on verification_tool (C++11)
cppcheck --enable=all --std=c++11 --suppress=missingIncludeSystem \
  -I verification_tool -I OrbbecSDK/include verification_tool/*.cpp verification_tool/*.hpp

# cppcheck on webrtc_streamer (C++17)
cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem \
  -I webrtc_streamer/src -I OrbbecSDK/include webrtc_streamer/src/*.cpp webrtc_streamer/src/*.hpp

# clang-tidy (per file, needs include paths)
clang-tidy <file> -- <compiler flags>
```

## OrbbecSDK path resolution

All CMakeLists.txt check `/workspace/OrbbecSDK/lib` first (Docker path), then fall back to `../OrbbecSDK/lib` (local dev). No need to set `LD_LIBRARY_PATH` or `CMAKE_PREFIX_PATH`.

## Docker

```bash
docker compose build
docker compose run --rm webrtc-streamer
# Binaries available at /app/bin inside container
docker compose run --rm webrtc-streamer cp -r /app/bin /workspace/bin/

# View camera from container:
xhost +local:docker && docker exec -ti orbbec_streamer_container /app/bin/view_camera
```

Container runs privileged + host networking + USB passthrough (required for camera + WebRTC).

## Verification tool usage

```bash
./verify_camera                    # default: 5s timeout, min 5 frames
./verify_camera --timeout 8 --min-frames 10
./verify_camera --device 0 --verbose

# Return codes: 0=OK, 2=NG (device failed), -1=no device/SDK error, 1=invalid args
```

## WebRTC streaming (automated signaling)

1. `./signaling_server` (WebSocket relay, port 8889)
2. `./webrtc_streamer` (connects to signaling server, sends SDP offer)
3. Open browser to web client (served from `webrtc_streamer/web/`)
4. Browser auto-connects via signaling server, no manual SDP copy-paste

Dummy mode (no camera): streamer logs "DUMMY MODE", sends synthetic test pattern.

## ROS 2 bridge

Supports both USB and Ethernet cameras:
```bash
# USB camera - auto-detects
ros2 launch femto_mega_bridge femto_mega.launch.py

# Ethernet camera
ros2 launch femto_mega_bridge femto_mega.launch.py camera_ip:=192.168.1.100
```

Published topics: `/camera/color/image_raw`, `/camera/depth/image_raw`, `/camera/ir/image_raw`, `/camera/imu/data_raw`, `/camera/depth/points`

D2C (Depth-to-Color) alignment is hardware mode (`ALIGN_D2C_HW_MODE`), enabled by default in both bridge and streamer.

## Conventions

- C++11 for `verification_tool`, C++17 for `webrtc_streamer` (libdatachannel dependency)
- 4-space indentation, camelCase variables, PascalCase classes
- Orbbec SDK errors caught via `ob::Error` exceptions
- SDK objects managed via `std::shared_ptr`
- Depth values in mm, point cloud in meters (ROS bridge divides by 1000)
- `bin/` and `build/` are gitignored
