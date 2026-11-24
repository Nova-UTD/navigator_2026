// Turning an OpenDRIVE map into a LaneNetwork.
#pragma once

#include <string>

#include "lane_map/core/config.hpp"
#include "lane_map/core/types.hpp"

namespace odr
{
class OpenDriveMap;
}

namespace lane_map::core
{

// Load a .xodr file. Throws std::runtime_error with a clear message if the file
// is missing, isn't valid OpenDRIVE, or has no roads.
odr::OpenDriveMap load_map_file(const std::string& path);

// Load a map given as XML text (CARLA sends it this way). libOpenDRIVE only
// reads files, so the text is written to a temporary file first. Same errors.
odr::OpenDriveMap load_map_string(const std::string& xodr);

// Build the lane network: every driving lane, its centre line, neighbours,
// successors and speed limits. Deterministic: the same map always gives the
// same network.
LaneNetwork build_network(const odr::OpenDriveMap& map, const Config& config);

}  // namespace lane_map::core
