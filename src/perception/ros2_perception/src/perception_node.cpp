#include <arpa/inet.h>
#include <cmath>
#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <netdb.h>
#include <netinet/in.h>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <sstream>
#include <std_msgs/msg/string.hpp>
#include <sys/socket.h>
#include <unistd.h>

class PerceptionNode : public rclcpp::Node {
public:
  PerceptionNode()
      : Node("perception_node"),
        ort_env_(ORT_LOGGING_LEVEL_WARNING, "PerceptionNode") {
    Ort::SessionOptions session_options;
    session_options.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);
    ort_session_ = std::make_unique<Ort::Session>(
        ort_env_, "/models/yolov8n-pose.onnx", session_options);

    gaze_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>(
        "/human/gaze", 10);
    gesture_pub_ =
        this->create_publisher<std_msgs::msg::String>("/human/gesture", 10);
    image_sub_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
        "/human/camera/compressed", 10,
        std::bind(&PerceptionNode::image_callback, this,
                  std::placeholders::_1));

    udp_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    dest_addr_.sin_family = AF_INET;
    dest_addr_.sin_port = htons(9998);
    const char *bridge_host = getenv("BRIDGE_HOST");
    if (bridge_host == nullptr)
      bridge_host = "web_input_bridge";
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(bridge_host, nullptr, &hints, &res) == 0) {
      dest_addr_.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
      freeaddrinfo(res);
    } else {
      inet_pton(AF_INET, "127.0.0.1", &dest_addr_.sin_addr);
    }

    // OpenCV coordinate system: X-right, Y-down, Z-away
    model_pts_.push_back(cv::Point3d(0.0, 0.0, 0.0));
    model_pts_.push_back(cv::Point3d(35.0, -35.0, 25.0));
    model_pts_.push_back(cv::Point3d(-35.0, -35.0, 25.0));
    model_pts_.push_back(cv::Point3d(85.0, -25.0, 90.0));
    model_pts_.push_back(cv::Point3d(-85.0, -25.0, 90.0));

    RCLCPP_INFO(this->get_logger(), "Perception node initialized");
  }

  ~PerceptionNode() {
    if (udp_fd_ >= 0)
      close(udp_fd_);
  }

