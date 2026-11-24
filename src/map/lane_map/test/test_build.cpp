// build_network on test/fixtures/test_map.xodr. The map is described at the top
// of that file; every expected value below can be worked out from it by hand.

#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "OpenDriveMap.h"
#include "lane_map/core/build.hpp"

using namespace lane_map::core;

namespace
{
const std::string kMapPath = std::string(FIXTURES_DIR) + "/test_map.xodr";
constexpr double kTol = 1e-6;
constexpr double kMph30 = 30 * 0.44704;  // m/s

class TestMap : public ::testing::Test
{
protected:
  Config config;
  LaneNetwork net = build_network(load_map_file(kMapPath), config);

  const Lane& lane(const std::string& id) { return net.lanes.at(id); }
};
}  // namespace

TEST_F(TestMap, OneLanePerDrivingLane)
{
  // Road 1: -1, +1 (the sidewalk -2 is left out). Road 2: -1, +1.
  // Road 3: -1 in section 0; -1 and the turn lane -2 in section 1. Road 4: -1.
  std::vector<std::string> ids;
  for (const auto& [id, lane] : net.lanes)
    ids.push_back(id);
  EXPECT_EQ(ids, (std::vector<std::string>{"1/0/-1", "1/0/1", "2/0/-1", "2/0/1", "3/0/-1", "3/1/-1", "3/1/-2",
                                           "4/0/-1"}));
  EXPECT_TRUE(net.warnings.empty());
}

TEST_F(TestMap, CentreLineAlongTheReferenceLine)
{
  // Lane -1 drives east, 1.75 m right of the reference line, points every 0.5 m.
  const Lane& l = lane("1/0/-1");
  ASSERT_EQ(l.points.size(), 201u);
  EXPECT_NEAR(l.points.front().x, 0.0, kTol);
  EXPECT_NEAR(l.points.front().y, -1.75, kTol);
  EXPECT_NEAR(l.points.back().x, 100.0, kTol);
  EXPECT_NEAR(l.points[1].s, 0.5, kTol);
  EXPECT_NEAR(l.length, 100.0, kTol);
  for (const LanePoint& p : l.points)
  {
    EXPECT_NEAR(p.heading, 0.0, kTol);
    EXPECT_NEAR(p.width, 3.5, kTol);
  }
}

TEST_F(TestMap, LanesOnTheLeftAreReversed)
{
  // Lane +1 drives west, against the reference line: its points start at x = 100.
  const Lane& l = lane("1/0/1");
  EXPECT_NEAR(l.points.front().x, 100.0, kTol);
  EXPECT_NEAR(l.points.front().y, 1.75, kTol);
  EXPECT_NEAR(l.points.back().x, 0.0, kTol);
  EXPECT_NEAR(std::abs(l.points.front().heading), M_PI, kTol);
  EXPECT_NEAR(l.points.front().s, 0.0, kTol);
  EXPECT_NEAR(l.points.back().s, 100.0, kTol);
}

TEST_F(TestMap, CurvedLaneLength)
{
  // Road 3 is an arc of radius 50 turning left. Lane -1's centre is 1.75 m
  // outside it, so section 0 (40 m of reference line, 0.8 rad) is 51.75 * 0.8 m.
  EXPECT_NEAR(lane("3/0/-1").length, 51.75 * 0.8, 1e-2);
  EXPECT_NEAR(lane("3/0/-1").points.front().heading, 0.0, 1e-2);
  EXPECT_NEAR(lane("3/0/-1").points.back().heading, 0.8, 1e-2);
}

TEST_F(TestMap, SpeedLimits)
{
  EXPECT_NEAR(lane("1/0/-1").points[10].speed_limit, kMph30, kTol);   // 30 mph
  EXPECT_NEAR(lane("2/0/-1").points[10].speed_limit, config.default_speed_limit_mps, kTol);  // none in the map
}

