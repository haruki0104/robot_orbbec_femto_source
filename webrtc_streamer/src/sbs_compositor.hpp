#pragma once

#include <opencv2/opencv.hpp>
#include <libobsensor/ObSensor.hpp>

class SbsCompositor {
public:
    /**
     * @brief Composes a Side-By-Side (SBS) frame from color and depth.
     * 
     * @param colorFrame The color frame from the SDK.
     * @param depthFrame The depth frame from the SDK (must be aligned to color).
     * @param output The output matrix (will be resized if necessary).
     */
    static void compose(std::shared_ptr<ob::ColorFrame> colorFrame, 
                        std::shared_ptr<ob::DepthFrame> depthFrame, 
                        cv::Mat &output) {
        if (!colorFrame || !depthFrame) return;

        // 1. Process Color Frame
        cv::Mat colorMat;
        if (colorFrame->format() == OB_FORMAT_RGB) {
            colorMat = cv::Mat(colorFrame->height(), colorFrame->width(), CV_8UC3, colorFrame->data());
        } else if (colorFrame->format() == OB_FORMAT_MJPG) {
            cv::Mat rawData(1, colorFrame->dataSize(), CV_8UC1, colorFrame->data());
            colorMat = cv::imdecode(rawData, cv::IMREAD_COLOR);
        } else {
            // Fallback for other formats if needed
            return;
        }

        // 2. Process Depth Frame (16-bit -> 8-bit Grayscale)
        cv::Mat depthMat(depthFrame->height(), depthFrame->width(), CV_16UC1, depthFrame->data());
        cv::Mat depth8;
        
        // Normalize depth for visualization/streaming:
        // Femto Mega range is typically up to 10m (10000mm).
        // We can scale it to 0-255. 0.05 factor approx maps 5000mm to 255.
        depthMat.convertTo(depth8, CV_8UC1, 255.0 / 5000.0); 

        // 3. Ensure both are same height for SBS (they should be if D2C aligned)
        if (colorMat.rows != depth8.rows) {
            cv::resize(depth8, depth8, colorMat.size());
        }

        // 4. Convert depth8 to 3-channel so we can hconcat with color
        cv::Mat depthRGB;
        cv::cvtColor(depth8, depthRGB, cv::COLOR_GRAY2BGR);

        // 5. Concatenate Side-By-Side
        cv::hconcat(colorMat, depthRGB, output);
    }
};
