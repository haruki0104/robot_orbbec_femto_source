#include <iostream>
#include <vector>
#include <string>
#include <libobsensor/ObSensor.hpp>
#include <opencv2/opencv.hpp>

// Include SbsCompositor from webrtc_streamer
#include "../webrtc_streamer/src/sbs_compositor.hpp"

int main() try {
    ob::Context ctx;
    auto devList = ctx.queryDeviceList();
    if (devList->deviceCount() == 0) {
        std::cerr << "No device found!" << std::endl;
        return -1;
    }

    auto dev = devList->getDevice(0);
    auto info = dev->getDeviceInfo();
    std::cout << "Device: " << info->name() << " (SN: " << info->serialNumber() << ")" << std::endl;

    ob::Pipeline pipe(dev);
    auto config = std::make_shared<ob::Config>();

    // 1. Configure Color Stream
    auto colorProfiles = pipe.getStreamProfileList(OB_SENSOR_COLOR);
    if (!colorProfiles) {
        std::cerr << "No color profiles found!" << std::endl;
        return -1;
    }
    auto colorProfile = colorProfiles->getVideoStreamProfile(640, 480, OB_FORMAT_ANY, 30);
    config->enableStream(colorProfile);

    // 2. Configure Depth Stream
    auto depthProfiles = pipe.getStreamProfileList(OB_SENSOR_DEPTH);
    if (!depthProfiles) {
        std::cerr << "No depth profiles found!" << std::endl;
        return -1;
    }
    auto depthProfile = depthProfiles->getVideoStreamProfile(640, 480, OB_FORMAT_ANY, 30);
    config->enableStream(depthProfile);

    // 3. MANDATORY: Align Depth to Color for SBS
    config->setAlignMode(ALIGN_D2C_HW_MODE);

    // 4. Start the camera
    pipe.start(config);
    std::cout << "SBS Preview started. Press 'q' or 'ESC' to exit." << std::endl;

    const std::string winName = "SBS Verification (Color | Depth)";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    cv::Mat sbsFrame;
    CompositorConfig compConfig;
    compConfig.mode = CompositorConfig::SBS;
    compConfig.useColormap = true;
    compConfig.showOverlay = true;

    while(true) {
        int key = cv::waitKey(1);
        if(key == 'q' || key == 'Q' || key == 27) break;

        // Check if window was closed
        if (cv::getWindowProperty(winName, cv::WND_PROP_VISIBLE) < 1) break;

        auto frameSet = pipe.waitForFrameset(100);
        if(!frameSet) continue;

        auto colorFrame = frameSet->colorFrame();
        auto depthFrame = frameSet->depthFrame();

        if(colorFrame && depthFrame) {
            SbsCompositor::compose(colorFrame, depthFrame, sbsFrame, compConfig);
            if(!sbsFrame.empty()) {
                cv::imshow(winName, sbsFrame);
            }
        }
    }

    pipe.stop();
    cv::destroyAllWindows();
    return 0;
}
catch(ob::Error &e) {
    std::cerr << "SDK Error: " << e.what() << std::endl;
    return -1;
}
catch(const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return -1;
}
