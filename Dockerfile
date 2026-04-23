# Use Ubuntu 24.04 as the base image
FROM ubuntu:24.04

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
    libopencv-dev \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Install libdatachannel from source or PPA if available
# Since it's a specific dependency, we'll build it to ensure compatibility
RUN apt-get update && apt-get install -y \
    libssl-dev \
    && git clone --recursive https://github.com/paullouisageneau/libdatachannel.git /tmp/libdatachannel \
    && cd /tmp/libdatachannel \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_GNUTLS=0 -DUSE_NICE=0 \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && rm -rf /tmp/libdatachannel

# Set the working directory
WORKDIR /workspace

# Copy the entire project into the container
COPY . .

# Build the WebRTC Streamer
RUN cd webrtc_streamer && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Build the Verification Tool
RUN cd verification_tool && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Expose ports (8000 for the web server, 8080 for signaling, and UDP range for WebRTC)
EXPOSE 8000
EXPOSE 8080
EXPOSE 10000-10100/udp

# Set the default command to show help or enter a shell
CMD ["/bin/bash"]
