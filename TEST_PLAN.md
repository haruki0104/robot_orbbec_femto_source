# Integrated Test Plan for Orbbec Femto Mega SDK

This document outlines the mandatory test suite to be executed before merging any feature into the `main` branch. It ensures that core SDK functionality, verification tools, WebRTC streaming, and containerized environments remain stable.

## 1. Environment & Build Verification
**Goal**: Ensure the project builds from scratch on a clean environment.

- [ ] **Clean Build**: 
  ```bash
  rm -rf build/ verification_tool/build/ webrtc_streamer/build/
  ./OrbbecSDK/build_examples.sh
  cd verification_tool && mkdir build && cd build && cmake .. && make
  cd ../../webrtc_streamer && mkdir build && cd build && cmake .. && make
  ```
- [ ] **Dependency Check**: Verify all required libraries (`libobsensor`, `OpenCV`, `libdatachannel`, `FFmpeg`) are correctly linked.

## 2. Hardware & Basic Verification
**Goal**: Confirm the camera is detected and producing valid frames.

- [ ] **Device Detection**: Run `./bin/enumerate` (from SDK examples) to confirm the Femto Mega is listed.
- [ ] **Automated Health Check**: Run `./verify.sh`.
  - **Success Criteria**: Output shows `OK` for all connected devices.
- [ ] **Direct Stream Check**: Run `./verification_tool/build/view_camera`.
  - **Success Criteria**: Color and Depth windows open and show live data.

## 3. 3D Side-By-Side (SBS) & D2C Alignment
**Goal**: Verify that hardware alignment (Depth-to-Color) is functioning correctly.

- [ ] **SBS Preview**: Run `./verification_tool/build/view_sbs`.
  - **Success Criteria**: A single window displays Color (left) and Colorized Depth (right). Features (like edges of objects) should align perfectly between the two halves.

## 4. WebRTC Streaming & Automated Signaling
**Goal**: Verify the end-to-end streaming pipeline.

- [ ] **Signaling Handshake**:
  1. Start `./webrtc_streamer/build/signaling_server`.
  2. Start `./webrtc_streamer/build/webrtc_streamer`.
  3. Open `http://localhost:8000` in a browser.
  - **Success Criteria**: Browser log shows `Connected to signaling server` and `Received SDP Offer`. The stream starts automatically.
- [ ] **Remote Control**: Toggle the "Laser" and "Auto Exposure" buttons in the web UI.
  - **Success Criteria**: Streamer terminal logs the property changes.
- [ ] **Dummy Mode (No Hardware)**: Run the streamer without a camera.
  - **Success Criteria**: Streamer enters "DUMMY MODE" and browser displays the moving test pattern.

## 5. Containerized Environment Validation
**Goal**: Ensure Docker integration remains functional.

- [ ] **Container Build**: `docker compose build`.
- [ ] **X11 Forwarding**: 
  1. Run `xhost +local:docker` on host.
  2. Run `docker exec -it orbbec_streamer_container /app/bin/view_camera`.
  - **Success Criteria**: The GUI window appears on the host desktop.
- [ ] **Library Integrity**: Run `ldd /app/bin/webrtc_streamer` inside the container to ensure no "not found" libraries.

## 6. Regression Testing
- [ ] **ROS 2 Integration** (if applicable): Verify `ros2_bridge` compiles and launches.
- [ ] **SDK Examples**: Run `0.basic.quick_start` to ensure core SDK functionality is intact.

---
**Approval**: A feature branch is ready for merge only when all relevant items above are checked.
