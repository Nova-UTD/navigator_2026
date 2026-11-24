#include "lane_map/core/build.hpp"

#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "Lane.h"
#include "LaneSection.h"
#include "OpenDriveMap.h"
#include "Road.h"
#include "RoadMark.h"

namespace lane_map::core
{
namespace
{
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using GeoPoint = bg::model::d2::point_xy<double>;
using GeoBox = bg::model::box<GeoPoint>;

std::string lower(std::string text)
{
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
  return text;
}

double wrap_angle(double a)
{
  return std::atan2(std::sin(a), std::cos(a));
}

RoadMark parse_mark(const std::string& odr_type)
{
  const std::string type = lower(odr_type);
  if (type.empty() || type == "none")
    return RoadMark::None;
  if (type == "solid")
    return RoadMark::Solid;
  if (type == "broken")
    return RoadMark::Broken;
  if (type == "solid solid")
    return RoadMark::SolidSolid;
  if (type == "solid broken")
    return RoadMark::SolidBroken;
  if (type == "broken solid")
    return RoadMark::BrokenSolid;
  if (type == "broken broken")
    return RoadMark::BrokenBroken;
  if (type == "curb")
    return RoadMark::Curb;
  return RoadMark::Other;
}

// A speed record's limit in m/s, or `fallback` if the map gives no usable number.
double to_mps(const odr::SpeedRecord& record, double fallback)
{
  char* end = nullptr;
  const double value = std::strtod(record.max.c_str(), &end);
  if (end == record.max.c_str() || !std::isfinite(value) || value <= 0)
    return fallback;  // empty, "no limit", "undefined", ...

  const std::string unit = lower(record.unit);
  if (unit == "mph")
    return value * 0.44704;
  if (unit == "km/h" || unit == "kmh")
    return value / 3.6;
  return value;  // "m/s", or no unit (OpenDRIVE's default is m/s)
}

// The speed limit in force at `s` along `road`.
double speed_at(const odr::Road& road, double s, double fallback)
{
  auto it = road.s_to_speed.upper_bound(s);
  if (it == road.s_to_speed.begin())
    return fallback;
  return to_mps(std::prev(it)->second, fallback);
}

MarkColor parse_color(const std::string& odr_color)
{
  const std::string color = lower(odr_color);
  if (color.empty() || color == "standard" || color == "white")
    return MarkColor::White;
  if (color == "yellow")
    return MarkColor::Yellow;
  return MarkColor::Other;
}

struct Mark
{
  RoadMark type = RoadMark::None;
  MarkColor color = MarkColor::White;
};

// The painted line along a lane's outer edge at `s` (along the road). A lane's
// road-mark groups each start at an s and last until the next one starts.
Mark mark_at(const odr::Lane& lane, double s)
{
  Mark found;
  for (const odr::RoadMarkGroup& group : lane.roadmark_groups)  // sorted by start
    if (group.lanesection_s0 + group.s_offset <= s + 1e-9)
      found = {parse_mark(group.type), parse_color(group.color)};
  return found;
}

int toward_zero(int odr_lane_id)
{
  return odr_lane_id > 0 ? odr_lane_id - 1 : odr_lane_id + 1;
}

// Along a road, s values every `spacing` from `start` to `end`, always including `end`.
std::vector<double> sample(double start, double end, double spacing)
{
  // start + i * spacing, not repeated addition, so rounding doesn't build up.
  const auto n = static_cast<std::size_t>(std::ceil((end - start - 1e-3) / spacing));
  std::vector<double> s_values;
  s_values.reserve(n + 1);
  for (std::size_t i = 0; i < n; ++i)
    s_values.push_back(start + static_cast<double>(i) * spacing);
  s_values.push_back(end);
  return s_values;
}

// Distances along the line and the heading at each point.
void fill_s_and_heading(std::vector<LanePoint>& points)
{
  const std::size_t n = points.size();
  if (n == 0)
    return;

  points[0].s = 0;
  for (std::size_t i = 1; i < n; ++i)
    points[i].s = points[i - 1].s + std::hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);

