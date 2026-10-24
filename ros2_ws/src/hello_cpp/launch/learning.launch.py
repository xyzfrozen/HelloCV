from pathlib import Path
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config=Path(get_package_share_directory("hello_cpp"))/"config"/"detector.yaml"

    return LaunchDescription([
        Node(
            package="hello_cpp",
            executable="status_publisher",
            output="screen",
            remappings=[("vision/status","vision/demo_status")],
        ),
        Node(
            package="hello_cpp",
            executable="status_listener",
            output="screen",
            remappings=[("vision/status","vision/demo_status")],
        ),
        Node(
            package="hello_cpp",
            executable="detection_server",
            output="screen",
        ),
        Node(
            package="hello_cpp",
            executable="threshold_node",
            parameters=[str(config)],
            output="screen",
        ),
    ])
