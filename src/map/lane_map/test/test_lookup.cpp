// LaneIndex::lane_at on test/fixtures/test_map.xodr.

#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "OpenDriveMap.h"
#include "lane_map/core/build.hpp"
#include "lane_map/core/lookup.hpp"

using namespace lane_map::core;

namespace
{
class Lookup : public ::testing::Test
{
protected:
  LaneNetwork net = build_network(load_map_file(std::string(FIXTURES_DIR) + "/test_map.xodr"), Config{});
  LaneIndex index{net};
};
}  // namespace

TEST_F(Lookup, PointInALane)
{
  EXPECT_EQ(index.lane_at(50.0, -1.75, 0.0), "1/0/-1");    // eastbound lane, facing east
  EXPECT_EQ(index.lane_at(50.0, 1.75, M_PI), "1/0/1");     // westbound lane, facing west
  EXPECT_EQ(index.lane_at(120.0, -1.0, 0.0), "2/0/-1");    // on road 2
}

TEST_F(Lookup, FacingTheWrongWay)
{
  // In the westbound lane but facing east: we're not driving in that lane.
  EXPECT_EQ(index.lane_at(50.0, 1.75, 0.0), std::nullopt);
  EXPECT_EQ(index.lane_at(50.0, -1.75, M_PI), std::nullopt);
}

TEST_F(Lookup, UpToNinetyDegreesOff)
{
  EXPECT_EQ(index.lane_at(50.0, -1.75, 1.0), "1/0/-1");   // 57 degrees off: still this lane
  EXPECT_EQ(index.lane_at(50.0, -1.75, -1.5), "1/0/-1");  // 86 degrees off
}

TEST_F(Lookup, NotInAnyLane)
{
  EXPECT_EQ(index.lane_at(50.0, -4.5, 0.0), std::nullopt);  // on the sidewalk
  EXPECT_EQ(index.lane_at(50.0, 30.0, 0.0), std::nullopt);  // off the road
}

TEST_F(Lookup, OnACurve)
{
  // Road 3, lane -1, near the end of the arc: radius 51.75 around (0, 150), at 1.3 rad.
  const double angle = 1.3;
  const double x = 51.75 * std::sin(angle);
  const double y = 150.0 - 51.75 * std::cos(angle);
  EXPECT_EQ(index.lane_at(x, y, angle), "3/1/-1");
}
