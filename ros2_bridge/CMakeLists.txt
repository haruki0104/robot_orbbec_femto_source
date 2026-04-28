cmake_minimum_required(VERSION 3.10)
project(femto_mega_bridge)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# Find OrbbecSDK (adjust path if needed)
set(OrbbecSDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../OrbbecSDK/lib)
find_package(OrbbecSDK REQUIRED)

# Set RPATH so the library is found without LD_LIBRARY_PATH
set(CMAKE_INSTALL_RPATH "${OrbbecSDK_DIR}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_executable(femto_mega_node src/femto_mega_node.cpp)
target_include_directories(femto_mega_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(femto_mega_node
  ob::OrbbecSDK
)
ament_target_dependencies(femto_mega_node
  rclcpp
  sensor_msgs
  geometry_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  cv_bridge
  OpenCV
)

install(TARGETS femto_mega_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
