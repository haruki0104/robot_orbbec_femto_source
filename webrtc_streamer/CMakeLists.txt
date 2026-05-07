cmake_minimum_required(VERSION 3.10)
project(webrtc_streamer)

set(CMAKE_CXX_STANDARD 17) # libdatachannel usually requires C++17

# 1. Orbbec SDK
if(EXISTS "/workspace/OrbbecSDK/lib")
    set(OrbbecSDK_DIR /workspace/OrbbecSDK/lib)
    set(OBS_LIB_PATH "/workspace/OrbbecSDK/lib")
else()
    set(OrbbecSDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../OrbbecSDK/lib)
    set(OBS_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../OrbbecSDK/lib")
endif()
find_package(OrbbecSDK REQUIRED)

# 2. OpenCV
find_package(OpenCV REQUIRED)

# 3. LibDataChannel
find_package(LibDataChannel CONFIG)
if(NOT LibDataChannel_FOUND)
    pkg_check_modules(LIBDATACHANNEL datachannel)
    if(NOT LIBDATACHANNEL_FOUND)
        message(FATAL_ERROR "LibDataChannel not found. Please install it: sudo apt install libdatachannel-dev")
    endif()
endif()

# 4. FFmpeg (avcodec, avutil, swscale)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC libavcodec REQUIRED)
pkg_check_modules(AVUTIL libavutil REQUIRED)
pkg_check_modules(SWSCALE libswscale REQUIRED)

include_directories(
    src
    ${OrbbecSDK_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${LIBDATACHANNEL_INCLUDE_DIRS}
)

add_executable(webrtc_streamer src/main.cpp)
add_executable(signaling_server src/signaling_server.cpp)

target_link_libraries(webrtc_streamer 
    PRIVATE 
    ob::OrbbecSDK 
    ${OpenCV_LIBS}
    ${LIBDATACHANNEL_LIBRARIES}
    datachannel # Try both if one is a target and other is a lib
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWSCALE_LIBRARIES}
)

target_link_libraries(signaling_server
    PRIVATE
    ${LIBDATACHANNEL_LIBRARIES}
    datachannel
)

# RPATH for OrbbecSDK and libdatachannel
if(UNIX)
    set_target_properties(webrtc_streamer signaling_server PROPERTIES
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "${OBS_LIB_PATH}:/usr/local/lib"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Install rules
install(TARGETS webrtc_streamer signaling_server DESTINATION bin)
