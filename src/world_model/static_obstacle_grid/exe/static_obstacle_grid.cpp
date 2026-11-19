/*
 * Package:   static_obstacle_grid
 * Filename:  static_obstacle_grid.cpp
 * Author:    Will Heitman
 * Email:     w at heit dot mn
 * Copyright: 2023, Nova UTD
 * License:   MIT License
 */

#include <iostream>
#include <memory> // std::make_shared
#include "rclcpp/rclcpp.hpp"
#include "static_obstacle_grid/StaticObstacleGridNode.hpp"

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  rclcpp::spin(std::make_shared<navigator::perception::StaticObstacleGridNode>());
  rclcpp::shutdown();
  return 0;
}
