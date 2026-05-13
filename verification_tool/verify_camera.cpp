#include <iostream>
#include <string>
#include <chrono>
#include <libobsensor/ObSensor.hpp>
#include "argparse.hpp"

template<typename DeviceType>
bool verifyDevice(DeviceType dev, int minFrames, int timeoutSeconds, bool verbose) {
    auto info = dev->getDeviceInfo();
    std::cout << "\nChecking device: " << info->name() << " (SN: " << info->serialNumber() << ")\n";

    try {
        ob::Pipeline pipe(dev);
        auto config = std::make_shared<ob::Config>();

        try {
            auto depthProfiles = pipe.getStreamProfileList(OB_SENSOR_DEPTH);
            if (depthProfiles && depthProfiles->getCount() > 0) {
                config->enableStream(depthProfiles->getProfile(0));
            }
        } catch (const ob::Error& e) {
            std::cerr << "  Depth config error: " << e.what() << "\n";
        }

        try {
            auto colorProfiles = pipe.getStreamProfileList(OB_SENSOR_COLOR);
            if (colorProfiles && colorProfiles->getCount() > 0) {
                config->enableStream(colorProfiles->getProfile(0));
            }
        } catch (const ob::Error& e) {
            std::cerr << "  Color config error: " << e.what() << "\n";
        }

        pipe.start(config);

        int depthFrames = 0;
        int colorFrames = 0;
        auto startTime = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - startTime < std::chrono::seconds(timeoutSeconds)) {
            auto frameSet = pipe.waitForFrameset(2000);
            if (!frameSet) {
                if (verbose) std::cerr << "  Timeout waiting frameset...\n";
                continue;
            }

            if (frameSet->depthFrame()) depthFrames++;
            if (frameSet->colorFrame()) colorFrames++;

            if (depthFrames >= minFrames && colorFrames >= minFrames) {
                break;
            }
        }

        pipe.stop();

        bool depthOk = (depthFrames >= minFrames);
        bool colorOk = (colorFrames >= minFrames);

        std::cout << "  Results: depth=" << depthFrames << " color=" << colorFrames
                  << " => " << ((depthOk && colorOk) ? "OK" : "NG") << "\n";

        return depthOk && colorOk;
    } catch (const ob::Error& e) {
        std::cerr << "  Pipeline error: " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char* argv[]) try {
    VerifyConfig config;
    int ret = parseArgs(argc, argv, config);
    if (ret != 0) return ret == 1 ? 0 : 1;

    ob::Context ctx;
    auto devList = ctx.queryDeviceList();

    if (!devList || devList->deviceCount() == 0) {
        std::cerr << "No device found!" << std::endl;
        return -1;
    }

    std::cout << "Found " << devList->deviceCount() << " device(s)." << std::endl;

    bool allOk = true;
    for (uint32_t i = 0; i < devList->deviceCount(); ++i) {
        if (config.deviceIndex >= 0 && static_cast<int>(i) != config.deviceIndex) continue;

        auto dev = devList->getDevice(i);
        if (!dev) continue;

        bool ok = verifyDevice(dev, config.minFrames, config.timeoutSeconds, config.verbose);
        if (!ok) allOk = false;
    }

    std::cout << "\nOverall status: " << (allOk ? "OK" : "NG") << std::endl;
    return allOk ? 0 : 2;
} catch (const ob::Error& e) {
    std::cerr << "SDK Error: " << e.what() << std::endl;
    return -1;
}
