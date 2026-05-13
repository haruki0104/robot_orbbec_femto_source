#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "h264_encoder.hpp"

class H264EncoderTest : public ::testing::Test {
protected:
    int width = 640;
    int height = 480;
};

TEST_F(H264EncoderTest, ConstructsAndDestroys) {
    EXPECT_NO_THROW({
        H264Encoder enc(width, height, 30);
    });
}

TEST_F(H264EncoderTest, GetDimensions) {
    H264Encoder enc(width, height, 30);
    EXPECT_EQ(enc.getWidth(), width);
    EXPECT_EQ(enc.getHeight(), height);
}

TEST_F(H264EncoderTest, EncodesSyntheticFrame) {
    H264Encoder enc(width, height, 30);
    cv::Mat frame(height, width, CV_8UC3, cv::Scalar(128, 128, 128));
    auto data = enc.encode(frame);
    EXPECT_FALSE(data.empty());
}

TEST_F(H264EncoderTest, EncodesMultipleFrames) {
    H264Encoder enc(width, height, 30);
    cv::Mat frame(height, width, CV_8UC3, cv::Scalar(64, 128, 192));
    for (int i = 0; i < 5; i++) {
        auto data = enc.encode(frame);
        EXPECT_FALSE(data.empty());
    }
}

TEST_F(H264EncoderTest, DifferentResolution) {
    H264Encoder enc(320, 240, 15);
    EXPECT_EQ(enc.getWidth(), 320);
    EXPECT_EQ(enc.getHeight(), 240);
    cv::Mat frame(240, 320, CV_8UC3, cv::Scalar(0, 255, 0));
    auto data = enc.encode(frame);
    EXPECT_FALSE(data.empty());
}
