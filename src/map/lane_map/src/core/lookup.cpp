#include "lane_map/core/lookup.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace lane_map::core
{
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using Point = bg::model::d2::point_xy<double>;
using Polygon = bg::model::polygon<Point>;
using Box = bg::model::box<Point>;
using Entry = std::pair<Box, std::size_t>;  // a piece's bounding box and its index in `pieces`

namespace
{

// Lanes are indexed in pieces of this many points (10 m at 0.5 m spacing). A
// whole long, curving lane has a huge bounding box that matches many queries,
// and checking a point against it walks hundreds of points. Short pieces have
// tight boxes and small outlines, so a query touches only a few, small ones.
constexpr std::size_t kPiecePoints = 20;

struct Piece
{
  const Lane* lane;
  std::size_t first, last;  // centre-line points [first, last]; neighbouring pieces share an end point
  Polygon outline;
};

// The outline of points [first, last]: the left edge forwards, then the right edge backwards.
Polygon outline(const Lane& lane, std::size_t first, std::size_t last)
{
  Polygon poly;
  poly.outer().reserve(2 * (last - first + 1) + 1);
  for (std::size_t i = first; i <= last; ++i)
  {
    const LanePoint& p = lane.points[i];
    const double half = 0.5 * p.width;
    bg::append(poly.outer(), Point(p.x - half * std::sin(p.heading), p.y + half * std::cos(p.heading)));
  }
  for (std::size_t i = last + 1; i-- > first;)
  {
    const LanePoint& p = lane.points[i];
    const double half = 0.5 * p.width;
    bg::append(poly.outer(), Point(p.x + half * std::sin(p.heading), p.y - half * std::cos(p.heading)));
  }
  bg::correct(poly);  // close the ring and fix its orientation
  return poly;
}

double heading_difference(double a, double b)
{
  return std::abs(std::atan2(std::sin(a - b), std::cos(a - b)));
}

}  // namespace

struct LaneIndex::Impl
{
  std::vector<Piece> pieces;
  bgi::rtree<Entry, bgi::rstar<16>> tree;
};

LaneIndex::LaneIndex(const LaneNetwork& network) : impl_(std::make_unique<Impl>())
{
  for (const auto& [id, lane] : network.lanes)
  {
    const std::size_t n = lane.points.size();
    for (std::size_t first = 0; first + 1 < n; first += kPiecePoints)
    {
      const std::size_t last = std::min(first + kPiecePoints, n - 1);
      impl_->pieces.push_back({&lane, first, last, outline(lane, first, last)});
    }
  }

  std::vector<Entry> entries;
  entries.reserve(impl_->pieces.size());
  for (std::size_t i = 0; i < impl_->pieces.size(); ++i)
    entries.emplace_back(bg::return_envelope<Box>(impl_->pieces[i].outline), i);
  impl_->tree = bgi::rtree<Entry, bgi::rstar<16>>(entries);  // bulk loading: faster and a better tree
}

LaneIndex::~LaneIndex() = default;
LaneIndex::LaneIndex(LaneIndex&&) noexcept = default;
LaneIndex& LaneIndex::operator=(LaneIndex&&) noexcept = default;

std::optional<std::string> LaneIndex::lane_at(double x, double y, double heading) const
{
  const Point query(x, y);
  const Lane* best = nullptr;
  double best_turn = std::numeric_limits<double>::infinity();
  double best_distance = std::numeric_limits<double>::infinity();

  for (auto it = impl_->tree.qbegin(bgi::intersects(query)); it != impl_->tree.qend(); ++it)
  {
    const Piece& piece = impl_->pieces[it->second];
    if (!bg::covered_by(query, piece.outline))
      continue;

    // The lane's heading at the centre-line point nearest the query, within this piece.
    double distance = std::numeric_limits<double>::infinity();
    double lane_heading = 0;
    for (std::size_t i = piece.first; i <= piece.last; ++i)
    {
      const LanePoint& p = piece.lane->points[i];
      const double d = std::hypot(p.x - x, p.y - y);
      if (d < distance)
      {
        distance = d;
        lane_heading = p.heading;
      }
    }

    const double turn = heading_difference(lane_heading, heading);
    if (turn > M_PI / 2)
      continue;  // drives the other way
    // Prefer the lane pointing most nearly our way; on a tie, the nearer centre line.
    if (turn < best_turn - 1e-6 || (std::abs(turn - best_turn) <= 1e-6 && distance < best_distance))
    {
      best_turn = turn;
      best_distance = distance;
      best = piece.lane;
    }
  }
  if (!best)
    return std::nullopt;
  return best->id;
}

}  // namespace lane_map::core
