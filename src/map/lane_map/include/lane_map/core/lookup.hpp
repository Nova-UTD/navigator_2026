// "Which lane is this point in?"
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "lane_map/core/types.hpp"

namespace lane_map::core
{

class LaneIndex
{
public:
  // Builds a spatial index (R-tree) of every lane's outline. The network must
  // outlive the index.
  explicit LaneIndex(const LaneNetwork& network);
  ~LaneIndex();
  LaneIndex(LaneIndex&&) noexcept;
  LaneIndex& operator=(LaneIndex&&) noexcept;

  // The lane containing (x, y) whose direction is within 90 degrees of
  // `heading` (rad). Where several match (junctions, where lanes overlap), the
  // one pointing most nearly along `heading`. Empty if the point is in no lane.
  std::optional<std::string> lane_at(double x, double y, double heading) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace lane_map::core
