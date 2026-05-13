#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "sbs_compositor.hpp"

class SbsCompositorTest : public ::testing::Test {
protected:
    cv::Mat colorMat;
    cv::Mat depthMat;
    cv::Mat output;

    void SetUp() override {
        // 640x480 BGR color image (solid blue)
        colorMat = cv::Mat(480, 640, CV_8UC3, cv::Scalar(255, 0, 0));

        // 640x480 16-bit depth image with a ramp pattern
        depthMat = cv::Mat(480, 640, CV_16UC1, cv::Scalar(0));
        for (int r = 0; r < depthMat.rows; r++) {
            uint16_t *row = depthMat.ptr<uint16_t>(r);
            for (int c = 0; c < depthMat.cols; c++) {
                row[c] = static_cast<uint16_t>((r * 5000) / depthMat.rows);
            }
        }

        output = cv::Mat();
    }
};

TEST_F(SbsCompositorTest, SBSModeProducesDoubleWidthOutput) {
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::SBS;
    SbsCompositor::composeFromMat(colorMat, depthMat, output, cfg);
    EXPECT_FALSE(output.empty());
    EXPECT_EQ(output.rows, 480);
    EXPECT_EQ(output.cols, 1280); // 640 * 2
    EXPECT_EQ(output.type(), CV_8UC3);
}

TEST_F(SbsCompositorTest, ColorOnlyModeSameSize) {
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::COLOR_ONLY;
    SbsCompositor::composeFromMat(colorMat, depthMat, output, cfg);
    EXPECT_FALSE(output.empty());
    EXPECT_EQ(output.rows, 480);
    EXPECT_EQ(output.cols, 640);
    EXPECT_EQ(output.type(), CV_8UC3);
}

TEST_F(SbsCompositorTest, DepthOnlyModeSameSize) {
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::DEPTH_ONLY;
    SbsCompositor::composeFromMat(colorMat, depthMat, output, cfg);
    EXPECT_FALSE(output.empty());
    EXPECT_EQ(output.rows, 480);
    EXPECT_EQ(output.cols, 640);
    EXPECT_EQ(output.type(), CV_8UC3);
}

TEST_F(SbsCompositorTest, WithoutColormapDepthIsGray) {
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::DEPTH_ONLY;
    cfg.useColormap = false;
    cfg.showOverlay = false;
    SbsCompositor::composeFromMat(colorMat, depthMat, output, cfg);
    EXPECT_FALSE(output.empty());
    // In grayscale BGR, all channels should be equal per pixel
    cv::Vec3b pixel = output.at<cv::Vec3b>(output.rows / 2, output.cols / 2);
    EXPECT_NEAR(pixel[0], pixel[1], 2);
    EXPECT_NEAR(pixel[1], pixel[2], 2);
}

TEST_F(SbsCompositorTest, MismatchedSizesAreResized) {
    cv::Mat smallColor(240, 320, CV_8UC3, cv::Scalar(0, 255, 0));
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::SBS;
    SbsCompositor::composeFromMat(smallColor, depthMat, output, cfg);
    EXPECT_FALSE(output.empty());
    EXPECT_EQ(output.rows, 240);
    EXPECT_EQ(output.cols, 640); // 320 * 2
}

TEST_F(SbsCompositorTest, OverlayTextPresentWhenEnabled) {
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::SBS;
    cfg.showOverlay = true;
    SbsCompositor::composeFromMat(colorMat, depthMat, output, cfg);
    EXPECT_FALSE(output.empty());
    // Overlay is drawn at (10, 30) with green text, ~200x15px area
    cv::Mat roi = output(cv::Rect(10, 15, 200, 18));
    cv::Scalar mean = cv::mean(roi);
    // Green channel should be elevated by the overlay text
    EXPECT_GT(mean[1], 50);
}

TEST_F(SbsCompositorTest, OverlayTextAbsentWhenDisabled) {
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::SBS;
    cfg.showOverlay = false;
    SbsCompositor::composeFromMat(colorMat, depthMat, output, cfg);
    EXPECT_FALSE(output.empty());
    // Without overlay, top-left area is pure blue
    cv::Mat roi = output(cv::Rect(0, 0, 300, 50));
    cv::Scalar mean = cv::mean(roi);
    // Blue channel should dominate green
    EXPECT_GT(mean[0], mean[1] * 2);
}

TEST_F(SbsCompositorTest, LeftHalfIsColorRightHalfIsDepth) {
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::SBS;
    cfg.showOverlay = false;
    SbsCompositor::composeFromMat(colorMat, depthMat, output, cfg);

    // Left half should be blue (solid color)
    cv::Scalar leftCenter = cv::mean(output(cv::Rect(320, 240, 10, 10)));
    EXPECT_GT(leftCenter[0], 200);

    // Right half should NOT be pure blue (depth colormap)
    cv::Scalar rightCenter = cv::mean(output(cv::Rect(960, 240, 10, 10)));
    EXPECT_LT(rightCenter[0], 200);
}

TEST_F(SbsCompositorTest, AllBlackDepthOutputsMinDepth) {
    cv::Mat zeroDepth(480, 640, CV_16UC1, cv::Scalar(0));
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::DEPTH_ONLY;
    cfg.showOverlay = false;
    SbsCompositor::composeFromMat(colorMat, zeroDepth, output, cfg);
    // Depth 0mm -> normalized to 0 -> JET colormap maps to dark blue (B≈128, G≈0, R≈0)
    // Green and red channels should be low for close-range depth
    cv::Scalar mean = cv::mean(output);
    EXPECT_LT(mean[1], 50); // Green channel low
    EXPECT_LT(mean[2], 50); // Red channel low
}

TEST_F(SbsCompositorTest, MaxDepthOutputsBright) {
    cv::Mat maxDepth(480, 640, CV_16UC1, cv::Scalar(5000));
    CompositorConfig cfg;
    cfg.mode = CompositorConfig::DEPTH_ONLY;
    cfg.showOverlay = false;
    SbsCompositor::composeFromMat(colorMat, maxDepth, output, cfg);
    // Depth 5000mm -> normalized to 255 -> JET colormap maps to red (B≈0, G≈0, R≈128)
    cv::Scalar mean = cv::mean(output);
    EXPECT_GT(mean[2], 80); // Red channel elevated
}
