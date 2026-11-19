"""This module defines nodes used in launch files."""

from launch_ros.actions import Node

NAVIGATOR_DIR = "/navigator/"

camera_streamer = Node(
    package='web_video_server',
    executable='web_video_server'
)

carla_guardian = Node(
    package='guardian',
    executable='guardian_node',
    parameters=[
        {'simulated': True}
    ]
)

clock = Node(
    package='clock',
    executable='clock_node'
)

costmap_recorder = Node(
    package="recording", 
    executable="costmap_recorder"
)

lidar_ground_filter = Node(
    package='lidar_ground_filter',
    executable='lidar_ground_filter',
    parameters=[
        {'sensitivity': 0.13},
    ]
)

guardian = Node(
    package='guardian',
    executable='guardian_node'
)


joy_translator = Node(
    package='joy_translation',
    executable='joy_translation_node'
)

odom2tf = Node(
    package='recording',
    executable='odom2tf'
)

recorder = Node(
    package='recording',
    executable='recorder'
)

routing_monitor = Node(
    package='helpers',
    executable='routing_monitor'
)

routing_hardcoded = Node(
    package='helpers',
    executable='routing_hardcoded',
    parameters=[{'filename': "/navigator/Route.csv"}]
)

rqt = Node(
    package='rqt_gui',
    executable='rqt_gui',
    arguments=["--perspective-file=/navigator/data/rqt.perspective"]
)

rviz = Node(
    package='rviz2',
    namespace='',
    executable='rviz2',
    name='rviz2',
    arguments=['-d' + '/navigator/data/navigator_default.rviz'],
    respawn=False,
    output={'both': 'log'}
)

sounds = Node(
    package='guardian',
    executable='sound_node'
)

static_obstacle_grid = Node(
    package='static_obstacle_grid',
    executable='static_obstacle_grid'
)

object_tracker = Node(
    package='object_tracker',
    executable='object_tracker'
)

web_bridge = Node(
    package='rosbridge_server',
    executable='rosbridge_websocket'
)

traffic_light_detector = Node(
    package='traffic_light_detector',
    executable='traffic_light_detector'
)
    
camera_object_detector = Node(
    package='camera_object_detector',
    executable='camera_object_detector'
)


