#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>

#include <libobsensor/ObSensor.hpp>
#include <opencv2/opencv.hpp>

// libdatachannel
#include "rtc/rtc.hpp"

// sbs helper and encoder
#include "sbs_compositor.hpp"
#include "h264_encoder.hpp"

using namespace std::chrono_literals;

class WebRtcSbsStreamer {
public:
    WebRtcSbsStreamer() {
        // 1. Initialize Orbbec SDK
        pipeline = std::make_shared<ob::Pipeline>();
        auto config = std::make_shared<ob::Config>();

        // 1.1 Dynamically find supported profiles instead of hardcoding
        auto colorProfiles = pipeline->getStreamProfileList(OB_SENSOR_COLOR);
        auto depthProfiles = pipeline->getStreamProfileList(OB_SENSOR_DEPTH);

        if (!colorProfiles || !depthProfiles) {
            throw std::runtime_error("Could not find Color or Depth sensor profiles");
        }

        // Try to find a reasonable profile, or just take the first one
        std::shared_ptr<ob::VideoStreamProfile> colorProfile;
        try {
            colorProfile = colorProfiles->getVideoStreamProfile(640, 480, OB_FORMAT_ANY, 30);
        } catch (...) {
            colorProfile = colorProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
        }

        std::shared_ptr<ob::VideoStreamProfile> depthProfile;
        try {
            depthProfile = depthProfiles->getVideoStreamProfile(640, 480, OB_FORMAT_ANY, 30);
        } catch (...) {
            depthProfile = depthProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
        }

        config->enableStream(colorProfile);
        config->enableStream(depthProfile);

        // MANDATORY: Align Depth to Color for SBS reconstruction
        config->setAlignMode(ALIGN_D2C_HW_MODE);

        pipeline->start(config);
        
        // Get device for remote control
        device = pipeline->getDevice();
        
        int colorWidth = colorProfile->width();
        int colorHeight = colorProfile->height();
        
        std::cout << "Camera started with D2C alignment." << std::endl;
        std::cout << "Color Profile: " << colorWidth << "x" << colorHeight << " format: " << colorProfile->format() << std::endl;
        std::cout << "Depth Profile: " << depthProfile->width() << "x" << depthProfile->height() << " format: " << depthProfile->format() << std::endl;

        // 2. Initialize H.264 Encoder (SBS width is colorWidth * 2)
        encoder = std::make_unique<H264Encoder>(colorWidth * 2, colorHeight, 30);

        // 3. Initialize WebRTC
        rtc::InitLogger(rtc::LogLevel::Info);
        rtcConfig.iceServers.emplace_back("stun:stun.l.google.com:19302");
        
        pc = std::make_shared<rtc::PeerConnection>(rtcConfig);

        pc->onStateChange([](rtc::PeerConnection::State state) {
            std::cout << "WebRTC State change: " << state << std::endl;
        });

        pc->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
            std::cout << "WebRTC Gathering State change: " << state << std::endl;
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                auto description = pc->localDescription();
                std::cout << "\n--- START LOCAL DESCRIPTION (SDP) ---" << std::endl;
                std::cout << std::string(*description) << std::endl;
                std::cout << "--- END LOCAL DESCRIPTION (SDP) ---\n" << std::endl;
                std::cout << "Copy the COMPLETE SDP above into the client signaling input." << std::endl;
            }
        });

        // Add Data Channel for Remote Control
        dc = pc->createDataChannel("control");
        dc->onOpen([this]() {
            std::cout << "Control DataChannel open!" << std::endl;
        });

        dc->onMessage([this](rtc::message_variant message) {
            if (std::holds_alternative<std::string>(message)) {
                auto msg = std::get<std::string>(message);
                std::cout << "Received control message: " << msg << std::endl;
                handleControlMessage(msg);
            }
        });

        // Add Video Track (H.264)
        rtc::Description::Video videoDescription("video", rtc::Description::Direction::SendOnly);
        videoDescription.addH264Codec(96); // PT=96
        videoTrack = pc->addTrack(videoDescription);

        // Create the offer
        pc->setLocalDescription(); 

        // Add a thread to wait for the Answer from the browser
        std::thread([this]() {
            std::cout << "Waiting for Answer from browser... Paste it below and press Enter twice:" << std::endl;
            std::string answer;
            std::string line;
            while (std::getline(std::cin, line)) {
                if (line.empty()) break;
                answer += line + "\n";
            }
            if (!answer.empty()) {
                try {
                    pc->setRemoteDescription(rtc::Description(answer, "answer"));
                    std::cout << "Remote description set successfully!" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error setting remote description: " << e.what() << std::endl;
                }
            }
        }).detach();
    }

    void handleControlMessage(const std::string& msg) {
        try {
            // Very simple JSON-like parser (expecting {"type": "...", "value": ...})
            if (msg.find("\"type\":\"laser\"") != std::string::npos) {
                bool value = (msg.find("\"value\":true") != std::string::npos);
                device->setBoolProperty(OB_PROP_LASER_BOOL, value);
                std::cout << "Laser set to: " << (value ? "ON" : "OFF") << std::endl;
            } else if (msg.find("\"type\":\"auto_exposure\"") != std::string::npos) {
                bool value = (msg.find("\"value\":true") != std::string::npos);
                device->setBoolProperty(OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, value);
                std::cout << "Auto Exposure set to: " << (value ? "ON" : "OFF") << std::endl;
            } else if (msg.find("\"type\":\"exposure\"") != std::string::npos) {
                size_t pos = msg.find("\"value\":");
                if (pos != std::string::npos) {
                    int value = std::stoi(msg.substr(pos + 8));
                    device->setIntProperty(OB_PROP_COLOR_EXPOSURE_INT, value);
                    std::cout << "Exposure set to: " << value << std::endl;
                }
            } else if (msg.find("\"type\":\"gain\"") != std::string::npos) {
                size_t pos = msg.find("\"value\":");
                if (pos != std::string::npos) {
                    int value = std::stoi(msg.substr(pos + 8));
                    device->setIntProperty(OB_PROP_COLOR_GAIN_INT, value);
                    std::cout << "Gain set to: " << value << std::endl;
                }
            } else if (msg.find("\"type\":\"viz_mode\"") != std::string::npos) {
                size_t pos = msg.find("\"value\":");
                if (pos != std::string::npos) {
                    int value = std::stoi(msg.substr(pos + 8));
                    compConfig.mode = static_cast<CompositorConfig::Mode>(value);
                    std::cout << "Visualization Mode set to: " << value << std::endl;
                }
            } else if (msg.find("\"type\":\"colormap\"") != std::string::npos) {
                bool value = (msg.find("\"value\":true") != std::string::npos);
                compConfig.useColormap = value;
                std::cout << "Colormap set to: " << (value ? "ON" : "OFF") << std::endl;
            } else if (msg.find("\"type\":\"overlay\"") != std::string::npos) {
                bool value = (msg.find("\"value\":true") != std::string::npos);
                compConfig.showOverlay = value;
                std::cout << "Overlay set to: " << (value ? "ON" : "OFF") << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling control message: " << e.what() << std::endl;
        }
    }

    void run() {
        cv::Mat sbsFrame;
        std::cout << "Streaming loop started. Waiting for WebRTC connection..." << std::endl;
        
        while (running) {
            auto frameset = pipeline->waitForFrameset(100);
            if (!frameset) continue;

            auto colorFrame = frameset->colorFrame();
            auto depthFrame = frameset->depthFrame();

            if (colorFrame && depthFrame) {
                // Transform to visualized frame
                SbsCompositor::compose(colorFrame, depthFrame, sbsFrame, compConfig);

                // Ensure frame is always the expected SBS size for the encoder
                if (sbsFrame.cols != encoder->getWidth() || sbsFrame.rows != encoder->getHeight()) {
                    cv::Mat canvas = cv::Mat::zeros(encoder->getHeight(), encoder->getWidth(), CV_8UC3);
                    
                    // Place it on the left
                    cv::Rect roi(0, 0, std::min(sbsFrame.cols, canvas.cols), std::min(sbsFrame.rows, canvas.rows));
                    sbsFrame(cv::Rect(0, 0, roi.width, roi.height)).copyTo(canvas(roi));
                    sbsFrame = canvas;
                }

                // Encode to H.264
                auto encodedData = encoder->encode(sbsFrame);

                // Send via WebRTC if track is open and has at least one subscriber
                if (videoTrack && videoTrack->isOpen()) {
                    videoTrack->send(reinterpret_cast<const rtc::byte*>(encodedData.data()), 
                                    static_cast<int>(encodedData.size()));
                }
            }
        }
    }

    void stop() {
        running = false;
        pipeline->stop();
    }

private:
    std::shared_ptr<ob::Pipeline> pipeline;
    std::shared_ptr<ob::Device> device;
    std::unique_ptr<H264Encoder> encoder;
    rtc::Configuration rtcConfig;
    std::shared_ptr<rtc::PeerConnection> pc;
    std::shared_ptr<rtc::Track> videoTrack;
    std::shared_ptr<rtc::DataChannel> dc;
    CompositorConfig compConfig;
    bool running = true;
};

int main() try {
    WebRtcSbsStreamer streamer;
    streamer.run();
    return 0;
} catch (const std::exception& e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return -1;
}