private:
  void image_callback(const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
    cv::Mat frame = cv::imdecode(msg->data, cv::IMREAD_COLOR);
    if (frame.empty())
      return;

    int orig_w = frame.cols;
    int orig_h = frame.rows;

    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(640, 640),
                           cv::Scalar(), true, false);

    std::vector<int64_t> input_shape = {1, 3, 640, 640};
    auto mem = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        mem, reinterpret_cast<float *>(blob.data), blob.total(),
        input_shape.data(), input_shape.size());

    const char *in_names[] = {"images"};
    const char *out_names[] = {"output0"};
    auto out = ort_session_->Run(Ort::RunOptions{nullptr}, in_names,
                                 &input_tensor, 1, out_names, 1);
    float *d = out.front().GetTensorMutableData<float>();

    constexpr int NA = 8400;
    auto kpx = [&](int kp, int a) { return d[(5 + kp * 3) * NA + a]; };
    auto kpy = [&](int kp, int a) { return d[(5 + kp * 3 + 1) * NA + a]; };
    auto kpv = [&](int kp, int a) { return d[(5 + kp * 3 + 2) * NA + a]; };

    int best = -1;
    float best_conf = 0.0f;
    for (int i = 0; i < NA; i++) {
      float c = d[4 * NA + i];
      if (c > best_conf) {
        best_conf = c;
        best = i;
      }
    }

    geometry_msgs::msg::PoseStamped gaze;
    gaze.header.stamp = this->now();
    gaze.header.frame_id = "camera_optical_frame";
    gaze.pose.orientation.w = 1.0;

    if (best >= 0 && best_conf > 0.5f) {
      float sx = (float)orig_w / 640.0f;
      float sy = (float)orig_h / 640.0f;

      std::vector<cv::Point2d> img_pts;
      std::vector<cv::Point3d> obj_pts;
      double kp_alpha = 0.2; // 2D point EMA for jitter reduction

      for (int k = 0; k < 5; k++) {
        if (kpv(k, best) < 0.3f) {
          kp_init_[k] = false;
          continue;
        }
        cv::Point2d raw_pt(kpx(k, best) * sx, kpy(k, best) * sy);
        if (!kp_init_[k]) {
          smoothed_kps_[k] = raw_pt;
          kp_init_[k] = true;
        } else {
          // Reset EMA if sudden large movement
          if (cv::norm(raw_pt - smoothed_kps_[k]) > 50.0) {
            smoothed_kps_[k] = raw_pt;
          } else {
            smoothed_kps_[k] =
                kp_alpha * raw_pt + (1.0 - kp_alpha) * smoothed_kps_[k];
          }
        }
        img_pts.push_back(smoothed_kps_[k]);
        obj_pts.push_back(model_pts_[k]);
      }

      if ((int)img_pts.size() >= 4) {
        double fl = (double)orig_w;
        cv::Mat cam = (cv::Mat_<double>(3, 3) << fl, 0, orig_w / 2.0, 0, fl,
                       orig_h / 2.0, 0, 0, 1);
        cv::Mat dist = cv::Mat::zeros(4, 1, CV_64F);
        cv::Mat rvec, tvec;
        bool solved = false;

        if (pose_initialized_) {
          rvec = smoothed_rvec_.clone();
          tvec = smoothed_tvec_.clone();
          solved = cv::solvePnP(obj_pts, img_pts, cam, dist, rvec, tvec, true,
                                cv::SOLVEPNP_ITERATIVE);
        }

        if (!solved) {
          solved = cv::solvePnP(obj_pts, img_pts, cam, dist, rvec, tvec, false,
                                cv::SOLVEPNP_EPNP);
        }

        if (solved) {
          if (!pose_initialized_) {
            smoothed_rvec_ = rvec.clone();
            smoothed_tvec_ = tvec.clone();
            pose_initialized_ = true;
          } else {
            double alpha = 0.3; // 3D pose EMA
            smoothed_rvec_ = alpha * rvec + (1.0 - alpha) * smoothed_rvec_;
            smoothed_tvec_ = alpha * tvec + (1.0 - alpha) * smoothed_tvec_;
          }

          std::vector<cv::Point3d> proj_pts;
          proj_pts.push_back(cv::Point3d(35, -35, 25));
          proj_pts.push_back(cv::Point3d(35, -35, -275)); // Left eye vector
          proj_pts.push_back(cv::Point3d(-35, -35, 25));
          proj_pts.push_back(cv::Point3d(-35, -35, -275)); // Right eye vector

          std::vector<cv::Point2d> proj2d;
          cv::projectPoints(proj_pts, smoothed_rvec_, smoothed_tvec_, cam, dist,
                            proj2d);

          cv::Mat rmat;
          cv::Rodrigues(smoothed_rvec_, rmat);
          double pitch = atan2(rmat.at<double>(2, 1), rmat.at<double>(2, 2));
          double yaw =
              atan2(-rmat.at<double>(2, 0),
                    sqrt(rmat.at<double>(2, 1) * rmat.at<double>(2, 1) +
                         rmat.at<double>(2, 2) * rmat.at<double>(2, 2)));
          double roll = atan2(rmat.at<double>(1, 0), rmat.at<double>(0, 0));

          std::ostringstream oss;
          oss << proj2d[0].x << "," << proj2d[0].y << "," << proj2d[1].x << ","
              << proj2d[1].y << "," << proj2d[2].x << "," << proj2d[2].y << ","
              << proj2d[3].x << "," << proj2d[3].y << ","
              << pitch * 180.0 / M_PI << "," << yaw * 180.0 / M_PI << ","
              << roll * 180.0 / M_PI;
          std::string payload = oss.str();
          sendto(udp_fd_, payload.c_str(), payload.size(), 0,
                 (struct sockaddr *)&dest_addr_, sizeof(dest_addr_));

          RCLCPP_INFO_THROTTLE(
              this->get_logger(), *this->get_clock(), 3000,
              "pose p=%.1f y=%.1f r=%.1f  rvec=[%.2f,%.2f,%.2f]  kps=%d",
              pitch * 180.0 / M_PI, yaw * 180.0 / M_PI, roll * 180.0 / M_PI,
              rvec.at<double>(0), rvec.at<double>(1), rvec.at<double>(2),
              (int)img_pts.size());

          gaze.pose.position.x = proj2d[0].x;
          gaze.pose.position.y = proj2d[0].y;
          gaze.pose.position.z = tvec.at<double>(2);
          double cy2 = cos(yaw * .5), sy2 = sin(yaw * .5);
          double cp = cos(pitch * .5), sp = sin(pitch * .5);
          double cr = cos(roll * .5), sr = sin(roll * .5);
          gaze.pose.orientation.w = cr * cp * cy2 + sr * sp * sy2;
          gaze.pose.orientation.x = sr * cp * cy2 - cr * sp * sy2;
          gaze.pose.orientation.y = cr * sp * cy2 + sr * cp * sy2;
          gaze.pose.orientation.z = cr * cp * sy2 - sr * sp * cy2;
        }
      }
    }
    gaze_pub_->publish(gaze);
  }

  Ort::Env ort_env_;
  std::unique_ptr<Ort::Session> ort_session_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr gaze_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr gesture_pub_;
  rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr image_sub_;
  int udp_fd_;
  struct sockaddr_in dest_addr_;
  std::vector<cv::Point3d> model_pts_;

  cv::Mat smoothed_rvec_;
  cv::Mat smoothed_tvec_;
  bool pose_initialized_ = false;

  std::vector<cv::Point2d> smoothed_kps_ =
      std::vector<cv::Point2d>(5, cv::Point2d(0, 0));
  std::vector<bool> kp_init_ = std::vector<bool>(5, false);
};

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PerceptionNode>());
  rclcpp::shutdown();
  return 0;
}
