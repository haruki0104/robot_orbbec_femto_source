#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "cv_bridge/cv_bridge.h"
#include <opencv2/opencv.hpp>
#include <libobsensor/ObSensor.hpp>

using namespace std::chrono_literals;

class FemtoMegaNode : public rclcpp::Node {
public:
    FemtoMegaNode() : Node("femto_mega_node"), pipe_() {
        // 0. Declare and get parameters
        this->declare_parameter<std::string>("camera_ip", "");
        std::string ip = this->get_parameter("camera_ip").as_string();

        // 1. Initialize Orbbec SDK
        ob::Context ctx;
        std::shared_ptr<ob::Device> dev;

        if (!ip.empty()) {
            RCLCPP_INFO(this->get_logger(), "Connecting to network device at %s...", ip.c_str());
            dev = ctx.createNetDevice(ip.c_str(), 8090);
        } else {
            auto devList = ctx.queryDeviceList();
            if (devList->deviceCount() == 0) {
                RCLCPP_ERROR(this->get_logger(), "No USB Orbbec device found!");
                throw std::runtime_error("No device");
            }
            dev = devList->getDevice(0);
        }

        if (!dev) {
            RCLCPP_ERROR(this->get_logger(), "Failed to connect to device!");
            throw std::runtime_error("Connection failed");
        }

        pipe_ = std::make_unique<ob::Pipeline>(dev);

        // 2. Configure Streams
        auto config = std::make_shared<ob::Config>();
        auto colorProfiles = pipe_->getStreamProfileList(OB_SENSOR_COLOR);
        std::shared_ptr<ob::VideoStreamProfile> profile_color;
        if (colorProfiles) {
            profile_color = colorProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
            config->enableStream(profile_color);
            RCLCPP_INFO(this->get_logger(), "Color enabled: %dx%d", profile_color->width(), profile_color->height());
        }

        auto depthProfiles = pipe_->getStreamProfileList(OB_SENSOR_DEPTH);
        std::shared_ptr<ob::VideoStreamProfile> profile_depth;
        if (depthProfiles) {
            profile_depth = depthProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
            config->enableStream(profile_depth);
            RCLCPP_INFO(this->get_logger(), "Depth enabled: %dx%d", profile_depth->width(), profile_depth->height());
        }

        // 2.1 Enable IMU
        try {
            auto accelProfiles = pipe_->getStreamProfileList(OB_SENSOR_ACCEL);
            auto gyroProfiles = pipe_->getStreamProfileList(OB_SENSOR_GYRO);
            if (accelProfiles && gyroProfiles) {
                config->enableStream(accelProfiles->getProfile(0));
                config->enableStream(gyroProfiles->getProfile(0));
                RCLCPP_INFO(this->get_logger(), "IMU (Accel/Gyro) enabled.");
            }
        } catch (const ob::Error& e) {
            RCLCPP_WARN(this->get_logger(), "IMU not supported or failed to enable: %s", e.what());
        }

        // 2.2 Enable D2C Alignment (Depth to Color)
        config->setAlignMode(ALIGN_D2C_HW_MODE);

        pipe_->start(config);

        // 3. Setup ROS Publishers
        color_pub_ = this->create_publisher<sensor_msgs::msg::Image>("camera/color/image_raw", 10);
        color_info_pub_ = this->create_publisher<sensor_msgs::msg::CameraInfo>("camera/color/camera_info", 10);
        depth_pub_ = this->create_publisher<sensor_msgs::msg::Image>("camera/depth/image_raw", 10);
        depth_info_pub_ = this->create_publisher<sensor_msgs::msg::CameraInfo>("camera/depth/camera_info", 10);
        imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("camera/imu/data_raw", 10);
        pc_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("camera/depth/points", 10);

        // 3.1 Cache Camera Info
        try {
            auto param = pipe_->getCameraParam();
            if (profile_color) {
                color_info_ = convert_to_camera_info(param.rgbIntrinsic, param.rgbDistortion, profile_color->width(), profile_color->height());
            }
            if (profile_depth) {
                depth_info_ = convert_to_camera_info(param.depthIntrinsic, param.depthDistortion, profile_depth->width(), profile_depth->height());
                // Initialize point cloud filter
                point_cloud_filter_.setCreatePointFormat(OB_FORMAT_RGB_POINT);
            }
        } catch (const ob::Error& e) {
            RCLCPP_WARN(this->get_logger(), "Could not fetch calibration: %s", e.what());
        }

        // 4. Start processing loop
        timer_ = this->create_wall_timer(33ms, std::bind(&FemtoMegaNode::process_frames, this));
    }

