"""This module contains the launch description for the perception stack."""

from os import path
import sys

from launch import LaunchDescription

sys.path.append(path.abspath("/navigator/"))
from launches.launch_node_definitions import *


def generate_launch_description():
    return LaunchDescription([
        ground_seg,                  # /lidar --> /lidar/filtered
        # traffic_light_detector,
        # road_user_detector,        # YOLO model required - disabled until model present
    ])
