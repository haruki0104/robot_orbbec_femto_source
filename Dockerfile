# --- Stage 1: Builder ---
FROM ubuntu:24.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    wget \
    libusb-1.0-0-dev \
    libssl-dev \
    libudev-dev \
    libopencv-dev \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    && rm -rf /var/lib/apt/lists/*

# Configurable Orbbec SDK version
ARG ORBBEC_SDK_VERSION=v2.7.6

# Build and install Orbbec SDK v2 from source to /workspace/OrbbecSDK
RUN git clone https://github.com/orbbec/OrbbecSDK_v2.git /tmp/OrbbecSDK_v2 && \
    cd /tmp/OrbbecSDK_v2 && \
    git checkout ${ORBBEC_SDK_VERSION} && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX=/workspace/OrbbecSDK && \
    make -j$(nproc) && \
    make install

# Build libdatachannel from source to /usr/local
RUN git clone --recursive https://github.com/paullouisageneau/libdatachannel.git /tmp/libdatachannel \
    && cd /tmp/libdatachannel \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_GNUTLS=0 -DUSE_NICE=0 \
    && make -j$(nproc) \
    && make install

# Copy project source
WORKDIR /src
COPY . .

# Build and Install WebRTC Streamer to /app
RUN mkdir -p /tmp/build_webrtc && cd /tmp/build_webrtc && \
    cmake /src/webrtc_streamer -DCMAKE_INSTALL_PREFIX=/app -DOrbbecSDK_DIR=/workspace/OrbbecSDK/lib && \
    make -j$(nproc) && make install

# Build and Install Verification Tool to /app
RUN mkdir -p /tmp/build_verify && cd /tmp/build_verify && \
    cmake /src/verification_tool -DCMAKE_INSTALL_PREFIX=/app -DOrbbecSDK_DIR=/workspace/OrbbecSDK/lib && \
    make -j$(nproc) && make install


# --- Stage 2: Runtime ---
FROM ubuntu:24.04 AS runtime

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install ONLY runtime dependencies
# Note: Ubuntu 24.04 (Noble) uses OpenCV 4.6.0 with t64 suffix for library packages
RUN apt-get update && apt-get install -y \
    libusb-1.0-0 \
    libssl3 \
    libudev1 \
    libopencv-core406t64 \
    libopencv-imgproc406t64 \
    libopencv-imgcodecs406t64 \
    libopencv-videoio406t64 \
    libopencv-highgui406t64 \
    libavcodec60 \
    libavformat60 \
    libavutil58 \
    libswscale7 \
    python3 \
    libx11-6 \
    libxcb1 \
    libxau6 \
    libxdmcp6 \
    && rm -rf /var/lib/apt/lists/*

# Copy built artifacts from builder
COPY --from=builder /workspace/OrbbecSDK /workspace/OrbbecSDK
COPY --from=builder /app /app
COPY --from=builder /usr/local/lib/libdatachannel.so* /usr/local/lib/
COPY --from=builder /src/webrtc_streamer/web /app/web

# Update library cache
RUN ldconfig

# Set environment
WORKDIR /workspace
ENV PATH="/app/bin:${PATH}"

# Expose ports
EXPOSE 8000
EXPOSE 8080
EXPOSE 10000-10100/udp

# Default command
CMD ["/bin/bash"]
