// DBlane filter for point cloud data
// formula 1 student version

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cmath>
#include <vector>
// ROS
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "visualization_msgs/msg/marker_array.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"
// PCL
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/filters/crop_box.h>

using namespace std::chrono_literals;
using std::placeholders::_1;

class Point
{
public:
  double x, y;
  int neighbor_pts = 0;
  bool core = false;
  int cluster_id = -1;
};

double distance(const Point &p1, const Point &p2)
{
  return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}

void find_neighbors(std::vector<Point> &points, double eps)
{
  for (Point &p : points)
  {
    for (const Point &q : points)
    {
      if (distance(p, q) <= eps)
      {
        p.neighbor_pts++;
      }
    }
    p.core = p.neighbor_pts >= 3;
  }
}

void find_clusters(std::vector<Point> &points, double eps)
{
  int actual_cluster_id = 0;
  for (int i = 0; i < 10; i++)
  {
    for (Point &p : points)
    {
      if (p.core && p.cluster_id == -1)
      {
        p.cluster_id = actual_cluster_id;
        break;
      }
    }
    for (Point &p : points)
    {
      if (p.cluster_id == actual_cluster_id)
      {
        for (Point &q : points)
        {
          if (q.core && q.cluster_id == -1 && distance(p, q) <= eps)
          {
            q.cluster_id = actual_cluster_id;
          }
        }
      }
    }
    actual_cluster_id++;
  }
  for (Point &p : points)
  {
    if (!p.core)
    {
      for (const Point &q : points)
      {
        if (q.core && q.cluster_id != -1 && distance(p, q) <= eps)
        {
          p.cluster_id = q.cluster_id;
        }
      }
    }
  }
}
class DblaneFormula : public rclcpp::Node
{
  rcl_interfaces::msg::SetParametersResult parametersCallback(const std::vector<rclcpp::Parameter> &parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    result.reason = "success";
    for (const auto &param : parameters)
    {
      RCLCPP_INFO_STREAM(this->get_logger(), "Param update: " << param.get_name().c_str() << ": " << param.value_to_string().c_str());
      if (param.get_name() == "minX")
      {
        minX = param.as_double();
      }
      if (param.get_name() == "minY")
      {
        minY = param.as_double();
      }
      if (param.get_name() == "minZ")
      {
        minZ = param.as_double();
      }
      if (param.get_name() == "maxX")
      {
        maxX = param.as_double();
      }
      if (param.get_name() == "maxY")
      {
        maxY = param.as_double();
      }
      if (param.get_name() == "maxZ")
      {
        maxZ = param.as_double();
      }
      if (param.get_name() == "verbose1")
      {
        verbose1 = param.as_bool();
      }
      if (param.get_name() == "verbose2")
      {
        verbose2 = param.as_bool();
      }
      if (param.get_name() == "search_start_width_x")
      {
        search_start_width_x = param.as_double();
      }
      if (param.get_name() == "search_start_width_y")
      {
        search_start_width_y = param.as_double();
      }
      if (param.get_name() == "points_in_topic")
      {
        points_in_topic = param.as_string();
        sub_lidar_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(points_in_topic, rclcpp::SensorDataQoS().keep_last(1), std::bind(&DblaneFormula::lidar_callback, this, std::placeholders::_1));
      }
      if (param.get_name() == "points_out_topic")
      {
        points_out_topic = param.as_string();
        pub_lidar_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(points_out_topic, 10);
      }
      if (param.get_name() == "marker_out_topic")
      {
        marker_out_topic = param.as_string();
        pub_marker_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(marker_out_topic, 10);
      }
    }
    return result;
  }

public:
  DblaneFormula() : Node("dblane_f1s"), count_(0)
  {
    this->declare_parameter<float>("minX", minX);
    this->declare_parameter<float>("minY", minY);
    this->declare_parameter<float>("minZ", minZ);
    this->declare_parameter<float>("maxX", maxX);
    this->declare_parameter<float>("maxY", maxY);
    this->declare_parameter<float>("maxZ", maxZ);
    this->declare_parameter<std::string>("points_in_topic", "/lexus3/os_center/points");
    this->declare_parameter<std::string>("points_out_topic", "clustered_points");
    this->declare_parameter<std::string>("marker_out_topic", "clustered_marker");
    this->declare_parameter<bool>("verbose1", verbose1);
    this->declare_parameter<bool>("verbose2", verbose2);
    this->declare_parameter<float>("search_start_width_x", search_start_width_x);
    this->declare_parameter<float>("search_start_width_y", search_start_width_y);
    this->get_parameter("minX", minX);
    this->get_parameter("minY", minY);
    this->get_parameter("minZ", minZ);
    this->get_parameter("maxX", maxX);
    this->get_parameter("maxY", maxY);
    this->get_parameter("maxZ", maxZ);
    this->get_parameter("points_in_topic", points_in_topic);
    this->get_parameter("points_out_topic", points_out_topic);
    this->get_parameter("marker_out_topic", marker_out_topic);
    this->get_parameter("verbose1", verbose1);
    this->get_parameter("verbose2", verbose2);
    this->get_parameter("search_start_width_x", search_start_width_x);
    this->get_parameter("search_start_width_y", search_start_width_y);

    pub_lidar_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(points_out_topic, 10);
    pub_marker_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(marker_out_topic, 10);
    // TODO: QoS // rclcpp::SensorDataQoS().keep_last(1)
    sub_lidar_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(points_in_topic, 10, std::bind(&DblaneFormula::lidar_callback, this, std::placeholders::_1));
    callback_handle_ = this->add_on_set_parameters_callback(std::bind(&DblaneFormula::parametersCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "DblaneFormula node has been started.");
    RCLCPP_INFO(this->get_logger(), "Subscribing to: '%s'", points_in_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "Publishing to: '%s' and '%s'", points_out_topic.c_str(), marker_out_topic.c_str());
  }

private:
  void lidar_callback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr input_msg)
  {
    // Convert to PCL data type
    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::fromROSMsg(*input_msg, *cloud);
    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZI>);
    int original_size = cloud->width * cloud->height;

