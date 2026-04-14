import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    return LaunchDescription([
        # Parameters
        DeclareLaunchArgument(
            'camera_ip',
            default_value='',
            description='IP Address of the Femto Mega (leave empty for USB)'
        ),
        
        # Node
        Node(
            package='femto_mega_bridge',
            executable='femto_mega_node',
            name='femto_mega_node',
            output='screen',
            parameters=[{
                'camera_ip': LaunchConfiguration('camera_ip'),
            }],
            # Remap topics if needed
            # remappings=[
            #     ('/camera/color/image_raw', '/my_robot/color'),
            # ]
        )
    ])
