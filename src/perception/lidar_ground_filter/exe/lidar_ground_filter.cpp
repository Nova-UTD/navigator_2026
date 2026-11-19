/*
 * Package:   lidar_ground_filter
 * Filename:  lidar_ground_filter.cpp
 * Author:    Will Heitman
 * Email:     w at heit dot mn
 * Copyright: 2023, Nova UTD
 * License:   MIT License
 */

#include <iostream>
#include <memory> // std::make_shared
#include "rclcpp/rclcpp.hpp"
#include "lidar_ground_filter/LidarGroundFilterNode.hpp"

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  rclcpp::spin(std::make_shared<navigator::perception::LidarGroundFilterNode>());
  rclcpp::shutdown();
  return 0;
}