    ~FemtoMegaNode() {
        if (pipe_) pipe_->stop();
    }

private:
    sensor_msgs::msg::CameraInfo convert_to_camera_info(OBCameraIntrinsic intrinsic, OBCameraDistortion distortion, int width, int height) {
        sensor_msgs::msg::CameraInfo info;
        info.width = width;
        info.height = height;
        info.k = {intrinsic.fx, 0, intrinsic.cx, 0, intrinsic.fy, intrinsic.cy, 0, 0, 1};
        info.p = {intrinsic.fx, 0, intrinsic.cx, 0, 0, intrinsic.fy, intrinsic.cy, 0, 0, 0, 1, 0};
        
        // Distortion model mapping
        if (distortion.model == OB_DISTORTION_KANNALA_BRANDT4) {
             info.distortion_model = "equidistant";
             info.d = {distortion.k1, distortion.k2, distortion.k3, distortion.k4};
        } else {
             info.distortion_model = "plumb_bob";
             // ROS plumb_bob expects k1, k2, p1, p2, k3 (in that order)
             info.d = {distortion.k1, distortion.k2, distortion.p1, distortion.p2, distortion.k3};
        }
        return info;
    }

    void process_frames() {
        auto frameSet = pipe_->waitForFrameset(100);

        // Handle IMU samples if any in the frameset
        if (frameSet) {
            auto accelFrame = frameSet->getFrame(OB_FRAME_ACCEL);
            auto gyroFrame = frameSet->getFrame(OB_FRAME_GYRO);
            if (accelFrame && gyroFrame) {
                auto accel = accelFrame->as<ob::AccelFrame>();
                auto gyro = gyroFrame->as<ob::GyroFrame>();

                sensor_msgs::msg::Imu imu_msg;
                imu_msg.header.stamp = this->get_clock()->now();
                imu_msg.header.frame_id = "camera_imu_frame";

                imu_msg.linear_acceleration.x = accel->value().x;
                imu_msg.linear_acceleration.y = accel->value().y;
                imu_msg.linear_acceleration.z = accel->value().z;

                imu_msg.angular_velocity.x = gyro->value().x;
                imu_msg.angular_velocity.y = gyro->value().y;
                imu_msg.angular_velocity.z = gyro->value().z;

                imu_pub_->publish(imu_msg);
            }
        }

        if (!frameSet) return;

        auto now = this->get_clock()->now();

        // Process Color
        auto colorFrame = frameSet->colorFrame();
        if (colorFrame && colorFrame->dataSize() > 0) {
            cv::Mat colorMat;
            if (colorFrame->format() == OB_FORMAT_MJPG) {
                cv::Mat rawData(1, colorFrame->dataSize(), CV_8UC1, colorFrame->data());
                colorMat = cv::imdecode(rawData, cv::IMREAD_COLOR);
            } else if (colorFrame->format() == OB_FORMAT_RGB) {
                cv::Mat rgbMat(colorFrame->height(), colorFrame->width(), CV_8UC3, colorFrame->data());
                cv::cvtColor(rgbMat, colorMat, cv::COLOR_RGB2BGR);
            }

            if (!colorMat.empty()) {
                auto msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", colorMat).toImageMsg();
                msg->header.stamp = now;
                msg->header.frame_id = "camera_color_frame";
                color_pub_->publish(*msg);

                color_info_.header = msg->header;
                color_info_pub_->publish(color_info_);
            }
        }

        // Process Depth
        auto depthFrame = frameSet->depthFrame();
        if (depthFrame && depthFrame->dataSize() > 0) {
            cv::Mat depthMat(depthFrame->height(), depthFrame->width(), CV_16UC1, depthFrame->data());
            // ROS 16-bit depth is usually published in millimeters (mono16)
            auto msg = cv_bridge::CvImage(std_msgs::msg::Header(), "mono16", depthMat).toImageMsg();
            msg->header.stamp = now;
            msg->header.frame_id = "camera_color_frame"; // D2C Aligned
            depth_pub_->publish(*msg);

            depth_info_.header = msg->header;
            depth_info_pub_->publish(depth_info_);

            // Generate Point Cloud
            if (colorFrame) {
                point_cloud_filter_.setCreatePointFormat(OB_FORMAT_RGB_POINT);
                auto pcFrame = point_cloud_filter_.process(frameSet);
                if (pcFrame) {
                    sensor_msgs::msg::PointCloud2 pc_msg;
                    pc_msg.header = msg->header;
                    pc_msg.width = depthFrame->width();
                    pc_msg.height = depthFrame->height();
                    pc_msg.is_dense = false;
                    pc_msg.is_bigendian = false;

                    // XYZ + RGB (3*float32 + 3*float32 = 24 bytes per point)
                    // Note: This matches ob::OBColorPoint structure
                    pc_msg.point_step = 24;
                    pc_msg.row_step = pc_msg.point_step * pc_msg.width;

                    pc_msg.fields.resize(6);
                    pc_msg.fields[0].name = "x"; pc_msg.fields[0].offset = 0; pc_msg.fields[0].datatype = sensor_msgs::msg::PointField::FLOAT32; pc_msg.fields[0].count = 1;
                    pc_msg.fields[1].name = "y"; pc_msg.fields[1].offset = 4; pc_msg.fields[1].datatype = sensor_msgs::msg::PointField::FLOAT32; pc_msg.fields[1].count = 1;
                    pc_msg.fields[2].name = "z"; pc_msg.fields[2].offset = 8; pc_msg.fields[2].datatype = sensor_msgs::msg::PointField::FLOAT32; pc_msg.fields[2].count = 1;
                    pc_msg.fields[3].name = "r"; pc_msg.fields[3].offset = 12; pc_msg.fields[3].datatype = sensor_msgs::msg::PointField::FLOAT32; pc_msg.fields[3].count = 1;
                    pc_msg.fields[4].name = "g"; pc_msg.fields[4].offset = 16; pc_msg.fields[4].datatype = sensor_msgs::msg::PointField::FLOAT32; pc_msg.fields[4].count = 1;
                    pc_msg.fields[5].name = "b"; pc_msg.fields[5].offset = 20; pc_msg.fields[5].datatype = sensor_msgs::msg::PointField::FLOAT32; pc_msg.fields[5].count = 1;

                    pc_msg.data.resize(pc_msg.row_step * pc_msg.height);
                    memcpy(pc_msg.data.data(), pcFrame->data(), pc_msg.data.size());
                    pc_pub_->publish(pc_msg);
                }
            }
        }
    }

    std::unique_ptr<ob::Pipeline> pipe_;
    ob::PointCloudFilter point_cloud_filter_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr color_pub_;
    rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr color_info_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr depth_pub_;
    rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr depth_info_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pc_pub_;
    sensor_msgs::msg::CameraInfo color_info_;
    sensor_msgs::msg::CameraInfo depth_info_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<FemtoMegaNode>());
    } catch (...) {
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Node shutting down...");
    }
    rclcpp::shutdown();
    return 0;
}
