#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include <std_msgs/msg/string.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

class PerceptionNode : public rclcpp::Node {
public:
    PerceptionNode() : Node("perception_node"), ort_env_(ORT_LOGGING_LEVEL_WARNING, "PerceptionNode") {
	Ort::SessionOptions session_options;
	session_options.SetIntraOpNumThreads(1);

	ort_session_ = std::make_unique<Ort::Session>(ort_env_, "/models/yolov8n-pose.onnx", session_options);

	gaze_pub_ = this->create_publisher<geometry_msgs::msg::Vector3>("/human/gaze", 10);
	gesture_pub_  = this->create_publisher<std_msgs::msg::String>("/human/gesture", 10);

	image_sub_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
	    "/human/camera/compressed", 10,
	    std::bind(&PerceptionNode::image_callback, this, std::placeholders::_1)
	);

        RCLCPP_INFO(this->get_logger(), "Perception node initialized");
    }

private:
    void image_callback(const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
        // decode compressed JPEG to OpenCV Mat
	cv::Mat frame = cv::imdecode(msg->data, cv::IMREAD_COLOR);
	if (frame.empty()) return;

	// resize Mat to match model input tensor size
	cv::Mat blob;
	constexpr double scale_factor { 1.0 / 255.0 };
	cv::dnn::blobFromImage(frame,
				blob,
				scale_factor,
				cv::Size(640,640), 
				cv::Scalar(), 
				true, 
				false);

	// bind OpenCV input tensors to ONNX input tensors
	std::vector<int64_t> input_shape = {1, 3, 640, 640};
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

	Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
	    memory_info,
	    reinterpret_cast<float*>(blob.data),
	    blob.total(),
	    input_shape.data(),
	    input_shape.size()
	);

	// run inference
	const char* input_names[] = {"images"};
	const char* output_names[] = {"output0"};

	auto output_tensors = ort_session_->Run(
	    Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
	
	// parse output 
	float* out_data = output_tensors.front().GetTensorMutableData<float>();

	// YOLOv8-pose # bounding boxes
	constexpr int NUM_ANCHORS = 8400;

	// YOLOv8-pose 56-channel output indicies
	constexpr int CH_CONFIDENCE = 4;
	constexpr int CH_NOSE_X = 5;
	constexpr int CH_NOSE_Y = 6;
	constexpr int CH_L_EYE_X = 8;
	constexpr int CH_L_EYE_Y = 9;
	constexpr int CH_R_EYE_X = 11;
	constexpr int CH_R_EYE_Y = 12;

	int best_idx {-1};
	float max_conf = 0.0f;

	// get highest confidence gaze vector
	for (int i = 0; i < NUM_ANCHORS; ++i) {
	    float conf = out_data[CH_CONFIDENCE * NUM_ANCHORS + i];
	    if (conf > max_conf) {
		max_conf = conf;
		best_idx = i;
	    }
	}
	
	geometry_msgs::msg::Vector3 gaze_msg;

	// if face found, calc vec
	if (best_idx >= 0 && max_conf > 0.5f) {
	    float nose_x = out_data[CH_NOSE_X * NUM_ANCHORS + best_idx];
	    float nose_y = out_data[CH_NOSE_Y * NUM_ANCHORS + best_idx];
	    float left_eye_x = out_data[CH_L_EYE_X * NUM_ANCHORS + best_idx];
	    float left_eye_y = out_data[CH_L_EYE_Y * NUM_ANCHORS + best_idx];
	    float right_eye_x = out_data[CH_R_EYE_X * NUM_ANCHORS + best_idx];
	    float right_eye_y = out_data[CH_R_EYE_Y * NUM_ANCHORS + best_idx];

	    // face orientation
	    float eye_center_x = (left_eye_x + right_eye_x) / 2.0f;
	    float eye_center_y = (left_eye_y + right_eye_y) / 2.0f;

	    gaze_msg.x = nose_x - eye_center_x;
	    gaze_msg.y = nose_y - eye_center_y;
	    gaze_msg.z = 1.0f;
	} else {
	    gaze_msg.x = 0.0f;
	    gaze_msg.y = 0.0f;
	    gaze_msg.z = 0.0f;
	}
	
	RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 500, 
        "Gaze -> X: %.2f, Y: %.2f, Z: %.2f", gaze_msg.x, gaze_msg.y, gaze_msg.z); //debug

	gaze_pub_->publish(gaze_msg);
}

    Ort::Env ort_env_;
    std::unique_ptr<Ort::Session> ort_session_;
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
