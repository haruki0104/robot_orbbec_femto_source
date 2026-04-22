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

        pc->onLocalDescription([](rtc::Description description) {
            std::cout << "\n--- START LOCAL DESCRIPTION (SDP) ---" << std::endl;
            std::cout << std::string(description) << std::endl;
            std::cout << "--- END LOCAL DESCRIPTION (SDP) ---\n" << std::endl;
            std::cout << "Copy the SDP above into the client signaling input." << std::endl;
        });

        pc->onLocalCandidate([](rtc::Candidate candidate) {
            std::cout << "Local Candidate: " << std::string(candidate) << std::endl;
        });

        pc->onStateChange([](rtc::PeerConnection::State state) {
            std::cout << "WebRTC State change: " << state << std::endl;
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

    void run() {
        cv::Mat sbsFrame;
        std::cout << "Streaming loop started. Waiting for WebRTC connection..." << std::endl;
        
        while (running) {
            auto frameset = pipeline->waitForFrameset(100);
            if (!frameset) continue;

            auto colorFrame = frameset->colorFrame();
            auto depthFrame = frameset->depthFrame();

            if (colorFrame && depthFrame) {
                // Transform to SBS
                SbsCompositor::compose(colorFrame, depthFrame, sbsFrame);

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
    std::unique_ptr<H264Encoder> encoder;
    rtc::Configuration rtcConfig;
    std::shared_ptr<rtc::PeerConnection> pc;
    std::shared_ptr<rtc::Track> videoTrack;
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
