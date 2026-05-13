#include <gtest/gtest.h>
#include <cstring>
#include <vector>

// Include the argparse header directly
#include "argparse.hpp"

class ArgParseTest : public ::testing::Test {
protected:
    VerifyConfig config;

    void SetUp() override {
        config = VerifyConfig();
    }

    // Helper: build argc/argv from a vector of strings
    std::pair<int, std::vector<char*>> makeArgs(const std::vector<const char*> &args) {
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        argv.push_back(const_cast<char*>("verify_camera"));
        for (auto &a : args) argv.push_back(const_cast<char*>(a));
        return {static_cast<int>(argv.size()), argv};
    }
};

TEST_F(ArgParseTest, DefaultValues) {
    auto [argc, argv] = makeArgs({});
    int ret = parseArgs(argc, argv.data(), config);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(config.deviceIndex, -1);
    EXPECT_EQ(config.timeoutSeconds, 5);
    EXPECT_EQ(config.minFrames, 5);
    EXPECT_FALSE(config.verbose);
}

TEST_F(ArgParseTest, TimeoutFlag) {
    auto [argc, argv] = makeArgs({"-t", "8"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_EQ(config.timeoutSeconds, 8);
}

TEST_F(ArgParseTest, TimeoutLongFlag) {
    auto [argc, argv] = makeArgs({"--timeout", "10"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_EQ(config.timeoutSeconds, 10);
}

TEST_F(ArgParseTest, MinFramesFlag) {
    auto [argc, argv] = makeArgs({"-n", "20"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_EQ(config.minFrames, 20);
}

TEST_F(ArgParseTest, MinFramesLongFlag) {
    auto [argc, argv] = makeArgs({"--min-frames", "15"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_EQ(config.minFrames, 15);
}

TEST_F(ArgParseTest, DeviceIndexFlag) {
    auto [argc, argv] = makeArgs({"-d", "2"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_EQ(config.deviceIndex, 2);
}

TEST_F(ArgParseTest, DeviceIndexLongFlag) {
    auto [argc, argv] = makeArgs({"--device", "1"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_EQ(config.deviceIndex, 1);
}

TEST_F(ArgParseTest, VerboseFlag) {
    auto [argc, argv] = makeArgs({"-v"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_TRUE(config.verbose);
}

TEST_F(ArgParseTest, VerboseLongFlag) {
    auto [argc, argv] = makeArgs({"--verbose"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_TRUE(config.verbose);
}

TEST_F(ArgParseTest, CombinedFlags) {
    auto [argc, argv] = makeArgs({"-v", "-t", "3", "-n", "10", "-d", "0"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 0);
    EXPECT_TRUE(config.verbose);
    EXPECT_EQ(config.timeoutSeconds, 3);
    EXPECT_EQ(config.minFrames, 10);
    EXPECT_EQ(config.deviceIndex, 0);
}

TEST_F(ArgParseTest, HelpFlagReturnsOne) {
    auto [argc, argv] = makeArgs({"-h"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 1);
}

TEST_F(ArgParseTest, HelpLongFlagReturnsOne) {
    auto [argc, argv] = makeArgs({"--help"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), 1);
}

TEST_F(ArgParseTest, UnknownFlagReturnsError) {
    auto [argc, argv] = makeArgs({"--unknown"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), -1);
}

TEST_F(ArgParseTest, MissingValueForFlagReturnsError) {
    auto [argc, argv] = makeArgs({"-t"});
    EXPECT_EQ(parseArgs(argc, argv.data(), config), -1);
}

TEST_F(ArgParseTest, DefaultsPreservedOnError) {
    config.timeoutSeconds = 99;
    auto [argc, argv] = makeArgs({"--unknown"});
    parseArgs(argc, argv.data(), config);
    EXPECT_EQ(config.timeoutSeconds, 99);
}
