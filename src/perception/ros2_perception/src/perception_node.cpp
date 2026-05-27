#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include <std_msgs/msg/string.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

class PerceptionNode : public rclcpp::Node {
public:
    PerceptionNode() : Node("perception_node") {
	gaze_pub_ = this->create_publisher<geometry_msgs::msg::Vector3>("/human/gaze", 10);
	gesture_pub_  = this->create_publisher<std_msgs::msg::String>("/human/gesture", 10);

	image_sub_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
	    "/human/camera/compressed", 10,
	    std::bind(&PerceptionNode::image_callback, this, std::placeholders::_1)
	);

        RCLCPP_INFO(this->get_logger(), "Perception node initialized");
    }

private:
    void image_callback(const sensor_msgs::msg::CompressedImage::SharedPtr msg) {}

    rclcpp::Publisher<geometry_msgs::msg::Vector3>::SharedPtr gaze_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr gesture_pub_;
    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr image_sub_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<PerceptionNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
