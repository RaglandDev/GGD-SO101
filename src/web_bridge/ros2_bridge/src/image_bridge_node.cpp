#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <thread>

class ImageBridgeNode : public rclcpp::Node {
public:
    ImageBridgeNode() : Node("web_input_bridge_node") {
	constexpr int qos_depth {10};
        publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>(
            "/human/camera/compressed", qos_depth);

        server_fd_ = socket(AF_INET, SOCK_DGRAM, 0); // udp 
        
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(9999); // Local IPC port

        bind(server_fd_, (struct sockaddr*)&address, sizeof(address));
        
        RCLCPP_INFO(this->get_logger(), "Bridge Node initialized on port :9999");
        
        worker_thread_ = std::thread(&ImageBridgeNode::listen_loop, this);
    }

    ~ImageBridgeNode() {
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        close(server_fd_);
    }

private:
    void listen_loop() {
        constexpr uint32_t max_udp_payload_size {65507};
        std::vector<uint8_t> buffer(max_udp_payload_size); 

        while (rclcpp::ok()) {
            ssize_t bytes_received = recv(server_fd_, buffer.data(), buffer.size(), 0);
            
            if (bytes_received > 0) {
                auto msg = sensor_msgs::msg::CompressedImage();
                msg.header.stamp = this->now();
                msg.header.frame_id = "camera_link";
                msg.format = "jpeg";
        
                msg.data.assign(buffer.begin(), buffer.begin() + bytes_received);

                publisher_->publish(msg);
		
		constexpr int interval_ms {2000};
		RCLCPP_INFO_THROTTLE(
                this->get_logger(), 
                *this->get_clock(), 
                interval_ms,
                "Streaming active. Receiving frames (Latest frame size: %ld bytes)", 
                bytes_received
            );
            }
        }
    }

    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_; 
    int server_fd_;
    std::thread worker_thread_; 
}; 

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv); 
    auto node = std::make_shared<ImageBridgeNode>(); 
    rclcpp::spin(node);
    rclcpp::shutdown(); 
    return 0;
}
