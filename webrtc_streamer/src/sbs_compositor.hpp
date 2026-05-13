#pragma once

#include <opencv2/opencv.hpp>
#include <libobsensor/ObSensor.hpp>

struct CompositorConfig {
    enum Mode {
        SBS = 0,
        COLOR_ONLY = 1,
        DEPTH_ONLY = 2
    } mode = SBS;

    bool useColormap = true;
    bool showOverlay = true;
};

class SbsCompositor {
public:
    /**
     * @brief Composes a frame from raw cv::Mat inputs (no SDK dependency).
     * 
     * @param colorMat The color image as BGR cv::Mat.
     * @param depthMat The depth image as 16UC1 cv::Mat (depth in mm).
     * @param output The output matrix.
     * @param config The compositor configuration.
     */
    static void composeFromMat(const cv::Mat &colorMat, const cv::Mat &depthMat,
                               cv::Mat &output,
                               const CompositorConfig& config = CompositorConfig()) {
        // 1. Process Depth Frame (16-bit -> 8-bit)
        cv::Mat depth8;
        depthMat.convertTo(depth8, CV_8UC1, 255.0 / 5000.0);

        cv::Mat depthVisual;
        if (config.useColormap) {
            cv::applyColorMap(depth8, depthVisual, cv::COLORMAP_JET);
        } else {
            cv::cvtColor(depth8, depthVisual, cv::COLOR_GRAY2BGR);
        }

        // 2. Ensure both are same height
        cv::Mat colorOut = colorMat;
        if (colorOut.rows != depthVisual.rows) {
            cv::resize(depthVisual, depthVisual, colorOut.size());
        }

        // 3. Compose Output based on Mode
        if (config.mode == CompositorConfig::COLOR_ONLY) {
            output = colorOut.clone();
        } else if (config.mode == CompositorConfig::DEPTH_ONLY) {
            output = depthVisual.clone();
        } else {
            cv::hconcat(colorOut, depthVisual, output);
        }

        // 4. Add Overlay if requested
        if (config.showOverlay) {
            std::string text = (config.mode == CompositorConfig::SBS ? "Mode: SBS" :
                               (config.mode == CompositorConfig::COLOR_ONLY ? "Mode: Color" : "Mode: Depth"));
            text += " | " + std::to_string(colorOut.cols) + "x" + std::to_string(colorOut.rows);

            cv::putText(output, text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
        }
    }

    /**
     * @brief Composes a frame based on the configuration.
     * 
     * @param colorFrame The color frame from the SDK.
     * @param depthFrame The depth frame from the SDK (must be aligned to color).
     * @param output The output matrix (will be resized if necessary).
     * @param config The compositor configuration.
     */
    static void compose(std::shared_ptr<ob::ColorFrame> colorFrame, 
                        std::shared_ptr<ob::DepthFrame> depthFrame, 
                        cv::Mat &output,
                        const CompositorConfig& config = CompositorConfig()) {
        if (!colorFrame || !depthFrame) return;

        // 1. Process Color Frame
        cv::Mat colorMat;
        if (colorFrame->format() == OB_FORMAT_RGB) {
            colorMat = cv::Mat(colorFrame->height(), colorFrame->width(), CV_8UC3, colorFrame->data());
            cv::cvtColor(colorMat, colorMat, cv::COLOR_RGB2BGR);
        } else if (colorFrame->format() == OB_FORMAT_MJPG) {
            cv::Mat rawData(1, colorFrame->dataSize(), CV_8UC1, colorFrame->data());
            colorMat = cv::imdecode(rawData, cv::IMREAD_COLOR);
        } else {
            return;
        }

        // 2. Extract raw depth data and delegate to composeFromMat
        cv::Mat depthMat(depthFrame->height(), depthFrame->width(), CV_16UC1, depthFrame->data());
        composeFromMat(colorMat, depthMat, output, config);
    }
};
