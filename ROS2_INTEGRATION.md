# ROS 2 Integration Guide for Femto Mega

This guide explains how to use the sample `ros2_bridge` included in this project.

## Prerequisites
- **OS**: Ubuntu 22.04
- **ROS 2**: Humble Hawksbill
- **Hardware**: Orbbec Femto Mega connected via USB or Ethernet

## How to Build the Bridge
1. Create a ROS 2 workspace:
   ```bash
   mkdir -p ~/ros2_ws/src
   cd ~/ros2_ws/src
   ```
2. Link this project into your workspace:
   ```bash
   ln -s /path/to/this/project/ros2_bridge .
   ```
3. Build the package:
   ```bash
   cd ~/ros2_ws
   source /opt/ros/humble/setup.bash
   colcon build --packages-select femto_mega_bridge
   ```

## Running the Node
1. Source the workspace:
   ```bash
   source ~/ros2_ws/install/setup.bash
   ```
2. Run the node:
   ```bash
   ros2 run femto_mega_bridge femto_mega_node
   ```

## Viewing the Stream
In another terminal, run RViz or the simple image viewer:
```bash
ros2 run rqt_image_view rqt_image_view
```
Select the topic `/camera/color/image_raw` to see the live feed.

## Next Steps
- **Depth Stream**: Add a similar publisher for depth images in `femto_mega_node.cpp`.
- **Official Wrapper**: For production use, consider using [orbbec_ros2](https://github.com/orbbec/orbbec_ros2) which is a full-featured driver.
