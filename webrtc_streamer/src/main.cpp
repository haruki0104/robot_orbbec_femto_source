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
        try {
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
            
            colorWidth = colorProfile->width();
            colorHeight = colorProfile->height();
            
            std::cout << "Camera started with D2C alignment." << std::endl;
            dummyMode = false;
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not initialize camera: " << e.what() << std::endl;
            std::cerr << "Entering DUMMY MODE for signaling verification." << std::endl;
            colorWidth = 640;
            colorHeight = 480;
            dummyMode = true;
        }

        // 2. Initialize H.264 Encoder (SBS width is colorWidth * 2)
        encoder = std::make_unique<H264Encoder>(colorWidth * 2, colorHeight, 30);

        // 3. Initialize WebRTC
        rtc::InitLogger(rtc::LogLevel::Info);
        rtcConfig.iceServers.emplace_back("stun:stun.l.google.com:19302");
        
        pc = std::make_shared<rtc::PeerConnection>(rtcConfig);

        pc->onStateChange([](rtc::PeerConnection::State state) {
            std::cout << "WebRTC State change: " << state << std::endl;
        });

        pc->onIceStateChange([](rtc::PeerConnection::IceState state) {
            std::cout << "ICE State change: " << state << std::endl;
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

        // 4. Initialize Signaling WebSocket
        ws = std::make_shared<rtc::WebSocket>();

        ws->onOpen([this]() {
            std::cout << "Connected to signaling server!" << std::endl;
            // Create the offer once connected to signaling
            pc->setLocalDescription();
        });

        ws->onMessage([this](rtc::message_variant message) {
            if (std::holds_alternative<std::string>(message)) {
                std::string msg = std::get<std::string>(message);
                
                // If browser requests an offer, or if it's an answer
                if (msg == "request") {
                    std::cout << "Received request from browser. Sending offer..." << std::endl;
                    if (pc->localDescription()) {
                        ws->send(std::string(*pc->localDescription()));
                    } else {
                        pc->setLocalDescription();
                    }
                } else if (msg.find("v=0") != std::string::npos && msg.find("m=video") != std::string::npos) {
                    std::cout << "Received SDP Answer via signaling." << std::endl;
                    try {
                        // libdatachannel can often parse the type from the SDP or we assume answer
                        pc->setRemoteDescription(rtc::Description(msg, "answer"));
                        std::cout << "Remote description (Answer) set successfully!" << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error setting remote description: " << e.what() << std::endl;
                    }
                }
            }
        });

        ws->onError([](std::string error) {
            std::cerr << "Signaling WebSocket error: " << error << std::endl;
        });

        ws->onClosed([]() {
            std::cout << "Signaling WebSocket closed." << std::endl;
        });

        // Connect to local signaling server
        ws->open("ws://127.0.0.1:8889");

        pc->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
            std::cout << "WebRTC Gathering State change: " << state << std::endl;
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                auto description = pc->localDescription();
                std::cout << "\n--- LOCAL DESCRIPTION READY ---" << std::endl;
                
                // Send offer to signaling server
                if (ws->isOpen()) {
                    std::string sdp = std::string(*description);
                    // Wrap in simple JSON for the browser if needed, but the browser side
                    // will be updated to handle this.
                    ws->send(sdp);
                    std::cout << "SDP Offer sent to signaling server." << std::endl;
                }
            }
        });
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
        
        int frameCounter = 0;
        while (running) {
            if (dummyMode) {
                // Generate synthetic SBS frame
                sbsFrame = cv::Mat::zeros(colorHeight, colorWidth * 2, CV_8UC3);
                
                // Left side: Color dummy (changing colors + moving text)
                cv::rectangle(sbsFrame, cv::Rect(0, 0, colorWidth, colorHeight), cv::Scalar(frameCounter % 255, 100, 200), -1);
                cv::putText(sbsFrame, "DUMMY COLOR", cv::Point(50, colorHeight/2), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(255, 255, 255), 3);
                
                // Right side: Depth dummy (gradient + moving circle)
                cv::rectangle(sbsFrame, cv::Rect(colorWidth, 0, colorWidth, colorHeight), cv::Scalar(50, 50, 50), -1);
                int circleX = colorWidth + (frameCounter * 5) % colorWidth;
                cv::circle(sbsFrame, cv::Point(circleX, colorHeight/2), 50, cv::Scalar(0, 255, 255), -1);
                cv::putText(sbsFrame, "DUMMY DEPTH", cv::Point(colorWidth + 50, colorHeight/2 + 50), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 255, 0), 3);
                
                frameCounter++;
                std::this_thread::sleep_for(33ms); // ~30 FPS
            } else {
                auto frameset = pipeline->waitForFrameset(100);
                if (!frameset) continue;

                auto colorFrame = frameset->colorFrame();
                auto depthFrame = frameset->depthFrame();

                if (colorFrame && depthFrame) {
                    // Transform to visualized frame
                    SbsCompositor::compose(colorFrame, depthFrame, sbsFrame, compConfig);
                } else {
                    continue;
                }
            }

            if (!sbsFrame.empty()) {
                // Ensure frame is always the expected SBS size for the encoder
                if (sbsFrame.cols != encoder->getWidth() || sbsFrame.rows != encoder->getHeight()) {
                    cv::Mat canvas = cv::Mat::zeros(encoder->getHeight(), encoder->getWidth(), CV_8UC3);
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
    std::shared_ptr<rtc::WebSocket> ws;
    std::shared_ptr<rtc::Track> videoTrack;
    std::shared_ptr<rtc::DataChannel> dc;
    CompositorConfig compConfig;
    bool running = true;
    bool dummyMode = false;
    int colorWidth = 640;
    int colorHeight = 480;
};

int main() try {
    WebRtcSbsStreamer streamer;
    streamer.run();
    return 0;
} catch (const std::exception& e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return -1;
}
