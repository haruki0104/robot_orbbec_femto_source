#pragma once

#include <iostream>
#include <string>
#include <cstdint>

struct VerifyConfig {
    int deviceIndex = -1;
    int timeoutSeconds = 5;
    int minFrames = 5;
    bool verbose = false;
};

inline void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "  -h, --help            Show this help message\n"
              << "  -d, --device INDEX    Verify only device INDEX (0-based)\n"
              << "  -t, --timeout SEC     Timeout seconds (default 5)\n"
              << "  -n, --min-frames N    Minimum frames per stream (default 5)\n"
              << "  -v, --verbose         Enable verbose output\n";
}

inline int parseArgs(int argc, char* argv[], VerifyConfig &config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 1;
        } else if (arg == "-d" || arg == "--device") {
            if (i + 1 >= argc) { std::cerr << "Missing value for " << arg << "\n"; return -1; }
            config.deviceIndex = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--timeout") {
            if (i + 1 >= argc) { std::cerr << "Missing value for " << arg << "\n"; return -1; }
            config.timeoutSeconds = std::stoi(argv[++i]);
        } else if (arg == "-n" || arg == "--min-frames") {
            if (i + 1 >= argc) { std::cerr << "Missing value for " << arg << "\n"; return -1; }
            config.minFrames = std::stoi(argv[++i]);
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return -1;
        }
    }
    return 0;
}
