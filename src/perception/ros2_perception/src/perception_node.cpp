#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

class PerceptionNode : public rclcpp::Node {
public:
    PerceptionNode() : Node("perception_node") {
        RCLCPP_INFO(this->get_logger(), "Perception node initialized");
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<PerceptionNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
