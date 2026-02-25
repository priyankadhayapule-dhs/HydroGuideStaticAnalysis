#include <ScanConverterCl.h>
#include <catch2/catch.hpp>
#include <memory>
#include <chrono>
#include <algorithm>

class ElapsedTimeRequirement {
public:
    template <typename Duration>
    ElapsedTimeRequirement(Duration d) : mDuration(ns(d)), mStart(now()) {}
    ~ElapsedTimeRequirement() { REQUIRE( (now()-mStart).count() <= mDuration.count()); }

private:
    template <typename Duration>
    static std::chrono::nanoseconds ns(Duration d)
    { return std::chrono::duration_cast<std::chrono::nanoseconds>(d);}

    static std::chrono::steady_clock::time_point now()
    { return std::chrono::steady_clock::now(); }

    std::chrono::nanoseconds mDuration;
    std::chrono::steady_clock::time_point mStart;
};

TEST_CASE("Test construct/destruct", "[scanconvertercl]" ) {
    ElapsedTimeRequirement timer(std::chrono::milliseconds(1));

    auto scanconverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
    for (int i=0; i < 100; ++i) {
        scanconverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
        REQUIRE(scanconverter != nullptr);
    }
}

TEST_CASE("Test init", "[scanconvertercl]") {
    // Initial init may take a while, since it needs to compile shaders
    ElapsedTimeRequirement timer(std::chrono::milliseconds(200));

    auto scanconverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
    SECTION("Init without setParameters") {
        ThorStatus status = scanconverter->init();
        REQUIRE(IS_THOR_ERROR(status));
    }

    SECTION("Init with smallest expected input/output size") {
        const int samples = 512;
        const float depth = 260.0f;
        const float sampleSpacing = samples/depth;
        ThorStatus status = scanconverter->setConversionParameters(
                samples, 112, // numSamples and numRays
                128, 128, // output size
                0.0f, depth, // Start and end depth (in mm)
                0.0f, sampleSpacing,
                -0.7819f, 0.013962f, // start ray radian and ray spacing
                -1, 1 // flip x and flip y
                );
        REQUIRE(!IS_THOR_ERROR(status));

        status = scanconverter->init();
        REQUIRE(!IS_THOR_ERROR(status));
    }

    SECTION("Init with largest expected input size") {
        const int samples = 512;
        const float depth = 40.0f;
        const float sampleSpacing = samples/depth;
        ThorStatus status = scanconverter->setConversionParameters(
            samples, 224, // numSamples and numRays
            512, 512, // output size
            0.0f, depth, // Start and end depth (in mm)
            0.0f, sampleSpacing,
            -0.7819f, 0.013962f, // start ray radian and ray spacing
            -1, 1 // flip x and flip y
        );
        REQUIRE(!IS_THOR_ERROR(status));

        status = scanconverter->init();
        REQUIRE(!IS_THOR_ERROR(status));
    }
}

TEST_CASE("Test re init", "[scanconvertercl]") {
    ThorStatus status;
    auto scanconverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
    const int margin = 64;

    std::vector<unsigned char> input(512*224);
    std::fill_n(input.begin()+margin, 512*224, 0x7F); // gray

    std::vector<float> output(128*128 + margin + margin);

    auto setParams = [](std::unique_ptr<ScanConverterCL> &scanconverter, int rays, int owidth, int oheight) {
        ThorStatus status = scanconverter->setConversionParameters(512, rays, owidth, oheight,
                0.0f, 120.0f, 0.0f, 512.0f/120.0f, -0.7819f, 0.013962f, -1.0f, 1.0f);
        REQUIRE(!IS_THOR_ERROR(status));
    };

    setParams(scanconverter, 224, 128, 128);

    status = scanconverter->init();
    REQUIRE(!IS_THOR_ERROR(status));

    // Run some conversions
    {
        const int trials = 100;
        auto timer = ElapsedTimeRequirement(std::chrono::milliseconds(trials*2)); // no more than 2ms per frame

        std::fill_n(output.begin(), margin, -10.0f); // invalid marker
        std::fill_n(output.rbegin(), margin, -10.0f);

        for (int i=0; i != trials; ++i) {
            status = scanconverter->runCLTex(input.data(), output.data()+margin);
            REQUIRE(!IS_THOR_ERROR(status));

            // Ensure that none of the margin was overwritten
            REQUIRE(output.begin()+margin == std::find_if_not(output.begin(), output.begin()+margin, [](float val) { return val < -1.0f;}));
            REQUIRE(output.rbegin()+margin == std::find_if_not(output.rbegin(), output.rbegin()+margin, [](float val) { return val < -1.0f;}));
        }
    }

    {
        auto timer = ElapsedTimeRequirement(std::chrono::milliseconds(1)); // no more than 1ms to reconfigure
        setParams(scanconverter, 112, 128, 128);
    }
    
    {
        const int trials = 100;
        auto timer = ElapsedTimeRequirement(std::chrono::milliseconds(trials*2)); // no more than 2ms per frame
        for (int i=0; i != trials; ++i) {
            status = scanconverter->runCLTex(input.data(), output.data()+margin);
            REQUIRE(!IS_THOR_ERROR(status));

            // Ensure that none of the margin was overwritten
            REQUIRE(output.begin()+margin == std::find_if_not(output.begin(), output.begin()+margin, [](float val) { return val < -1.0f;}));
            REQUIRE(output.rbegin()+margin == std::find_if_not(output.rbegin(), output.rbegin()+margin, [](float val) { return val < -1.0f;}));
        }
    }

}