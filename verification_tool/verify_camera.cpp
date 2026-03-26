#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <libobsensor/ObSensor.hpp>

int main() try {
    ob::Context ctx;
    auto devList = ctx.queryDeviceList();
    if(devList->deviceCount() == 0) {
        std::cerr << "No device found!" << std::endl;
        return -1;
    }

    std::cout << "Found " << devList->deviceCount() << " device(s)." << std::endl;

    for(uint32_t i = 0; i < devList->deviceCount(); i++) {
        auto dev = devList->getDevice(i);
        auto info = dev->getDeviceInfo();
        std::string sn = info->serialNumber();
        std::string name = info->name();
        std::string connectionType = info->getConnectionType();
        
        std::cout << "\nChecking device [" << i << "]: " << name << " (SN: " << sn << ")" << std::endl;
        std::cout << "  Connection: " << connectionType << std::endl;

        bool depth_ok = false;
        bool color_ok = false;

        try {
            ob::Pipeline pipe(dev);
            std::shared_ptr<ob::Config> config = std::make_shared<ob::Config>();

            // Configure depth stream
            try {
                auto depthProfiles = pipe.getStreamProfileList(OB_SENSOR_DEPTH);
                if(depthProfiles) config->enableStream(depthProfiles->getProfile(0));
            } catch (ob::Error &e) { std::cerr << "  Depth config error: " << e.what() << std::endl; }

            // Configure color stream based on connection
            try {
                auto colorProfiles = pipe.getStreamProfileList(OB_SENSOR_COLOR);
                if(colorProfiles) {
                    std::shared_ptr<ob::VideoStreamProfile> selectedProfile;
                    if (connectionType.find("USB3") != std::string::npos) {
                        selectedProfile = colorProfiles->getVideoStreamProfile(OB_WIDTH_ANY, OB_HEIGHT_ANY, OB_FORMAT_RGB, OB_FPS_ANY);
                    } else {
                        selectedProfile = colorProfiles->getVideoStreamProfile(OB_WIDTH_ANY, OB_HEIGHT_ANY, OB_FORMAT_MJPG, OB_FPS_ANY);
                    }
                    config->enableStream(selectedProfile);
                    std::cout << "  Selected Color Format: " << selectedProfile->format() << std::endl;
                }
            } catch (ob::Error &e) { std::cerr << "  Color config error: " << e.what() << std::endl; }

            pipe.start(config);

            int depth_frames = 0;
            int color_frames = 0;
            auto start_time = std::chrono::steady_clock::now();
            
            while(std::chrono::steady_clock::now() - start_time < std::chrono::seconds(5)) {
                auto frameSet = pipe.waitForFrameset(2000);
                if(frameSet) {
                    if(frameSet->depthFrame()) depth_frames++;
                    if(frameSet->colorFrame()) color_frames++;
                }
                if(depth_frames > 5 && color_frames > 5) break;
            }

            pipe.stop();
            depth_ok = (depth_frames > 0);
            color_ok = (color_frames > 0);

            std::cout << "  Results: Depth[" << (depth_ok?"OK":"FAIL") << "] Color[" << (color_ok?"OK":"FAIL") << "]" << std::endl;
            std::cout << "  DEVICE STATUS: " << (depth_ok && color_ok ? "OK" : "NG") << std::endl;

        } catch (ob::Error &e) {
            std::cerr << "  Pipeline error: " << e.what() << std::endl;
            std::cout << "  DEVICE STATUS: NG" << std::endl;
        }
    }
    return 0;
}
catch(ob::Error &e) {
    std::cerr << "SDK Error: " << e.what() << std::endl;
    return -1;
}