    // Filter out points outside of the box
    pcl::CropBox<pcl::PointXYZI> crop;
    crop.setInputCloud(cloud);
    crop.setMin(Eigen::Vector4f(minX, minY, minZ, 1.0));
    crop.setMax(Eigen::Vector4f(maxX, maxY, maxZ, 1.0));
    crop.filter(*cloud);

    if (verbose1)
    {
      // print the length of the pointcloud
      RCLCPP_INFO_STREAM(this->get_logger(), "PointCloud in: " << original_size << " reduced size before cluster: " << cloud->width * cloud->height);
    }

    // DBSCAN
    // find neighbors in cloud
    std::vector<Point> points;


    for (const pcl::PointXYZI &p : cloud->points)
    {
        Point point;
        point.x = p.x;
        point.y = p.y;
        points.push_back(point);
    }

    find_neighbors(points, 3.5);
    // find clusters in cloud
    find_clusters(points, 3.5);

    // convert to PointXYZI
    for (const Point &p : points)
    {
      pcl::PointXYZI point;
      point.x = p.x;
      point.y = p.y;
      point.z = 0.0;
      point.intensity = p.cluster_id;
      cloud_filtered->points.push_back(point);
    }

    // Convert to ROS data type
    sensor_msgs::msg::PointCloud2 output_msg;
    pcl::toROSMsg(*cloud_filtered, output_msg);
    // Add the same frame_id as the input, it is not included in pcl PointXYZI
    output_msg.header.frame_id = input_msg->header.frame_id;
    // Publish the data as a ROS message
    pub_lidar_->publish(output_msg);


    // create marker array
    visualization_msgs::msg::MarkerArray mark_array;
    visualization_msgs::msg::Marker blue_left;
    blue_left.header.frame_id = input_msg->header.frame_id;
    blue_left.header.stamp = this->now();
    blue_left.ns = "search_start";
    blue_left.type = visualization_msgs::msg::Marker::CUBE;
    blue_left.action = visualization_msgs::msg::Marker::MODIFY;
    blue_left.scale.x = search_start_width_x;
    blue_left.scale.y = search_start_width_y;
    blue_left.scale.z = 0.2;
    blue_left.color.r = md_blue_500_r;
    blue_left.color.g = md_blue_500_g;
    blue_left.color.b = md_blue_500_b;
    blue_left.color.a = 0.8;
    blue_left.id = 0;
    blue_left.pose.position.x = 0.0;
    blue_left.pose.position.y = -0.5 * search_start_width_y - 0.1;
    blue_left.pose.position.z = 0.0;

    visualization_msgs::msg::Marker amber_right;
    amber_right.header.frame_id = input_msg->header.frame_id;
    amber_right.header.stamp = this->now();
    amber_right.ns = "search_start";
    amber_right.type = visualization_msgs::msg::Marker::CUBE;
    amber_right.action = visualization_msgs::msg::Marker::MODIFY;
    amber_right.scale.x = search_start_width_x;
    amber_right.scale.y = search_start_width_y;
    amber_right.scale.z = 0.2;
    amber_right.color.r = md_amber_500_r;
    amber_right.color.g = md_amber_500_g;
    amber_right.color.b = md_amber_500_b;
    amber_right.color.a = 0.8;
    amber_right.id = 1;
    amber_right.pose.position.x = 0.0;
    amber_right.pose.position.y = 0.5 * search_start_width_y + 0.1;
    amber_right.pose.position.z = 0.0;

    mark_array.markers.push_back(blue_left);
    mark_array.markers.push_back(amber_right);
    pub_marker_->publish(mark_array);
  }

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_lidar_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr pub_marker_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr sub_lidar_;
  OnSetParametersCallbackHandle::SharedPtr callback_handle_;
  float minX = -80.0, minY = -25.0, minZ = -2.0;
  float maxX = +80.0, maxY = +25.0, maxZ = -0.15;
  bool verbose1 = false, verbose2 = false;
  float search_start_width_x = 4.5, search_start_width_y = 2.5;
  std::string points_in_topic, points_out_topic, marker_out_topic;
  // colors from https://github.com/jkk-research/colors
  const float md_amber_500_r = 1.00, md_amber_500_g = 0.76, md_amber_500_b = 0.03;
  const float md_blue_500_r = 0.13, md_blue_500_g = 0.59, md_blue_500_b = 0.95;
  size_t count_;
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DblaneFormula>());
  rclcpp::shutdown();
  return 0;
}