cmake_minimum_required(VERSION 3.10)
project(verify_camera)

set(CMAKE_CXX_STANDARD 11)

# Set OrbbecSDK path
set(OrbbecSDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../OrbbecSDK/lib)
find_package(OrbbecSDK REQUIRED)

find_package(OpenCV REQUIRED)

add_executable(verify_camera verify_camera.cpp)
target_link_libraries(verify_camera PRIVATE ob::OrbbecSDK)

add_executable(view_camera view_camera.cpp)
target_link_libraries(view_camera PRIVATE ob::OrbbecSDK ${OpenCV_LIBS})

# RPATH for finding the library at runtime
if(UNIX)
    set_target_properties(verify_camera view_camera PROPERTIES
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN/../OrbbecSDK/lib"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()
