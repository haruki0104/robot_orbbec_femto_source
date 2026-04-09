# Orbbec Femto Mega SDK & Verification Project

This project provides the Orbbec SDK v2 (`libobsensor`) and a dedicated verification tool for Orbbec Femto Mega cameras. It is designed to automate camera hardware validation, ensuring that both depth and color streams are functional.

## Project Overview

- **Orbbec SDK v2**: Located in `OrbbecSDK/`, it includes the core libraries, headers, and official examples for interacting with Orbbec devices.
- **Verification Tool**: Located in `verification_tool/`, this C++ application performs automated checks on connected cameras, reporting an `OK` or `NG` (No Good) status based on frame acquisition.
- **Automation Scripts**: Root-level and SDK-level scripts facilitate environment setup, example building, and rapid camera verification.

### Main Technologies
- **Language**: C++11
- **Build System**: CMake (3.10+)
- **SDK**: Orbbec SDK v2 (`libobsensor`)
- **Dependencies**: OpenCV (optional, used for `view_camera` and certain SDK examples), `libusb-1.0`, `build-essential`.

---

## Building and Running

### Prerequisites
Ensure you have the necessary build tools and dependencies installed. On Ubuntu/Debian, you can use:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libopencv-dev
```

### Initial Setup
To install udev rules (required for USB device access without root) and prepare the SDK environment:
```bash
./OrbbecSDK/setup.sh
```

### Camera Verification
The primary way to verify connected hardware:
```bash
./verify.sh
```
This script builds the `verification_tool` (if needed) and runs it automatically.

### Building SDK Examples
To build all provided Orbbec SDK examples:
```bash
./OrbbecSDK/build_examples.sh
```
Executable binaries will be located in `OrbbecSDK/bin/`.

### Manual Verification Tool Build
```bash
cd verification_tool
mkdir -p build && cd build
cmake ..
make
```

---

## Development Conventions

### Coding Style
- **C++ Standard**: C++11 or higher.
- **Error Handling**: Use `try-catch` blocks with `ob::Error` for SDK-related operations to ensure robustness.
- **Resource Management**: Prefer `std::shared_ptr` (as used by the SDK's C++ wrapper) for managing SDK objects like `ob::Context`, `ob::Device`, and `ob::Pipeline`.
- **Formatting**: Adhere to the existing 4-space indentation and standard C++ naming conventions (camelCase for variables/functions, PascalCase for classes).

### Testing & Validation
- **Hardware Simulation**: If no physical camera is connected, `verify_camera` will report "No device found!".
- **Frame Validation**: The verification tool validates streams by counting received frames over a configurable timeout (default 5 seconds, 5 frames minimum).
- **Return Codes**: 
  - `0`: All devices passed.
  - `2`: One or more devices failed (`NG`).
  - `-1`: SDK or connection error.

### Project Structure Key Paths
- `OrbbecSDK/include/libobsensor`: Core C++ headers (`ObSensor.hpp`).
- `OrbbecSDK/lib`: Shared libraries for linking.
- `verification_tool/verify_camera.cpp`: Implementation of the OK/NG logic.
- `verification_tool/view_camera.cpp`: Simple OpenCV-based stream viewer.
