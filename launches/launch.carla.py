"""This module contains the launch description for Navigator in the CARLA simulator."""

from os import path
import sys

from launch import LaunchDescription
import launch_ros.actions

sys.path.append(path.abspath("/navigator/"))
from launches.launch_node_definitions import *


def generate_launch_description():
    """Carla launch file description. New nodes are added here as each package lands."""

    return LaunchDescription(
        [
            # Set the global config file as a parameter for all nodes.
            launch_ros.actions.SetParameter(name="global_config", value="/navigator/param/global_parameters.yaml"),
            # MAP
            lane_map,                    # /carla/world_info --> /map/lanes/viz
            # PERCEPTION
            lidar_ground_filter,         # /lidar --> /perception/lidar/nonground
            # traffic_light_detector,
            # camera_object_detector,    # YOLO model required - disabled until model present
            # WORLD MODEL, BEHAVIOUR, PLANNING, CONTROL, SAFETY
            # VISUALISATION
            rviz,
        ]
    )
