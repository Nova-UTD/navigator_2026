"""This module contains the launch description for the vehicle."""

from os import path
import sys

from launch import LaunchDescription
import launch_ros.actions

sys.path.append(path.abspath("/navigator/"))
from launches.launch_node_definitions import *
from launches.utils import (
    err_fatal,
    try_get_launch_description_from_include,
    IncludeError,
)


def generate_launch_description():
    """Vehicle launch file description."""

    # Include perception launch file and extract launch entities for reuse.
    try:
        perception_launch_description = try_get_launch_description_from_include(
            "launches/launch.perception.py"
        )
    except IncludeError as e:
        err_fatal(e)

    perception_launch_entities = perception_launch_description.describe_sub_entities()

    return LaunchDescription(
        [
            # Set the global config file as a parameter for all nodes.
            launch_ros.actions.SetParameter(name="global_config", value="/navigator/param/global_parameters.yaml"),
            # PERCEPTION
            *perception_launch_entities,
            # MAP, WORLD MODEL, BEHAVIOUR, PLANNING, CONTROL, SAFETY
            # Added here as each new package lands (docs/NEW_ARCHITECTURE.md §4.3).
            rviz,
        ]
    )
