#include <iostream>
#include <vector>
#include <string>
#include <libobsensor/ObSensor.hpp>
#include <opencv2/opencv.hpp>

int main() try {
    ob::Context ctx;
    auto devList = ctx.queryDeviceList();
    if (devList->deviceCount() == 0) {
        std::cerr << "No device found!" << std::endl;
        return -1;
    }

    auto dev = devList->getDevice(0);
    auto info = dev->getDeviceInfo();
    std::string connectionType = info->getConnectionType();
    std::cout << "Device: " << info->name() << " (SN: " << info->serialNumber() << ")" << std::endl;
    std::cout << "Connection Type: " << connectionType << std::endl;

    ob::Pipeline pipe(dev);
    auto config = std::make_shared<ob::Config>();

    // 1. Configure Color Stream based on connection type
    auto colorProfiles = pipe.getStreamProfileList(OB_SENSOR_COLOR);
    if(colorProfiles) {
        std::shared_ptr<ob::VideoStreamProfile> selectedProfile;
        
        if (connectionType.find("USB3") != std::string::npos) {
            std::cout << "High speed connection detected. Preferring RGB format." << std::endl;
            try {
                selectedProfile = colorProfiles->getVideoStreamProfile(OB_WIDTH_ANY, OB_HEIGHT_ANY, OB_FORMAT_RGB, OB_FPS_ANY);
            } catch (...) {
                selectedProfile = colorProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
            }
        } else {
            std::cout << "Standard/Low speed connection (USB 2.1 or below) detected. Using MJPG format." << std::endl;
            try {
                selectedProfile = colorProfiles->getVideoStreamProfile(OB_WIDTH_ANY, OB_HEIGHT_ANY, OB_FORMAT_MJPG, OB_FPS_ANY);
            } catch (...) {
                selectedProfile = colorProfiles->getProfile(0)->as<ob::VideoStreamProfile>();
            }
        }

        if (selectedProfile) {
            config->enableStream(selectedProfile);
            std::cout << "Selected Color Profile: " << selectedProfile->width() << "x" << selectedProfile->height() 
                      << " @ " << selectedProfile->fps() << "fps, Format: " << selectedProfile->format() << std::endl;
        }
    }

    // 2. Configure Depth Stream
    auto depthProfiles = pipe.getStreamProfileList(OB_SENSOR_DEPTH);
    if(depthProfiles) {
        config->enableStream(depthProfiles->getProfile(0));
    }

    // 3. Start the camera
    pipe.start(config);
    std::cout << "Streaming started. Press 'q' or 'ESC' to exit." << std::endl;

    const std::string colorWin = "Color Stream";
    const std::string depthWin = "Depth Stream";
    cv::namedWindow(colorWin, cv::WINDOW_AUTOSIZE);
    cv::namedWindow(depthWin, cv::WINDOW_AUTOSIZE);

    while(true) {
        auto frameSet = pipe.waitForFrameset(100);
        if(!frameSet) {
            if (cv::getWindowProperty(colorWin, cv::WND_PROP_VISIBLE) < 1 && 
                cv::getWindowProperty(depthWin, cv::WND_PROP_VISIBLE) < 1) break;
            continue;
        }

        // 4. Handle Color Frame
        auto colorFrame = frameSet->colorFrame();
        if(colorFrame && colorFrame->dataSize() > 0) {
            cv::Mat colorMat;
            if(colorFrame->format() == OB_FORMAT_MJPG) {
                cv::Mat rawData(1, colorFrame->dataSize(), CV_8UC1, colorFrame->data());
                colorMat = cv::imdecode(rawData, cv::IMREAD_COLOR);
            } else if(colorFrame->format() == OB_FORMAT_RGB) {
                cv::Mat rgbMat(colorFrame->height(), colorFrame->width(), CV_8UC3, colorFrame->data());
                cv::cvtColor(rgbMat, colorMat, cv::COLOR_RGB2BGR);
            } else if(colorFrame->format() == OB_FORMAT_BGR) {
                colorMat = cv::Mat(colorFrame->height(), colorFrame->width(), CV_8UC3, colorFrame->data()).clone();
            }

            if(!colorMat.empty()) cv::imshow(colorWin, colorMat);
        }

        // 5. Handle Depth Frame
        auto depthFrame = frameSet->depthFrame();
        if(depthFrame && depthFrame->dataSize() > 0) {
            cv::Mat depthMat(depthFrame->height(), depthFrame->width(), CV_16UC1, depthFrame->data());
            cv::Mat depthDisplay;
            depthMat.convertTo(depthDisplay, CV_8U, 255.0 / 5000.0);
            cv::applyColorMap(depthDisplay, depthDisplay, cv::COLORMAP_JET);
            cv::imshow(depthWin, depthDisplay);
        }

        int key = cv::waitKey(1);
        if(key == 'q' || key == 27) break;
        if (cv::getWindowProperty(colorWin, cv::WND_PROP_VISIBLE) < 1 && 
            cv::getWindowProperty(depthWin, cv::WND_PROP_VISIBLE) < 1) break;
    }

    pipe.stop();
    cv::destroyAllWindows();
    return 0;
}
catch(ob::Error &e) {
    std::cerr << "SDK Error: " << e.what() << std::endl;
    return -1;
}
