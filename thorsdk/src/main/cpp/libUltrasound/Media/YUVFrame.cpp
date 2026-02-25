#define LOG_TAG "YUVFrame"
#include <Media/YUVFrame.h>
#include <ThorUtils.h>
#include <assert.h>
//#include "lz4.h"
//#include "lz4hc.h"
//#include "log.h"

#define ENABLE_YUV_COMPRESSION 0
#define USE_HC_COMPRESSION 0

static float rgbsample(int channel, int row, int col, int width, const unsigned char *data) {
    return data[row*width*3+col*3+channel]/255.0f;
}
static float rgbsample2x2(int channel, int row, int col, int width, const unsigned char *data) {
    float s00 = rgbsample(channel, row, col, width, data);
    float s01 = rgbsample(channel, row, col+1, width, data);
    float s10 = rgbsample(channel, row+1, col, width, data);
    float s11 = rgbsample(channel, row+1, col+1, width, data);
    return (s00+s01+s10+s11)/4.0f;
}

static int clamp(int value) {
    if (value <= 0) return 0;
    if (value >= 255) return 255;
    return value;
}

YUVFrame YUVFrame::ConvertBGR(int width, int height, const unsigned char *data) {
    assert(width % 2 == 0);
    assert(height % 2 == 0);

    YUVFrame result;
    result.mWidth = width;
    result.mHeight = height;
    result.mBuffer.resize(width*height*6/4);

    // either:
    //  reading rgb image too far ahead (by 1 px per row??)
    // or
    //  writing yuv image too far behind
    //  BUT this would leave the ending unwritten

    // y plane is full resolution
    unsigned char *destY = result.ychannel();
    for (int row=0; row != height; ++row) {
        for (int col=0; col != width; ++col) {
            float b = rgbsample(0, row, col, width, data);
            float g = rgbsample(1, row, col, width, data);
            float r = rgbsample(2, row, col, width, data);
            float y = 0.257*r + 0.504*g + 0.098*b;
            destY[row*width+col] = clamp(y * 255.0f);
        }
    }

    // uv plane is half resolution (in both directions)
    unsigned char *destUV = result.uvchannel();
    for (int row=0; row != result.uvheight(); ++row) {
        for (int col=0; col != result.uvwidth(); ++col) {
            float b = rgbsample2x2(0, 2*row, 2*col, width, data);
            float g = rgbsample2x2(1, 2*row, 2*col, width, data);
            float r = rgbsample2x2(2, 2*row, 2*col, width, data);
            float u = -0.148*r - 0.291*g + 0.439*b + 0.5;
            float v = 0.439*r - 0.368*g - 0.071*b + 0.5;
            destUV[row * width + col * 2 + 0] = clamp(u * 255.0f);
            destUV[row * width + col * 2 + 1] = clamp(v * 255.0f);
        }
    }
    LOGD("Last value = %d", (int)result.mBuffer.back());
    return result;
}

YUVFrame::YUVFrame() :
        mWidth(0),
        mHeight(0),
        mBuffer() {}
YUVFrame::YUVFrame(int width, int height, const unsigned char *data) :
        mWidth(width),
        mHeight(height),
        mBuffer(width*height*6/4) {
    std::copy_n(data, mBuffer.size(), mBuffer.data());
}
int YUVFrame::size() const {
    return static_cast<int>(mBuffer.size());
}
bool YUVFrame::empty() const {
    return mBuffer.empty();
}
void YUVFrame::clear() {
    resize(0,0);
}
void YUVFrame::resize(int width, int height) {
    mWidth = width;
    mHeight = height;
    mBuffer.resize(width*height*6/4);
}
unsigned char* YUVFrame::data() {
    return mBuffer.data();
}
const unsigned char* YUVFrame::data() const {
    return mBuffer.data();
}
int YUVFrame::ywidth() const {
    return mWidth;
}
int YUVFrame::yheight() const {
    return mHeight;
}
const unsigned char* YUVFrame::ychannel() const {
    return mBuffer.data();
}
unsigned char* YUVFrame::ychannel() {
    return mBuffer.data();
}
int YUVFrame::uvwidth() const {
    return mWidth/2;
}
int YUVFrame::uvheight() const {
    return mHeight/2;
}
const unsigned char* YUVFrame::uvchannel() const {
    return mBuffer.data() + mWidth*mHeight;
}
unsigned char* YUVFrame::uvchannel() {
    return mBuffer.data() + mWidth*mHeight;
}
bool YUVFrame::compress(CompressedYUVFrame& compressed) const {
    std::vector<unsigned char> temp;
    return compress(compressed, temp);
}
bool YUVFrame::compress(CompressedYUVFrame &compressed,
                        std::vector<unsigned char>& temp) const {
#if ENABLE_YUV_COMPRESSION
    temp.resize(LZ4_compressBound(size()));

#if USE_HC_COMPRESSION
    int rc = LZ4_compress_HC(
            reinterpret_cast<const char*>(data()),
            reinterpret_cast<char*>(temp.data()),
            size(),
            static_cast<int>(temp.size()),
            LZ4HC_CLEVEL_MAX);
#else
    int rc = LZ4_compress_default(
            reinterpret_cast<const char*>(data()),
            reinterpret_cast<char*>(temp.data()),
            size(),
            static_cast<int>(temp.size()));
#endif
    if (rc == 0)  {
        LOGE("Compression failed");
        return false;
    }
    compressed.resize(mWidth, mHeight, rc);
    std::copy_n(temp.data(), rc, compressed.data());
    return true;
#else
    compressed.resize(mWidth, mHeight, size());
    std::copy_n(data(), size(), compressed.data());
    return true;
#endif
}

CompressedYUVFrame::CompressedYUVFrame() :
        mWidth(0),
        mHeight(0),
        mBuffer() {}
int CompressedYUVFrame::width() const {
    return mWidth;
}
int CompressedYUVFrame::height() const {
    return mHeight;
}
int CompressedYUVFrame::size() const {
    return static_cast<int>(mBuffer.size());
}
unsigned char* CompressedYUVFrame::data() {
    return mBuffer.data();
}
const unsigned char* CompressedYUVFrame::data() const {
    return mBuffer.data();
}
void CompressedYUVFrame::resize(int width, int height, int compressedSize) {
    mWidth = width;
    mHeight = height;
    mBuffer.resize(compressedSize);
}
bool CompressedYUVFrame::decompress(YUVFrame &frame) const {
#if ENABLE_YUV_COMPRESSION
    frame.resize(mWidth, mHeight);
    int rc = LZ4_decompress_safe(
            reinterpret_cast<const char*>(data()),
            reinterpret_cast<char*>(frame.data()),
            size(),
            frame.size());
    if (rc != frame.size()) {
        LOGE("Decompression failed, rc = %d", rc);
        return false;
    }
    return true;
#else
    frame.resize(mWidth, mHeight);
    std::copy_n(data(), size(), frame.data());
    return true;
#endif
}