cmake_minimum_required(VERSION 3.10)
project(orbec_tests CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(GTest REQUIRED)
find_package(OpenCV REQUIRED)

# OrbbecSDK include paths (needed because sbs_compositor.hpp includes libobsensor/ObSensor.hpp)
if(EXISTS "/workspace/OrbbecSDK/include")
    set(OBS_INCLUDE_DIR /workspace/OrbbecSDK/include)
else()
    set(OBS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../OrbbecSDK/include)
endif()
if(EXISTS "/workspace/OrbbecSDK/lib")
    set(OBS_LIB_DIR /workspace/OrbbecSDK/lib)
else()
    set(OBS_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../OrbbecSDK/lib)
endif()

enable_testing()

# Include paths for source headers
set(VERIFICATION_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../verification_tool)
set(WEBRTC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../webrtc_streamer/src)

# ---- test_argparse ----
add_executable(test_argparse
    test_argparse.cpp
    ${VERIFICATION_SRC}/argparse.hpp
)
target_include_directories(test_argparse PRIVATE ${VERIFICATION_SRC})
target_link_libraries(test_argparse PRIVATE GTest::GTest GTest::Main)
add_test(NAME test_argparse COMMAND test_argparse)

# ---- test_sbs_compositor ----
add_executable(test_sbs_compositor
    test_sbs_compositor.cpp
    ${WEBRTC_SRC}/sbs_compositor.hpp
)
target_include_directories(test_sbs_compositor PRIVATE
    ${WEBRTC_SRC}
    ${OBS_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(test_sbs_compositor PRIVATE GTest::GTest GTest::Main ${OpenCV_LIBS})
add_test(NAME test_sbs_compositor COMMAND test_sbs_compositor)

# ---- test_h264_encoder (optional, needs FFmpeg) ----
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(AVCODEC libavcodec IMPORTED_TARGET QUIET)
    pkg_check_modules(AVUTIL libavutil IMPORTED_TARGET QUIET)
    pkg_check_modules(SWSCALE libswscale IMPORTED_TARGET QUIET)
    if(AVCODEC_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND)
        add_executable(test_h264_encoder
            test_h264_encoder.cpp
            ${WEBRTC_SRC}/h264_encoder.hpp
        )
        target_include_directories(test_h264_encoder PRIVATE
            ${WEBRTC_SRC}
            ${OpenCV_INCLUDE_DIRS}
            ${AVCODEC_INCLUDE_DIRS}
            ${AVUTIL_INCLUDE_DIRS}
            ${SWSCALE_INCLUDE_DIRS}
        )
        target_link_libraries(test_h264_encoder PRIVATE
            GTest::GTest GTest::Main
            ${OpenCV_LIBS}
            PkgConfig::AVCODEC
            PkgConfig::AVUTIL
            PkgConfig::SWSCALE
        )
        add_test(NAME test_h264_encoder COMMAND test_h264_encoder)
    endif()
endif()