  for (std::size_t i = 0; i < n; ++i)
  {
    // Central difference inside the line, one-sided at its ends.
    const LanePoint& a = points[i == 0 ? 0 : i - 1];
    const LanePoint& b = points[i + 1 == n ? n - 1 : i + 1];
    if (std::hypot(b.x - a.x, b.y - a.y) > 1e-9)
      points[i].heading = std::atan2(b.y - a.y, b.x - a.x);
    else if (i > 0)
      points[i].heading = points[i - 1].heading;
  }
}

// Does `to` carry on where `from` ends? `from`'s end must run into `to`'s start:
// within the tolerance along the road, and, sideways, inside `to`'s width (plus
// the tolerance), pointing the same way at the same height. Landing inside the
// width rather than on the centre covers a lane that ends by narrowing to 0 m
// (a merge: it ends on the edge of the lane it joins) and lanes shifted sideways
// where a new lane section starts (roadworks). An adjacent lane, a whole lane
// width away, doesn't count; nor does a lane that opens up beside us from 0 m.
bool connects(const Lane& from, const Lane& to, const Config& config)
{
  if (from.points.empty() || to.points.empty())
    return false;
  const LanePoint& end = from.points.back();
  const LanePoint& start = to.points.front();
  const double dx = start.x - end.x, dy = start.y - end.y;
  const double along = dx * std::cos(end.heading) + dy * std::sin(end.heading);
  const double sideways = -dx * std::sin(end.heading) + dy * std::cos(end.heading);
  return std::abs(along) <= config.connect_tolerance_m &&
         std::abs(sideways) <= 0.5 * start.width + config.connect_tolerance_m &&
         std::abs(start.z - end.z) <= config.connect_height_tolerance_m &&
         std::abs(wrap_angle(start.heading - end.heading)) <= config.connect_heading_tolerance_rad;
}

odr::OpenDriveMap check_loaded(odr::OpenDriveMap map, const std::string& what)
{
  if (map.id_to_road.empty())
    throw std::runtime_error(what + " has no roads. Is it a valid OpenDRIVE file?");
  return map;
}

}  // namespace

odr::OpenDriveMap load_map_file(const std::string& path)
{
  if (!std::filesystem::is_regular_file(path))
    throw std::runtime_error("Map file not found: " + path);
  try
  {
    return check_loaded(odr::OpenDriveMap(path), "Map file " + path);
  }
  catch (const std::runtime_error& e)
  {
    if (std::string(e.what()).rfind("Map file", 0) == 0)
      throw;
    throw std::runtime_error("Could not read map file " + path + ": " + e.what());
  }
}

odr::OpenDriveMap load_map_string(const std::string& xodr)
{
  if (xodr.empty())
    throw std::runtime_error("The map text is empty");

  std::string path = (std::filesystem::temp_directory_path() / "lane_map_XXXXXX").string();
  const int fd = mkstemp(path.data());
  if (fd < 0)
    throw std::runtime_error("Could not create a temporary file for the map");
  close(fd);

  struct RemoveOnExit
  {
    std::string path;
    ~RemoveOnExit() { std::remove(path.c_str()); }
  } cleanup{path};

  {
    std::ofstream out(path, std::ios::binary);
    out << xodr;
    if (!out)
      throw std::runtime_error("Could not write the map to " + path);
  }
  try
  {
    return check_loaded(odr::OpenDriveMap(path), "The map text");
  }
  catch (const std::runtime_error& e)
  {
    if (std::string(e.what()).rfind("The map text", 0) == 0)
      throw;
    throw std::runtime_error(std::string("Could not read the map text: ") + e.what());
  }
}

