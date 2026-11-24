// The lane network: every driving lane of a map as plain data, ready to use.
// No ROS and no libOpenDRIVE types here, so anything can read it.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace lane_map::core
{

// The painted line along one edge of a lane. One byte, because every lane
// point stores two of these and two colours.
enum class RoadMark : std::uint8_t
{
  None,
  Solid,
  Broken,
  SolidSolid,
  SolidBroken,  // solid on the inside (towards the reference line), broken outside
  BrokenSolid,
  BrokenBroken,
  Curb,
  Other,  // botts dots, grass, custom, ...: drawn, but treated as not crossable
};

// The paint colour of a road mark, as written in the map.
enum class MarkColor : std::uint8_t
{
  White,  // "white" or "standard" (OpenDRIVE's default)
  Yellow,
  Other,  // blue, green, red, orange
};

// One breadcrumb on a lane's centre line. Points run in the direction cars drive.
struct LanePoint
{
  double x = 0;            // m, map frame
  double y = 0;            // m
  double z = 0;            // m
  double heading = 0;      // rad, direction of travel, 0 = +x, counter-clockwise
  double s = 0;            // m along this lane from its first point
  double width = 0;        // m
  double speed_limit = 0;  // m/s
  RoadMark left_mark = RoadMark::None;   // line on the left edge, looking along the lane
  RoadMark right_mark = RoadMark::None;  // line on the right edge
  MarkColor left_color = MarkColor::White;
  MarkColor right_color = MarkColor::White;
};

// A lane directly beside another one, in the same lane section.
struct Neighbour
{
  std::string lane_id;
  bool same_direction = true;  // false: oncoming traffic
};

struct Lane
{
  std::string id;            // "road/section/lane", e.g. "15/0/-1" (section = index along the road)
  std::string road_id;       // OpenDRIVE road id
  int section_index = 0;     // 0 = first lane section of the road
  double section_s0 = 0;     // m, where the section starts along the road's reference line
  int odr_lane_id = 0;       // OpenDRIVE lane id: < 0 right of the reference line, > 0 left
  std::string junction_id;   // empty unless the lane is inside a junction
  std::vector<LanePoint> points;
  double length = 0;         // m, along the centre line
  std::optional<Neighbour> left;   // looking along the lane
  std::optional<Neighbour> right;
  std::vector<std::string> successors;    // lanes this one leads into, sorted
  std::vector<std::string> predecessors;  // lanes that lead into this one, sorted

  bool in_junction() const { return !junction_id.empty(); }
};

struct LaneNetwork
{
  std::map<std::string, Lane> lanes;  // by lane id, sorted so every run gives the same order
  std::vector<std::string> warnings;  // problems found in the map, one line each
};

// "road/section/lane"
inline std::string make_lane_id(const std::string& road_id, int section_index, int odr_lane_id)
{
  return road_id + "/" + std::to_string(section_index) + "/" + std::to_string(odr_lane_id);
}

}  // namespace lane_map::core
