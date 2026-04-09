#include <iostream>
#include <string>
#include <chrono>
#include <libobsensor/ObSensor.hpp>

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "  -h, --help            Show this help message\n"
              << "  -d, --device INDEX    Verify only device INDEX (0-based)\n"
              << "  -t, --timeout SEC     Timeout seconds (default 5)\n"
              << "  -n, --min-frames N    Minimum frames per stream (default 5)\n"
              << "  -v, --verbose         Enable verbose output\n";
}

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
    int deviceIndex = -1;
    int timeoutSeconds = 5;
    int minFrames = 5;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-d" || arg == "--device") {
            if (i + 1 >= argc) { std::cerr << "Missing value for " << arg << "\n"; return 1; }
            deviceIndex = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--timeout") {
            if (i + 1 >= argc) { std::cerr << "Missing value for " << arg << "\n"; return 1; }
            timeoutSeconds = std::stoi(argv[++i]);
        } else if (arg == "-n" || arg == "--min-frames") {
            if (i + 1 >= argc) { std::cerr << "Missing value for " << arg << "\n"; return 1; }
            minFrames = std::stoi(argv[++i]);
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    ob::Context ctx;
    auto devList = ctx.queryDeviceList();

    if (!devList || devList->deviceCount() == 0) {
        std::cerr << "No device found!" << std::endl;
        return -1;
    }

    std::cout << "Found " << devList->deviceCount() << " device(s)." << std::endl;

    bool allOk = true;
    for (uint32_t i = 0; i < devList->deviceCount(); ++i) {
        if (deviceIndex >= 0 && static_cast<int>(i) != deviceIndex) continue;

        auto dev = devList->getDevice(i);
        if (!dev) continue;

        bool ok = verifyDevice(dev, minFrames, timeoutSeconds, verbose);
        if (!ok) allOk = false;
    }

    std::cout << "\nOverall status: " << (allOk ? "OK" : "NG") << std::endl;
    return allOk ? 0 : 2;
} catch (const ob::Error& e) {
    std::cerr << "SDK Error: " << e.what() << std::endl;
    return -1;
}
