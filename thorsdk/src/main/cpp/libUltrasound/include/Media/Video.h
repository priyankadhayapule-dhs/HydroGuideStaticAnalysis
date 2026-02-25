//
// Copyright 2022 EchoNous Inc.
//
#pragma once

#include "YUVFrame.h"
#include <iosfwd>
#include <vector>
#include <chrono>

// Forward declarations
struct AAssetManager;
class MediaDecoder;
class Filesystem;

class Video {
public:
    Video();
    ~Video();

    bool decode(Filesystem &filesystem, const char *path);
    bool decode(MediaDecoder *decoder, Filesystem &filesystem, const char *path);
    bool decodeImage(Filesystem &filesystem, const char *path);
    void close();

    /// Save compressed video (in a custom format) to a stream
    bool save(std::vector<unsigned char> &out) const;
    /// Load compressed video from custom format
    bool load(const std::vector<unsigned char> &in);

    int width() const;
    int height() const;
    int numFrames() const;
    size_t compressedSize() const;
    size_t uncompressedSize() const;

    void setFrameTime(double secsPerFrame);
    void setHoldTime(double seconds);

    void resetTime();
    const YUVFrame& currentFrame(double *elapsedtime = nullptr);
private:
    int mWidth;
    int mHeight;
    size_t mUncompressedSize;
    size_t mCompressedSize;
    std::vector<CompressedYUVFrame> mFrames;
    YUVFrame mCurrentFrame;
    unsigned long mUncompressedCRC32;
    unsigned long mCompressedCRC32;
    double mSecsPerFrame;
    double mHoldTime;

    int mCurrentFrameIndex;

    std::chrono::high_resolution_clock::time_point mStartTime;
};


