"""Run lane_map on its own.

    ros2 launch lane_map lane_map.launch.py                                        # map from CARLA
    ros2 launch lane_map lane_map.launch.py town:=Town03 rviz:=true                # a CARLA town's file, no CARLA needed
    ros2 launch lane_map lane_map.launch.py map_file:=/navigator/data/maps/campus.xodr rviz:=true

town:= reads the town's .xodr from CARLA's install (mounted at /carla), so you
can look at any town's lanes without running CARLA. rviz:=true opens RViz
looking at the map frame.
"""

import os
from os import path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

CARLA_TOWNS = "/carla/CarlaUE4/Content/Carla/Maps/OpenDrive"


def lane_map_node(context):
    map_file = LaunchConfiguration("map_file").perform(context)
    town = LaunchConfiguration("town").perform(context)
    if not map_file and town:
        map_file = path.join(CARLA_TOWNS, f"{town}.xodr")
        if not path.isfile(map_file):
            towns = sorted(f[:-5] for f in os.listdir(CARLA_TOWNS)
                           if f.endswith(".xodr") and "_Opt" not in f) if path.isdir(CARLA_TOWNS) else []
            raise RuntimeError(f"No map for town '{town}' at {map_file}. Towns: {', '.join(towns) or 'none found'}")

    params = path.join(get_package_share_directory("lane_map"), "config", "params.yaml")
    overrides = {"map_source": "file", "map_file": map_file} if map_file else {"map_source": "carla"}
    return [Node(package="lane_map", executable="lane_map_node", name="lane_map", output="screen",
                 parameters=[params, overrides])]


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("map_file", default_value="", description="A .xodr file to load."),
            DeclareLaunchArgument("town", default_value="", description="A CARLA town to load from its file, e.g. Town03."),
            DeclareLaunchArgument("rviz", default_value="false", description="Also open RViz, looking at the map frame."),
            OpaqueFunction(function=lane_map_node),
            Node(
                package="rviz2",
                executable="rviz2",
                arguments=["-d", "/navigator/data/navigator_default.rviz", "-f", "map"],
                condition=IfCondition(LaunchConfiguration("rviz")),
            ),
        ]
    )