TEST_F(TestMap, RoadMarksLookingAlongTheLane)
{
  // Both directions have the solid centre line on their left and a solid line on their right.
  for (const std::string id : {"1/0/-1", "1/0/1"})
  {
    EXPECT_EQ(lane(id).points[10].left_mark, RoadMark::Solid) << id;
    EXPECT_EQ(lane(id).points[10].right_mark, RoadMark::Solid) << id;
    // The centre line is yellow in the map; the outer lines are "standard" (white).
    EXPECT_EQ(lane(id).points[10].left_color, MarkColor::Yellow) << id;
    EXPECT_EQ(lane(id).points[10].right_color, MarkColor::White) << id;
  }
  EXPECT_EQ(lane("2/0/-1").points[10].left_mark, RoadMark::None);  // road 2 has no marks
}

TEST_F(TestMap, Neighbours)
{
  // Road 1: the two lanes are each other's left neighbour, and oncoming.
  ASSERT_TRUE(lane("1/0/-1").left.has_value());
  EXPECT_EQ(lane("1/0/-1").left->lane_id, "1/0/1");
  EXPECT_FALSE(lane("1/0/-1").left->same_direction);
  EXPECT_EQ(lane("1/0/1").left->lane_id, "1/0/-1");
  // The sidewalk isn't a driving lane, so lane -1 has nothing on its right.
  EXPECT_FALSE(lane("1/0/-1").right.has_value());

  // Road 3, section 1: the turn lane is on the right, same direction.
  ASSERT_TRUE(lane("3/1/-1").right.has_value());
  EXPECT_EQ(lane("3/1/-1").right->lane_id, "3/1/-2");
  EXPECT_TRUE(lane("3/1/-1").right->same_direction);
  EXPECT_EQ(lane("3/1/-2").left->lane_id, "3/1/-1");
  EXPECT_FALSE(lane("3/1/-1").left.has_value());  // one-way road
}

TEST_F(TestMap, SuccessorsAndPredecessors)
{
  using Ids = std::vector<std::string>;
  EXPECT_EQ(lane("1/0/-1").successors, Ids{"2/0/-1"});  // east onto road 2
  EXPECT_EQ(lane("2/0/1").successors, Ids{"1/0/1"});    // west onto road 1
  EXPECT_EQ(lane("3/0/-1").successors, Ids{"3/1/-1"});  // across the section boundary
  EXPECT_TRUE(lane("1/0/1").successors.empty());        // drives off the map
  EXPECT_TRUE(lane("2/0/-1").successors.empty());

  EXPECT_EQ(lane("2/0/-1").predecessors, Ids{"1/0/-1"});
  EXPECT_EQ(lane("1/0/1").predecessors, Ids{"2/0/1"});
  EXPECT_TRUE(lane("3/1/-2").predecessors.empty());  // the turn lane just appears
}

TEST_F(TestMap, LinksMissingFromTheFileAreFoundFromTheGeometry)
{
  // The file never links road 3 to road 4, but lane -1 runs straight on.
  EXPECT_EQ(lane("3/1/-1").successors, std::vector<std::string>{"4/0/-1"});
  // The turn lane ends 3.25 m further right, where road 4 has no lane.
  EXPECT_TRUE(lane("3/1/-2").successors.empty());
}

TEST_F(TestMap, SameMapSameNetwork)
{
  const LaneNetwork again = build_network(load_map_file(kMapPath), config);
  ASSERT_EQ(again.lanes.size(), net.lanes.size());
  for (const auto& [id, l] : net.lanes)
  {
    const Lane& other = again.lanes.at(id);
    ASSERT_EQ(other.points.size(), l.points.size());
    EXPECT_EQ(other.successors, l.successors);
    EXPECT_DOUBLE_EQ(other.points.back().x, l.points.back().x);
  }
}

TEST(Loading, FromTextGivesTheSameNetwork)
{
  std::ifstream file(kMapPath);
  std::stringstream text;
  text << file.rdbuf();
  const LaneNetwork from_text = build_network(load_map_string(text.str()), Config{});
  const LaneNetwork from_file = build_network(load_map_file(kMapPath), Config{});
  EXPECT_EQ(from_text.lanes.size(), from_file.lanes.size());
}

TEST(Loading, ClearErrors)
{
  EXPECT_THROW(load_map_file("/does/not/exist.xodr"), std::runtime_error);
  EXPECT_THROW(load_map_string(""), std::runtime_error);
  EXPECT_THROW(load_map_string("<not opendrive/>"), std::runtime_error);
  EXPECT_THROW(load_map_string("this isn't even XML"), std::runtime_error);
}
