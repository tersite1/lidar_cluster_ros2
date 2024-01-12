#ifndef MARKER_HPP_
#define MARKER_HPP_

#include "rclcpp/rclcpp.hpp"
#include "visualization_msgs/msg/marker_array.hpp"
#include "lidar_cluster/material_color.hpp"



void init_debug_marker(visualization_msgs::msg::Marker& debug_marker, double pose_x, double pose_y){
    debug_marker.ns = "debug1";
    debug_marker.type = visualization_msgs::msg::Marker::CYLINDER;
    debug_marker.action = visualization_msgs::msg::Marker::MODIFY;
    debug_marker.scale.x = 0.4;
    debug_marker.scale.y = 0.4;
    debug_marker.scale.z = 0.4;
    std_msgs::msg::ColorRGBA c1 = material_color::hex_to_ros(material_color::md_pink_500);
    debug_marker.color.r = c1.r;
    debug_marker.color.g = c1.g;
    debug_marker.color.b = c1.b;
    debug_marker.color.a = 1.0;
    debug_marker.id = 3;
    debug_marker.pose.position.x = pose_x;
    debug_marker.pose.position.y = pose_y;
    debug_marker.pose.position.z = 0.0;
    // debug_marker.points.clear();
}


#endif // MARKER_HPP_