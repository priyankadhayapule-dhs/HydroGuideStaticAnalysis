//
// Copyright 2022 EchoNous Inc.
//
#pragma once

#include <vector>

class YUVFrame;
class CompressedYUVFrame;

/// YUVFrames are specified to use YUV_420_NV21 layout
class YUVFrame {
public:

    /// Converts the data from BGR data into a YUV frame
    /// Input is expected to be tightly packed 24bpp, row contiguous
    static YUVFrame ConvertBGR(int width, int height, const unsigned char* data);

    /// Constructs an empty YUVFrame
    YUVFrame();
    /// Constructs a YUVFrame from raw data (makes a copy of data)
    YUVFrame(int width, int height, const unsigned char* data);

    /// Total size of this frame, in bytes
    int size() const;
    /// Is this frame empty (contains data?)
    bool empty() const;
    void clear();

    /// Resize this frame
    void resize(int width, int height);
    /// Get a pointer to the raw frame data
    unsigned char* data();
    /// Get a const pointer to the raw frame data
    const unsigned char* data() const;

    /// Gets the width of the y channel data
    int ywidth() const;
    /// Gets the height of the y channel data
    int yheight() const;
    /// Gets the base pointer to the y channel data
    const unsigned char* ychannel() const;
    /// Gets the width of the uv channel data
    int uvwidth() const;
    /// Gets the height of the uv channel data
    int uvheight() const;
    /// Gets the base pointer to the uv channel data
    const unsigned char* uvchannel() const;
    /// Compresses this frame's data
    bool compress(CompressedYUVFrame& compressed) const;
    /// Compresses this frame's data, using temp as a temporary storage buffer
    bool compress(CompressedYUVFrame& compressed,
                  std::vector<unsigned char>& temp) const;
private:
    /// Non-const access to ychannel data
    unsigned char* ychannel();

    // Non-const access to uvchannel data
    unsigned char* uvchannel();

    /// Width of the image
    int mWidth;
    /// Height of the image
    int mHeight;
    /// Buffer containing y channel data, followed by interlaced UV channel data
    std::vector<unsigned char> mBuffer;
};

/// A YUV Frame which is compressed in memory (using a single LZ4 block). Must be decoded before
/// being displayed, but LZ4 decode is very fast and can be done on demand.
class CompressedYUVFrame {
public:
    CompressedYUVFrame();

    /// Width of the uncompressed frame
    int width() const;
    /// Height of the uncompressed frame
    int height() const;

    /// Size of the compressed frame, in bytes
    int size() const;
    unsigned char* data();
    const unsigned char* data() const;
    /// Resize
    void resize(int width, int height, int compressedSize);
    /// Decompresses this frame
    bool decompress(YUVFrame& frame) const;
private:
    int mWidth;
    int mHeight;
    std::vector<unsigned char> mBuffer;
};

