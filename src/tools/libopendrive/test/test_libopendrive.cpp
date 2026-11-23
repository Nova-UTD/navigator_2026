// Checks that libOpenDRIVE builds, links and gives the answers lane_map will
// rely on. Every expected value comes from test/fixtures/test_map.xodr and can
// be worked out by hand from the comment at the top of that file.

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "Lane.h"
#include "LaneSection.h"
#include "OpenDriveMap.h"
#include "Road.h"
#include "RoutingGraph.h"

namespace
{
const std::string kMap = std::string(FIXTURES_DIR) + "/test_map.xodr";
constexpr double kTol = 1e-6;  // metres

bool contains(const std::vector<odr::LaneKey>& keys, const odr::LaneKey& key)
{
  return std::find(keys.begin(), keys.end(), key) != keys.end();
}

class TestMap : public ::testing::Test
{
protected:
  odr::OpenDriveMap map{kMap};
};
}  // namespace

TEST_F(TestMap, LoadsEveryRoad)
{
  ASSERT_EQ(map.id_to_road.size(), 3u);
  EXPECT_EQ(map.id_to_road.count("1"), 1u);
  EXPECT_EQ(map.id_to_road.count("2"), 1u);
  EXPECT_EQ(map.id_to_road.count("3"), 1u);
}

// A bad path does not throw: the library logs an error and returns an empty
// map. lane_map must check for "no roads" itself and fail with a clear message.
TEST(LoadErrors, MissingFileGivesEmptyMapNotException)
{
  odr::OpenDriveMap map{"/does/not/exist.xodr"};
  EXPECT_TRUE(map.id_to_road.empty());
}

TEST_F(TestMap, PointOnStraightRoad)
{
  // 50 m along road 1, 1.75 m to the right (the middle of lane -1).
  const odr::Vec3D p = map.id_to_road.at("1").get_xyz(50.0, -1.75, 0.0);
  EXPECT_NEAR(p[0], 50.0, kTol);
  EXPECT_NEAR(p[1], -1.75, kTol);
  EXPECT_NEAR(p[2], 0.0, kTol);
}

TEST_F(TestMap, PointOnCurvedRoad)
{
  // The end of the quarter circle: radius 50 from (0,100) heading east.
  const odr::Road& road = map.id_to_road.at("3");
  const odr::Vec3D p = road.get_xyz(road.length, 0.0, 0.0);
  EXPECT_NEAR(p[0], 50.0, 1e-3);
  EXPECT_NEAR(p[1], 150.0, 1e-3);
}

TEST_F(TestMap, LaneTypesAndWidths)
{
  const odr::LaneSection& sec = map.id_to_road.at("1").s_to_lanesection.at(0.0);
  EXPECT_EQ(sec.id_to_lane.at(-1).type, "driving");
  EXPECT_EQ(sec.id_to_lane.at(1).type, "driving");
  EXPECT_EQ(sec.id_to_lane.at(-2).type, "sidewalk");

  // Each lane stores its width and its outer edge, measured sideways from the
  // reference line (negative = right). Its inner edge is the next lane in's
  // outer edge, so lane_map gets the centre from outer edge and width.
  const odr::Lane& right = sec.id_to_lane.at(-1);
  EXPECT_NEAR(right.lane_width.evaluate(50.0), 3.5, kTol);
  EXPECT_NEAR(right.outer_border.evaluate(50.0), -3.5, kTol);
  const odr::Lane& sidewalk = sec.id_to_lane.at(-2);
  EXPECT_NEAR(sidewalk.outer_border.evaluate(50.0), -5.5, kTol);
}

TEST_F(TestMap, SpeedLimit)
{
  const odr::Road& road = map.id_to_road.at("1");
  ASSERT_EQ(road.s_to_speed.count(0.0), 1u);
  EXPECT_EQ(road.s_to_speed.at(0.0).max, "30");
  EXPECT_EQ(road.s_to_speed.at(0.0).unit, "mph");

  // Road 2 has no speed limit: lane_map will use its default instead.
  EXPECT_TRUE(map.id_to_road.at("2").s_to_speed.empty());
}

TEST_F(TestMap, RoadMarks)
{
  const odr::Lane& centre = map.id_to_road.at("1").s_to_lanesection.at(0.0).id_to_lane.at(0);
  const std::vector<odr::RoadMark> marks = centre.get_roadmarks(0.0, 100.0);
  ASSERT_FALSE(marks.empty());
  EXPECT_EQ(marks.front().type, "solid");
}

TEST_F(TestMap, SuccessorsFollowTheDrivingDirection)
{
  const odr::RoutingGraph graph = map.get_routing_graph();
  const odr::LaneKey r1_right("1", 0.0, -1), r1_left("1", 0.0, 1);
  const odr::LaneKey r2_right("2", 0.0, -1), r2_left("2", 0.0, 1);

  // Eastbound: road 1 lane -1 leads into road 2 lane -1.
  const auto east = graph.get_lane_successors(r1_right);
  ASSERT_EQ(east.size(), 1u);
  EXPECT_TRUE(contains(east, r2_right));

  // Westbound: the +1 lanes drive against the reference line, so road 2 lane +1
  // leads back into road 1 lane +1, even though the file says it the other way.
  const auto west = graph.get_lane_successors(r2_left);
  ASSERT_EQ(west.size(), 1u);
  EXPECT_TRUE(contains(west, r1_left));

  // Road 1 lane +1 drives west off the start of the map: a dead end.
  EXPECT_TRUE(graph.get_lane_successors(r1_left).empty());
}

// Each edge's weight is the length of the lane it starts from. Older
// libOpenDRIVE (the copy we replaced) took the length from the wrong road for
// lanes that drive against the reference line, so this edge came out as 100.
TEST_F(TestMap, EdgeWeightIsTheStartingLanesLength)
{
  const odr::RoutingGraph graph = map.get_routing_graph();
  bool found_east = false, found_west = false;
  for (const odr::RoutingGraphEdge& e : graph.edges)
  {
    if (e.from == odr::LaneKey("1", 0.0, -1) && e.to == odr::LaneKey("2", 0.0, -1))
    {
      EXPECT_NEAR(e.weight, 100.0, kTol);  // road 1 is 100 m
      found_east = true;
    }
    if (e.from == odr::LaneKey("2", 0.0, 1) && e.to == odr::LaneKey("1", 0.0, 1))
    {
      EXPECT_NEAR(e.weight, 50.0, kTol);  // road 2 is 50 m
      found_west = true;
    }
  }
  EXPECT_TRUE(found_east);
  EXPECT_TRUE(found_west);
}

TEST_F(TestMap, NewLaneSectionWhereALaneAppears)
{
  const odr::Road& road = map.id_to_road.at("3");
  ASSERT_EQ(road.s_to_lanesection.size(), 2u);
  EXPECT_EQ(road.s_to_lanesection.at(0.0).id_to_lane.count(-2), 0u);   // not there yet
  EXPECT_EQ(road.s_to_lanesection.at(40.0).id_to_lane.count(-2), 1u);  // the turn lane

  // Lane -1 carries on across the section boundary.
  const auto next = map.get_routing_graph().get_lane_successors(odr::LaneKey("3", 0.0, -1));
  EXPECT_TRUE(contains(next, odr::LaneKey("3", 40.0, -1)));
}
