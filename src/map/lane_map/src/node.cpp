// The ROS wrapper around lane_map::core. It makes no decisions: it reads
// parameters, stores the newest map message, and on a timer loads the map,
// builds the lane network and publishes the drawing.

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <string>

#include <carla_msgs/msg/carla_world_info.hpp>
#include <diagnostic_msgs/msg/diagnostic_status.hpp>
#include <rclcpp/rclcpp.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include "OpenDriveMap.h"
#include "lane_map/core/build.hpp"
#include "lane_map/viz.hpp"

namespace lane_map
{

class LaneMapNode : public rclcpp::Node
{
public:
  LaneMapNode() : Node("lane_map")
  {
    map_source_ = declare_parameter<std::string>("map_source", "carla");
    map_file_ = declare_parameter<std::string>("map_file", "");
    frame_id_ = declare_parameter<std::string>("frame_id", "map");
    viz_point_step_ = declare_parameter<int>("viz_point_step", 2);
    const double viz_republish_period_s = declare_parameter<double>("viz_republish_period_s", 5.0);
    const double check_period_s = declare_parameter<double>("check_period_s", 0.1);

    config_.sample_spacing_m = declare_parameter<double>("sample_spacing_m", config_.sample_spacing_m);
    config_.default_speed_limit_mps = declare_parameter<double>("default_speed_limit_mps", config_.default_speed_limit_mps);
    config_.min_lane_width_m = declare_parameter<double>("min_lane_width_m", config_.min_lane_width_m);
    config_.connect_tolerance_m = declare_parameter<double>("connect_tolerance_m", config_.connect_tolerance_m);
    config_.connect_heading_tolerance_rad = declare_parameter<double>("connect_heading_tolerance_rad", config_.connect_heading_tolerance_rad);
    config_.connect_height_tolerance_m = declare_parameter<double>("connect_height_tolerance_m", config_.connect_height_tolerance_m);

    if (map_source_ != "carla" && map_source_ != "file")
      throw std::invalid_argument("map_source must be 'carla' or 'file', not '" + map_source_ + "'");
    if (map_source_ == "file" && map_file_.empty())
      throw std::invalid_argument("map_source is 'file' but map_file is empty");

    if (map_source_ == "carla")
    {
      // Two publishers send the map, with different QoS, and a subscription
      // only connects to publishers whose QoS it's compatible with:
      //  - carla_ros_bridge latches it (transient local): a transient-local
      //    subscription gets the stored copy the moment it connects.
      //  - carla_interface's route_reader re-sends it every 5 s (volatile),
      //    which a transient-local subscription can't receive.
      // Subscribing both ways gets the map immediately from the bridge, and
      // still works when only the re-sender is running.
      const auto store = [this](carla_msgs::msg::CarlaWorldInfo::SharedPtr msg) { latest_world_info_ = msg; };
      // Not being able to receive from the volatile re-sender is expected
      // (that's what the second subscription is for), so don't warn about it.
      rclcpp::SubscriptionOptions latched_options;
      latched_options.event_callbacks.incompatible_qos_callback = [](rclcpp::QOSRequestedIncompatibleQoSInfo&) {};
      world_info_latched_sub_ = create_subscription<carla_msgs::msg::CarlaWorldInfo>(
          "/carla/world_info", rclcpp::QoS(1).transient_local(), store, latched_options);
      world_info_sub_ = create_subscription<carla_msgs::msg::CarlaWorldInfo>("/carla/world_info", rclcpp::QoS(1), store);
    }

    // Latched: RViz gets the lanes whenever it starts.
    lanes_viz_pub_ = create_publisher<visualization_msgs::msg::MarkerArray>("/map/lanes/viz",
                                                                            rclcpp::QoS(1).transient_local());
    heartbeat_pub_ = create_publisher<diagnostic_msgs::msg::DiagnosticStatus>("/system/heartbeat", 10);

    tick_timer_ = create_wall_timer(std::chrono::duration<double>(check_period_s), [this] { tick(); });
    heartbeat_timer_ = create_wall_timer(std::chrono::seconds(1), [this] { heartbeat(); });
    if (viz_republish_period_s > 0)
      viz_timer_ = create_wall_timer(std::chrono::duration<double>(viz_republish_period_s), [this] { republish_viz(); });
    status_ = map_source_ == "carla" ? "Waiting for the map on /carla/world_info" : "Loading " + map_file_;
    tick();
  }

private:
  void tick()
  {
    if (map_source_ == "file")
    {
      if (!loaded_name_)
        load([&] { return core::load_map_file(map_file_); }, map_file_);
      return;
    }

    if (!latest_world_info_ || latest_world_info_->opendrive.empty())
      return;
    // Rebuild only when the map changes (CARLA loaded a different town).
    const std::string& name = latest_world_info_->map_name;
    const std::size_t size = latest_world_info_->opendrive.size();
    if (loaded_name_ == name && loaded_size_ == size)
      return;
    const auto msg = latest_world_info_;
    load([&] { return core::load_map_string(msg->opendrive); }, name);
    loaded_size_ = size;  // even on failure, so a bad map isn't retried every tick
  }

