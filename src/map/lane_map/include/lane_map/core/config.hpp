// Every number lane_map uses. Filled from config/params.yaml by the node; the
// defaults here match that file so tests can use Config{} directly.
#pragma once

namespace lane_map::core
{

struct Config
{
  double sample_spacing_m = 0.5;       // m between centre-line points
  double default_speed_limit_mps = 13.4;  // m/s (30 mph), for roads the map gives no limit
  double min_lane_width_m = 0.05;      // m; lanes never wider than this are dropped
  double connect_tolerance_m = 0.5;    // m; a lane's end must be this close to its successor's start
                                       // along the road, and within its width plus this sideways
  double connect_heading_tolerance_rad = 1.05;  // rad (60 deg); and point roughly the same way
  double connect_height_tolerance_m = 1.0;      // m; and be at the same height (not a bridge above)
};

}  // namespace lane_map::core
