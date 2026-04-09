# Orbbec Femto/Mega Verification Tool

This project provides a simple, robust camera verification app for Orbbec devices using the `OrbbecSDK` (`libobsensor`).

## Objective

- Detect connected Orbbec cameras
- For each device, start depth + color streams
- Collect frame counts and validate operation
- Report per-device and overall status (OK/NG)

## Features

- Configurable timeout (`--timeout`)
- Configurable minimum frame count (`--min-frames`)
- Optional single-device check (`--device`)
- Verbose output mode (`--verbose`)

## Build

```sh
mkdir -p build && cd build
cmake ..
make
```

## Run

```sh
./verify_camera
./verify_camera --timeout 8 --min-frames 10
./verify_camera --device 0 --verbose
```

## Return codes

- `0` - all selected devices passed
- `2` - one or more devices failed
- `-1` - no device found or SDK error
- `1` - invalid command-line arguments

## Notes

- Ensure `OrbbecSDK` is available in `../OrbbecSDK/lib` (as configured by `CMakeLists.txt`).
- This utility is intended for production hardware validation and automated test traces.