  template <typename LoadFn>
  void load(LoadFn load_fn, const std::string& name)
  {
    const auto start = std::chrono::steady_clock::now();
    loaded_name_ = name;
    try
    {
      const odr::OpenDriveMap map = load_fn();
      network_ = core::build_network(map, config_);
    }
    catch (const std::exception& e)
    {
      network_.reset();
      // Don't leave the previous map's lanes on screen.
      lanes_viz_.markers.clear();
      visualization_msgs::msg::Marker clear;
      clear.action = visualization_msgs::msg::Marker::DELETEALL;
      visualization_msgs::msg::MarkerArray clear_all;
      clear_all.markers.push_back(clear);
      lanes_viz_pub_->publish(clear_all);
      status_ = "Could not load map " + name + ": " + e.what();
      RCLCPP_ERROR(get_logger(), "%s", status_.c_str());
      return;
    }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    int in_junction = 0, no_successor = 0;
    for (const auto& [id, lane] : network_->lanes)
    {
      in_junction += lane.in_junction();
      no_successor += lane.successors.empty();
    }
    status_ = "Loaded " + name + ": " + std::to_string(network_->lanes.size()) + " lanes";
    RCLCPP_INFO(get_logger(), "%s (%d in junctions, %d with no successor) in %.0f ms", status_.c_str(), in_junction,
                no_successor, ms);
    for (std::size_t i = 0; i < network_->warnings.size() && i < 20; ++i)
      RCLCPP_WARN(get_logger(), "%s", network_->warnings[i].c_str());
    if (network_->warnings.size() > 20)
      RCLCPP_WARN(get_logger(), "... and %zu more warnings", network_->warnings.size() - 20);

    // Stamp 0 = "draw with the latest transform". The lanes never move, and a
    // real stamp breaks RViz when its clock and the transforms' clock differ
    // (wall time vs CARLA's simulation time): it looks for a transform at a
    // time that doesn't exist and silently draws nothing.
    lanes_viz_ = lane_markers(*network_, frame_id_, builtin_interfaces::msg::Time(), viz_point_step_);
    lanes_viz_pub_->publish(lanes_viz_);
  }

  // RViz throws away markers it can't place yet: if they arrive before it has
  // the map -> base_link transform (just after everything starts together), it
  // drops them and never asks again. So the drawing is sent again every few
  // seconds, from the copy already built, and only while someone is listening.
  void republish_viz()
  {
    if (!lanes_viz_.markers.empty() && lanes_viz_pub_->get_subscription_count() > 0)
      lanes_viz_pub_->publish(lanes_viz_);
  }

  void heartbeat()
  {
    diagnostic_msgs::msg::DiagnosticStatus msg;
    msg.name = get_name();
    msg.hardware_id = "lane_map";
    msg.message = status_;
    if (network_)
      msg.level = diagnostic_msgs::msg::DiagnosticStatus::OK;
    else if (loaded_name_)
      msg.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;  // tried and failed
    else
      msg.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;  // still waiting
    heartbeat_pub_->publish(msg);
  }

  // Parameters
  std::string map_source_, map_file_, frame_id_;
  int viz_point_step_ = 2;
  core::Config config_;

  // Newest input
  carla_msgs::msg::CarlaWorldInfo::SharedPtr latest_world_info_;

  // State
  std::optional<std::string> loaded_name_;
  std::size_t loaded_size_ = 0;
  std::optional<core::LaneNetwork> network_;
  visualization_msgs::msg::MarkerArray lanes_viz_;  // built once per map, re-sent by republish_viz
  std::string status_;

  rclcpp::Subscription<carla_msgs::msg::CarlaWorldInfo>::SharedPtr world_info_latched_sub_, world_info_sub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr lanes_viz_pub_;
  rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticStatus>::SharedPtr heartbeat_pub_;
  rclcpp::TimerBase::SharedPtr tick_timer_, heartbeat_timer_, viz_timer_;
};

}  // namespace lane_map

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  try
  {
    rclcpp::spin(std::make_shared<lane_map::LaneMapNode>());
  }
  catch (const std::exception& e)
  {
    RCLCPP_FATAL(rclcpp::get_logger("lane_map"), "%s", e.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
