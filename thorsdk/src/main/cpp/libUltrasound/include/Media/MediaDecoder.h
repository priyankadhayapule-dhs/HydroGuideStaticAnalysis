//
// Copyright 2022 EchoNous Inc.
//
#pragma once

#include <cstdint>
#include <string>

// Forward declarations
struct AAssetManager;
struct AAsset;
struct AMediaExtractor;
struct AMediaFormat;
struct AMediaCodec;
class YUVFrame;

class MediaDecoder {
public:
    MediaDecoder();
    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder(MediaDecoder&&);
    ~MediaDecoder();

    MediaDecoder& operator=(const MediaDecoder&) = delete;
    MediaDecoder& operator=(MediaDecoder&&);

    bool open(AAssetManager* assets, const char* path);
    void close();

    bool nextFrame(YUVFrame& frame);

    bool eof() const;
    int width() const;
    int height() const;
    int colorFormat() const;

    int getFormatChangeFrameNumber() { return (mFormatChangeFrameNumber); };

private:
    bool readFromExtractor();
    bool readFromCodec(YUVFrame& image);

    /// Opened asset (may be NULL)
    AAsset *mAsset;
    /// Opened media extractor (may be NULL)
    AMediaExtractor *mExtractor;
    /// Current format (may be NULL). May change as stream is read
    AMediaFormat *mFormat;
    /// Opened codec (may be NULL)
    AMediaCodec *mCodec;
    /// Current MIME type
    std::string mMIME;

    /// Have we seen end of file from the extractor?
    bool mExtractorEOF;
    /// Have we seen end of file from the codec?
    bool mCodecEOF;

    int mFrames;
    int mFormatChangeFrameNumber;
};
