cmake_minimum_required(VERSION 3.14)
project(lane_map)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

find_package(ament_cmake REQUIRED)
find_package(OpenDrive REQUIRED)
find_package(Boost REQUIRED)

# The logic. No ROS here: the tests link only this, so if anything in core/
# needed ROS, the tests wouldn't build.
add_library(lane_map_core
  src/core/build.cpp
  src/core/lookup.cpp
)
target_include_directories(lane_map_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(lane_map_core PUBLIC OpenDrive::OpenDrive Boost::boost)

# The ROS wrapper.
find_package(rclcpp REQUIRED)
find_package(carla_msgs REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)

add_executable(lane_map_node src/node.cpp src/viz.cpp)
target_link_libraries(lane_map_node lane_map_core)
ament_target_dependencies(lane_map_node rclcpp carla_msgs diagnostic_msgs visualization_msgs)

install(TARGETS lane_map_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_build test/test_build.cpp)
  target_link_libraries(test_build lane_map_core)
  ament_add_gtest(test_lookup test/test_lookup.cpp)
  target_link_libraries(test_lookup lane_map_core)
  ament_add_gtest(test_real_maps test/test_real_maps.cpp TIMEOUT 300)
  target_link_libraries(test_real_maps lane_map_core)
  foreach(test test_build test_lookup test_real_maps)
    target_compile_definitions(${test} PRIVATE FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/fixtures")
  endforeach()
endif()

ament_package()
