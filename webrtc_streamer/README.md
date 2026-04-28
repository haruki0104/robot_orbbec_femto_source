# WebRTC SBS Streamer for Femto Mega

This subproject provides a real-time WebRTC streaming solution for the Orbbec Femto Mega camera, specifically designed for 3D visualization in VR/AR headsets.

## Features

- **SBS (Side-By-Side) Composition**: Merges synchronized Color and Depth frames into a single wide frame.
- **Hardware D2C Alignment**: Aligns depth data to the color image using the camera's hardware-accelerated transformation.
- **WebRTC Streaming**: Low-latency video streaming using `libdatachannel`.
- **H.264 Encoding**: Real-time video compression via FFmpeg (libavcodec).
- **WebVR Client**: A browser-based viewer using Three.js and GLSL shaders to reconstruct 3D geometry from the SBS stream.

## Dependencies

Install the following system libraries:
```bash
sudo apt-get update
sudo apt-get install libdatachannel-dev libavcodec-dev libavutil-dev libswscale-dev libopencv-dev
```

## Build Instructions

```bash
mkdir build && cd build
cmake ..
make
```

## How to Use

1. **Start the Streamer**:
   ```bash
   ./webrtc_streamer
   ```
2. **Copy the SDP Offer**: The terminal will output a block of text (Offer). Copy it.
3. **Open the Client**:
   - Serve the `web/` directory (e.g., `python3 -m http.server 8000`).
   - Open `http://localhost:8000` in a browser.
4. **Signaling**:
   - Paste the Offer into the browser.
   - Click "Connect".
   - Copy the "Answer" from the browser back to the terminal and press Enter twice.
5. **View in VR**: Once connected, click "Enter VR" (requires a VR-compatible browser or WebXR emulator).

## Structure

- `src/main.cpp`: Main application loop and WebRTC signaling.
- `src/sbs_compositor.hpp`: Logic for frame merging and depth normalization.
- `src/h264_encoder.hpp`: FFmpeg-based H.264 video encoding.
- `web/index.html`: WebVR receiver client.

## Docker Deployment

### Accessing Binaries Outside Container
To copy the pre-built binaries from the Docker image to your host machine's `bin/` folder, run:
```bash
docker compose run --rm webrtc-streamer cp -r /app/bin /workspace/bin/
```
The binaries (`webrtc_streamer`, `verify_camera`, etc.) will then be available in your local `./bin/` directory.

## TODO

- [ ] **Automated Signaling**: Replace the manual SDP copy-paste process with a WebSocket-based signaling server (e.g., using a simple Python/FastAPI or Node.js relay) for a seamless "plug-and-play" connection experience.
