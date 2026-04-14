cmake_minimum_required(VERSION 3.10)
project(femto_mega_bridge)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# Find OrbbecSDK (adjust path if needed)
set(OrbbecSDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../OrbbecSDK/lib)
find_package(OrbbecSDK REQUIRED)

add_executable(femto_mega_node src/femto_mega_node.cpp)
target_include_directories(femto_mega_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(femto_mega_node
  rclcpp::rclcpp
  sensor_msgs::sensor_msgs
  cv_bridge::cv_bridge
  ${OpenCV_LIBS}
  ob::OrbbecSDK
)

install(TARGETS femto_mega_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