LaneNetwork build_network(const odr::OpenDriveMap& map, const Config& config)
{
  LaneNetwork network;

  // 1. One lane record per driving lane.
  for (const auto& [road_id, road] : map.id_to_road)
  {
    const bool rht = !road.left_hand_traffic;
    int section_index = 0;
    for (const auto& [s0, section] : road.s_to_lanesection)
    {
      const int this_section = section_index++;
      const double s_end = road.get_lanesection_end(s0);
      if (s_end - s0 < 1e-3)
        continue;  // a zero-length section has no lanes to drive

      for (const auto& [odr_lane_id, odr_lane] : section.id_to_lane)
      {
        if (odr_lane_id == 0 || odr_lane.type != "driving")
          continue;

        const std::string id = make_lane_id(road_id, this_section, odr_lane_id);
        const odr::Lane& inner_lane = section.id_to_lane.at(toward_zero(odr_lane_id));

        Lane lane;
        lane.id = id;
        lane.road_id = road_id;
        lane.section_index = this_section;
        lane.section_s0 = s0;
        lane.odr_lane_id = odr_lane_id;
        if (!road.junction.empty() && road.junction != "-1")
          lane.junction_id = road.junction;

        double max_width = 0;
        const std::vector<double> s_values = sample(s0, s_end, config.sample_spacing_m);
        lane.points.reserve(s_values.size());
        for (const double s : s_values)
        {
          const double outer_t = odr_lane.outer_border.evaluate(s);
          const double inner_t = inner_lane.outer_border.evaluate(s);
          // get_xyz includes the road's elevation and banking (superelevation).
          // get_surface_pt would also add crossfall and lane height offsets (a
          // few cm), but it copies lane objects internally and is 6-10x slower:
          // ~85% of the whole build. z is only used for the 1 m same-height
          // check and for drawing, so get_xyz is plenty.
          const odr::Vec3D p = road.get_xyz(s, 0.5 * (outer_t + inner_t), 0.0);

          LanePoint point;
          point.x = p[0];
          point.y = p[1];
          point.z = p[2];
          point.width = std::abs(outer_t - inner_t);
          point.speed_limit = speed_at(road, s, config.default_speed_limit_mps);
          // A lane's own marks are on its outer edge; its inner edge's are the
          // next lane in's. In right-hand traffic the inner edge (nearer the
          // reference line) is on the driver's left, whichever way the lane drives.
          const Mark outer = mark_at(odr_lane, s);
          const Mark inner = mark_at(inner_lane, s);
          const Mark& left = rht ? inner : outer;
          const Mark& right = rht ? outer : inner;
          point.left_mark = left.type;
          point.left_color = left.color;
          point.right_mark = right.type;
          point.right_color = right.color;
          lane.points.push_back(point);
          max_width = std::max(max_width, point.width);
        }

        if (max_width < config.min_lane_width_m)
        {
          network.warnings.push_back("Lane " + id + " is never wider than " +
                                     std::to_string(config.min_lane_width_m) + " m; skipped");
          continue;
        }

        // Points run the way cars drive: along the reference line for lanes on
        // its right in right-hand traffic, against it for lanes on its left.
        const bool along_reference_line = (odr_lane_id < 0) == rht;
        if (!along_reference_line)
          std::reverse(lane.points.begin(), lane.points.end());
        fill_s_and_heading(lane.points);
        lane.length = lane.points.back().s;

        network.lanes.emplace(id, std::move(lane));
      }
    }
  }

  // 2. Neighbours: the lanes beside each one in the same section, looking along it.
  for (auto& [id, lane] : network.lanes)
  {
    const odr::Road& road = map.id_to_road.at(lane.road_id);
    const odr::LaneSection& section = road.s_to_lanesection.at(lane.section_s0);
    const bool rht = !road.left_hand_traffic;
    const int toward_ref = lane.odr_lane_id > 0 ? lane.odr_lane_id - 1 : lane.odr_lane_id + 1;
    const int away_from_ref = lane.odr_lane_id > 0 ? lane.odr_lane_id + 1 : lane.odr_lane_id - 1;
    // Lane 0 is the reference line itself, not a lane: step over it.
    const auto skip_zero = [&](int other) { return other != 0 ? other : (lane.odr_lane_id > 0 ? -1 : 1); };

    const auto neighbour = [&](int other_odr_id) -> std::optional<Neighbour> {
      auto it = section.id_to_lane.find(other_odr_id);
      if (it == section.id_to_lane.end())
        return std::nullopt;
      const std::string other_id = make_lane_id(lane.road_id, lane.section_index, other_odr_id);
      if (network.lanes.count(other_id) == 0)
        return std::nullopt;  // not a driving lane, or skipped
      return Neighbour{other_id, (other_odr_id > 0) == (lane.odr_lane_id > 0)};
    };
    lane.left = neighbour(rht ? skip_zero(toward_ref) : away_from_ref);
    lane.right = neighbour(rht ? away_from_ref : skip_zero(toward_ref));
  }

  // 3. Successors: every lane this one runs into where it ends (see connects()).
  // This comes from the geometry, not from the links written in the map file,
  // because those are unreliable: CARLA's Town07 leaves out links between lanes
  // that really join, and Town03 and Town06 list links to lanes that start
  // metres away. The geometry is what a car actually drives.
  std::vector<std::pair<GeoPoint, const Lane*>> starts;
  double widest_start = 0;
  for (const auto& [id, lane] : network.lanes)
  {
    starts.emplace_back(GeoPoint(lane.points.front().x, lane.points.front().y), &lane);
    widest_start = std::max(widest_start, lane.points.front().width);
  }
  const bgi::rtree<std::pair<GeoPoint, const Lane*>, bgi::rstar<16>> start_index(starts);

  for (auto& [id, lane] : network.lanes)
  {
    const LanePoint& end = lane.points.back();
    // Search far enough to reach any lane whose width this end could land in.
    const double r = config.connect_tolerance_m + 0.5 * widest_start;
    const GeoBox around_end(GeoPoint(end.x - r, end.y - r), GeoPoint(end.x + r, end.y + r));
    std::set<std::string> found;
    for (auto it = start_index.qbegin(bgi::intersects(around_end)); it != start_index.qend(); ++it)
      if (connects(lane, *it->second, config))
        found.insert(it->second->id);
    lane.successors.assign(found.begin(), found.end());
  }
  for (const auto& [id, lane] : network.lanes)
    for (const std::string& next : lane.successors)
      network.lanes.at(next).predecessors.push_back(id);
  for (auto& [id, lane] : network.lanes)
    std::sort(lane.predecessors.begin(), lane.predecessors.end());

  return network;
}

}  // namespace lane_map::core
