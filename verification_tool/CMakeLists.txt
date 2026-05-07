cmake_minimum_required(VERSION 3.10)
project(verify_camera)

set(CMAKE_CXX_STANDARD 11)

# Set OrbbecSDK path
if(EXISTS "/workspace/OrbbecSDK/lib")
    set(OrbbecSDK_DIR /workspace/OrbbecSDK/lib)
    set(OBS_LIB_PATH "/workspace/OrbbecSDK/lib")
else()
    set(OrbbecSDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../OrbbecSDK/lib)
    set(OBS_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../OrbbecSDK/lib")
endif()
find_package(OrbbecSDK REQUIRED)

find_package(OpenCV REQUIRED)

include_directories(
    ${OrbbecSDK_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../webrtc_streamer/src
)

add_executable(verify_camera verify_camera.cpp)
target_link_libraries(verify_camera PRIVATE ob::OrbbecSDK)

add_executable(view_camera view_camera.cpp)
target_link_libraries(view_camera PRIVATE ob::OrbbecSDK ${OpenCV_LIBS})

add_executable(view_sbs view_sbs.cpp)
target_link_libraries(view_sbs PRIVATE ob::OrbbecSDK ${OpenCV_LIBS})

# RPATH for finding the library at runtime
if(UNIX)
    set_target_properties(verify_camera view_camera view_sbs PROPERTIES
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "${OBS_LIB_PATH}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Install rules
install(TARGETS verify_camera view_camera view_sbs DESTINATION bin)
