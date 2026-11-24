// The same code on every real map we can find: the CARLA towns and campus.
// These maps are too big for the repo (> 1 MB), so the test looks for them on
// disk and skips if there are none (as on GitHub's CI). To choose the maps:
//   LANE_MAP_REAL_MAPS=/path/a.xodr:/path/b.xodr colcon test --packages-select lane_map

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "Log.hpp"
#include "OpenDriveMap.h"
#include "lane_map/core/build.hpp"
#include "lane_map/core/lookup.hpp"

using namespace lane_map::core;
namespace fs = std::filesystem;

namespace
{

std::vector<std::string> find_maps()
{
  std::vector<std::string> maps;
  if (const char* env = std::getenv("LANE_MAP_REAL_MAPS"))
  {
    std::string list = env;
    for (std::size_t start = 0, end; start <= list.size(); start = end + 1)
    {
      end = list.find(':', start);
      if (end == std::string::npos)
        end = list.size();
      if (end > start)
        maps.push_back(list.substr(start, end - start));
    }
    return maps;
  }

  // The defaults, as mounted in the Navigator container.
  const fs::path towns = "/carla/CarlaUE4/Content/Carla/Maps/OpenDrive";
  if (fs::is_directory(towns))
    for (const auto& entry : fs::directory_iterator(towns))
    {
      const std::string name = entry.path().filename().string();
      // Town01.xodr etc. The _Opt versions are the same roads.
      if (name.rfind("Town", 0) == 0 && name.find("_Opt") == std::string::npos && entry.path().extension() == ".xodr")
        maps.push_back(entry.path().string());
    }
  for (const char* extra : {"/carla/PythonAPI/carla_interface/data/town12.xodr", "/navigator/data/maps/campus.xodr"})
    if (fs::is_regular_file(extra))
      maps.push_back(extra);
  std::sort(maps.begin(), maps.end());
  return maps;
}

std::string name_of(const std::string& path)
{
  return fs::path(path).filename().string();
}

}  // namespace

TEST(RealMaps, EveryMapBuildsAndHangsTogether)
{
  const std::vector<std::string> maps = find_maps();
  if (maps.empty())
    GTEST_SKIP() << "No real maps found (set LANE_MAP_REAL_MAPS to choose some)";

  odr::log::set_level(odr::log::Level::Error);  // Town12's thousands of unsupported-shape warnings
  const Config config;

  std::printf("%-16s %6s %8s %9s %8s %9s %9s\n", "map", "lanes", "no next", "no prev", "lookup", "missed", "warnings");
  for (const std::string& path : maps)
  {
    SCOPED_TRACE(path);
    const odr::OpenDriveMap map = load_map_file(path);
    const LaneNetwork net = build_network(map, config);
    ASSERT_FALSE(net.lanes.empty());

    // Same map, same network.
    const LaneNetwork again = build_network(map, config);
    EXPECT_EQ(again.lanes.size(), net.lanes.size());

    int no_successor = 0, no_predecessor = 0, lookup_ok = 0, lookup_tried = 0;
    for (const auto& [id, lane] : net.lanes)
    {
      ASSERT_GE(lane.points.size(), 2u) << id;
      EXPECT_GT(lane.length, 0.0) << id;
      for (const LanePoint& p : lane.points)
      {
        ASSERT_TRUE(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.heading)) << id;
        ASSERT_GT(p.speed_limit, 0.0) << id;
      }
      // Every link joins up: this lane ends inside the next lane's start (within its width plus tolerance).
      for (const std::string& next_id : lane.successors)
      {
        const LanePoint& end = lane.points.back();
        const LanePoint& start = net.lanes.at(next_id).points.front();
        EXPECT_LE(std::hypot(start.x - end.x, start.y - end.y), 0.5 * start.width + 1.5 * config.connect_tolerance_m)
            << id << " -> " << next_id;
      }
      no_successor += lane.successors.empty();
      no_predecessor += lane.predecessors.empty();
    }

    // Lookup: the middle of every lane outside junctions (where lanes overlap)
    // should find that lane.
    const LaneIndex index(net);
    for (const auto& [id, lane] : net.lanes)
    {
      if (lane.in_junction() || lane.length < 1.0)
        continue;
      const LanePoint& mid = lane.points[lane.points.size() / 2];
      if (mid.width < 1.0)
        continue;  // a lane still opening up
      ++lookup_tried;
      lookup_ok += index.lane_at(mid.x, mid.y, mid.heading) == id;
    }
    EXPECT_GE(lookup_ok, 0.99 * lookup_tried);

    // Links the routing graph missed: a lane ending exactly where another starts,
    // pointing the same way, that isn't listed as its successor.
    int missed = 0;
    for (const auto& [id, lane] : net.lanes)
    {
      const LanePoint& end = lane.points.back();
      const auto ahead = index.lane_at(end.x + 0.2 * std::cos(end.heading), end.y + 0.2 * std::sin(end.heading), end.heading);
      if (ahead && *ahead != id && net.lanes.at(*ahead).points.front().s == 0 &&
          std::hypot(net.lanes.at(*ahead).points.front().x - end.x, net.lanes.at(*ahead).points.front().y - end.y) < 0.1 &&
          std::find(lane.successors.begin(), lane.successors.end(), *ahead) == lane.successors.end())
        ++missed;
    }

    std::printf("%-16s %6zu %8d %9d %4d/%-4d %9d %9zu\n", name_of(path).c_str(), net.lanes.size(), no_successor,
                no_predecessor, lookup_ok, lookup_tried, missed, net.warnings.size());
  }
}
