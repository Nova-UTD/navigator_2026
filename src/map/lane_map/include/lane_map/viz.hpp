// RViz markers for the lane network (ROS side: not part of core).
#pragma once

#include <string>

#include <builtin_interfaces/msg/time.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include "lane_map/core/types.hpp"

namespace lane_map
{

// Every lane: centre lines with direction chevrons, and both edges coloured by
// their painted line. Starts with a DELETEALL so a new map replaces the old one.
// `point_step`: use every n-th centre-line point (2 = every 1 m at 0.5 m spacing),
// to keep the message small on big maps.
visualization_msgs::msg::MarkerArray lane_markers(const core::LaneNetwork& network, const std::string& frame_id,
                                                  const builtin_interfaces::msg::Time& stamp, int point_step);

}  // namespace lane_map
