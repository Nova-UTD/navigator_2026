#include "lane_map/viz.hpp"

#include <cmath>
#include <map>
#include <optional>
#include <vector>

#include <geometry_msgs/msg/point.hpp>
#include <std_msgs/msg/color_rgba.hpp>

namespace lane_map
{
namespace
{
using visualization_msgs::msg::Marker;
using core::RoadMark;

// Colours chosen to read on RViz's light grey background.
enum class Style
{
  WhiteSolid,   // painted white line (drawn dark grey)
  WhiteBroken,  // dashed white line
  YellowSolid,  // painted yellow line (drawn amber)
  YellowBroken,
  Curb,
  Unmarked,  // no painted line: faint, so the lane's shape still shows
};

struct StyleInfo
{
  const char* ns;
  float r, g, b, a;
  double width;  // m
};

StyleInfo info(Style style)
{
  switch (style)
  {
    case Style::WhiteSolid: return {"edges/white_solid", 0.25f, 0.25f, 0.25f, 1.0f, 0.15};
    case Style::WhiteBroken: return {"edges/white_broken", 0.25f, 0.25f, 0.25f, 1.0f, 0.15};
    case Style::YellowSolid: return {"edges/yellow_solid", 0.95f, 0.65f, 0.0f, 1.0f, 0.15};
    case Style::YellowBroken: return {"edges/yellow_broken", 0.95f, 0.65f, 0.0f, 1.0f, 0.15};
    case Style::Curb: return {"edges/curb", 0.55f, 0.40f, 0.25f, 1.0f, 0.25};
    case Style::Unmarked: return {"edges/unmarked", 0.6f, 0.6f, 0.6f, 0.6f, 0.05};
  }
  return {"edges/unmarked", 0.6f, 0.6f, 0.6f, 0.6f, 0.05};
}

bool is_broken(RoadMark mark)
{
  return mark == RoadMark::Broken || mark == RoadMark::BrokenBroken;
}

// The paint as the map describes it. Other colours (blue, red, ...) are drawn as white.
Style style_for(RoadMark mark, core::MarkColor color)
{
  switch (mark)
  {
    case RoadMark::None:
    case RoadMark::Other: return Style::Unmarked;
    case RoadMark::Curb: return Style::Curb;
    default:
      if (color == core::MarkColor::Yellow)
        return is_broken(mark) ? Style::YellowBroken : Style::YellowSolid;
      return is_broken(mark) ? Style::WhiteBroken : Style::WhiteSolid;
  }
}

// An edge shared by two lanes is drawn once, by the lane whose id sorts first.
// (Drawn twice, the dashes of two lanes running opposite ways fill each other's
// gaps and a dashed line looks solid.)
bool draws_edge(const core::Lane& lane, const std::optional<core::Neighbour>& beside)
{
  return !beside || lane.id < beside->lane_id;
}

geometry_msgs::msg::Point point(double x, double y, double z)
{
  geometry_msgs::msg::Point p;
  p.x = x;
  p.y = y;
  p.z = z;
  return p;
}

Marker line_list(const std::string& ns, const std::string& frame_id, const builtin_interfaces::msg::Time& stamp,
                 float r, float g, float b, float a, double width)
{
  Marker m;
  m.header.frame_id = frame_id;
  m.header.stamp = stamp;
  m.ns = ns;
  m.id = 0;
  m.type = Marker::LINE_LIST;
  m.action = Marker::ADD;
  m.pose.orientation.w = 1.0;
  m.scale.x = width;
  m.color.r = r;
  m.color.g = g;
  m.color.b = b;
  m.color.a = a;
  return m;
}

constexpr double kDash = 3.0;       // m, length of each dash and each gap
constexpr double kChevronEvery = 10.0;  // m between direction chevrons
constexpr double kChevronSize = 0.8;    // m
constexpr double kLift = 0.05;          // m above the road, so lines aren't hidden in it

}  // namespace

visualization_msgs::msg::MarkerArray lane_markers(const core::LaneNetwork& network, const std::string& frame_id,
                                                  const builtin_interfaces::msg::Time& stamp, int point_step)
{
  const int step = std::max(1, point_step);
  Marker centre = line_list("centre_lines", frame_id, stamp, 0.15f, 0.45f, 0.85f, 0.8f, 0.05);
  Marker chevrons = line_list("direction", frame_id, stamp, 0.15f, 0.45f, 0.85f, 0.8f, 0.08);
  std::map<Style, Marker> edges;

  // Size the centre-line buffer once (2 points per drawn segment) instead of growing it.
  std::size_t segments = 0;
  for (const auto& [id, lane] : network.lanes)
    segments += lane.points.size() / step + 1;
  centre.points.reserve(2 * segments);

  for (const auto& [id, lane] : network.lanes)
  {
    // Points used for drawing: every `step`-th, always including the last.
    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < lane.points.size(); i += step)
      idx.push_back(i);
    if (idx.back() != lane.points.size() - 1)
      idx.push_back(lane.points.size() - 1);

    const bool draw_left = draws_edge(lane, lane.left);
    const bool draw_right = draws_edge(lane, lane.right);

    double next_chevron = kChevronEvery / 2;
    for (std::size_t k = 0; k + 1 < idx.size(); ++k)
    {
      const core::LanePoint& a = lane.points[idx[k]];
      const core::LanePoint& b = lane.points[idx[k + 1]];
      centre.points.push_back(point(a.x, a.y, a.z + kLift));
      centre.points.push_back(point(b.x, b.y, b.z + kLift));

      // Each edge: centre point moved half the lane width to the side.
      for (const int side : {+1, -1})  // +1 = left, -1 = right
      {
        if (!(side > 0 ? draw_left : draw_right))
          continue;
        const Style style = side > 0 ? style_for(a.left_mark, a.left_color) : style_for(a.right_mark, a.right_color);
        if ((style == Style::WhiteBroken || style == Style::YellowBroken) && std::fmod(a.s, 2 * kDash) >= kDash)
          continue;  // a gap in the dashes
        auto it = edges.find(style);
        if (it == edges.end())
        {
          const StyleInfo s = info(style);
          it = edges.emplace(style, line_list(s.ns, frame_id, stamp, s.r, s.g, s.b, s.a, s.width)).first;
        }
        const double ha = side * 0.5 * a.width, hb = side * 0.5 * b.width;
        it->second.points.push_back(point(a.x - ha * std::sin(a.heading), a.y + ha * std::cos(a.heading), a.z + kLift));
        it->second.points.push_back(point(b.x - hb * std::sin(b.heading), b.y + hb * std::cos(b.heading), b.z + kLift));
      }

      // A ">" pointing the way the lane drives.
      if (a.s >= next_chevron)
      {
        next_chevron += kChevronEvery;
        const double c = std::cos(a.heading), s = std::sin(a.heading);
        const auto tip = point(a.x, a.y, a.z + kLift);
        for (const int side : {+1, -1})
        {
          chevrons.points.push_back(tip);
          chevrons.points.push_back(point(a.x - kChevronSize * c - side * kChevronSize * s,
                                          a.y - kChevronSize * s + side * kChevronSize * c, a.z + kLift));
        }
      }
    }
  }

  visualization_msgs::msg::MarkerArray array;
  Marker clear;
  clear.action = Marker::DELETEALL;
  array.markers.push_back(clear);
  array.markers.push_back(centre);
  array.markers.push_back(chevrons);
  for (auto& [style, marker] : edges)
    array.markers.push_back(marker);
  return array;
}

}  // namespace lane_map
