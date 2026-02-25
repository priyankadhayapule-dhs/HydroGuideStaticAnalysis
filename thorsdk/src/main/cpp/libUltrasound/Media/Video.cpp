#define LOG_TAG "Video"
#include <Media/Video.h>
#include <Media/MediaDecoder.h>
#include <Filesystem.h>
#include <ThorUtils.h>
#include <zlib.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
namespace {
    using clock = std::chrono::high_resolution_clock;
    using ms = std::chrono::duration<double, std::milli>;
}

static bool strendswith(const char *string, const char *ending) {
    size_t stringLength = strlen(string);
    size_t endingLength = strlen(ending);

    if (stringLength < endingLength)
        return false;
    const char *suffix = string+stringLength-endingLength;
    if (strncmp(suffix, ending, endingLength) == 0)
        return true;
    else
        return false;
}

Video::Video() :
        mWidth(0),
        mHeight(0),
        mUncompressedSize(0),
        mCompressedSize(0),
        mFrames(),
        mCurrentFrame(),
        mUncompressedCRC32(0),
        mCompressedCRC32(0),
        mSecsPerFrame(1.0/30.0),
        mHoldTime(1.0),
        mCurrentFrameIndex(-1) {}
Video::~Video() {
    close();
}
bool Video::decode(Filesystem &filesystem, const char *path) {
    if (strendswith(path, ".jpeg") || strendswith(path, ".jpg") || strendswith(path, ".png")) {
        return decodeImage(filesystem, path);
    } else {
        MediaDecoder decoder;
        return decode(&decoder, filesystem, path);
    }
}
bool Video::decode(MediaDecoder *decoder, Filesystem &filesystem, const char *path) {
    if (strendswith(path, ".jpeg") || strendswith(path, ".jpg") || strendswith(path, ".png")) {
        return decodeImage(filesystem, path);
    }

    const auto startTime = clock::now();
    AAssetManager *manager = filesystem.getAAssetManager();

    if (!decoder->open(manager, path)) return false;

    mUncompressedSize = 0;
    mCompressedSize = 0;
    mUncompressedCRC32 = 0;
    mCompressedCRC32 = 0;
    YUVFrame frame;
    std::vector<unsigned char> compressBuffer;
    while (!decoder->eof()) {
        // If not EOF, we should always get another frame
        if (!decoder->nextFrame(frame)) return false;

        // Compress frame in memory
        mFrames.push_back({});
        if (!frame.compress(mFrames.back(), compressBuffer))
            return false;

        mUncompressedSize += frame.size();
        mCompressedSize += mFrames.back().size();

        mUncompressedCRC32 = crc32(mUncompressedCRC32,
                                   frame.data(),
                                   frame.size());
        mCompressedCRC32 = crc32(mCompressedCRC32,
                                 mFrames.back().data(),
                                 mFrames.back().size());
    }

    // Sanity check resulting frames
    if (mFrames.empty()) {
        LOGE("Video contained no frames");
        return false;
    }

    int formatChangeFrameNumber = decoder->getFormatChangeFrameNumber();

    // remove last frame
    mFrames.pop_back();

    // remove front frames if needed (before the format change)
    for (int i = 0; i <= formatChangeFrameNumber; i++)
        mFrames.erase(mFrames.begin());

    mWidth = mFrames.front().width();
    mHeight = mFrames.front().height();
    for (const auto& frame : mFrames) {
        if (frame.width() != mWidth) {
            LOGE("Width mismatch (%d vs %d)", mWidth, frame.width());
            return false;
        }
        if (frame.height() != mHeight) {
            LOGE("Height mismatch (%d vs %d)", mHeight, frame.height());
            return false;
        }
    }

    // Set default fps to 30, default hold time to 1 second
    setFrameTime(1.0/30.0);
    setHoldTime(1.0);
    resetTime();

    mFrames.pop_back();

    const auto elapsed = ms(clock::now()-startTime);
    LOGD("Opened video file %s in %fms, compression ratio %f",
         path, elapsed.count(), static_cast<double>(mCompressedSize)/mUncompressedSize);

    LOGD("Uncompressed crc32 = 0x%08lx", mUncompressedCRC32);
    LOGD("Compressed crc32 = 0x%08lx", mCompressedCRC32);

    return true;
}
bool Video::decodeImage(Filesystem &filesystem, const char *path) {
    LOGD("Reading image %s", path);
    std::vector<unsigned char> imgData;
    if (IS_THOR_ERROR(filesystem.readAsset(path, imgData))) {
        LOGE("Failed to read asset %s", path);
        return false;
    }

    cv::Mat image = cv::imdecode(imgData, cv::IMREAD_COLOR);
    if(image.data == nullptr) {
        LOGE("Failed to decode image %s", path);
        return false;
    }

    YUVFrame frame = YUVFrame::ConvertBGR(image.cols, image.rows, image.data);
    mFrames.push_back({});
    if (!frame.compress(mFrames.back()))
        return false;

    mWidth = image.cols;
    mHeight = image.rows;
    mCompressedCRC32 = crc32(0, mFrames.back().data(), mFrames.back().size());
    mUncompressedCRC32 = crc32(0, frame.data(), frame.size());
    setFrameTime(1.0/30.0);
    setHoldTime(1.0);
    resetTime();

    return true;
}

void Video::close() {
    mWidth = 0;
    mHeight = 0;
    mUncompressedSize = 0;
    mCompressedSize = 0;
    mUncompressedCRC32 = 0;
    mCompressedCRC32 = 0;
    mFrames.clear();
}
bool Video::save(std::vector<unsigned char> &out) const {
    const auto startTime = clock::now();
    out.clear();
    char id[4] = {'T', 'V', 'I', 'D'};
    int version = 0;
    int flags = 0;
    size_t numFrames = mFrames.size();

    size_t headerSize =
            4 + // Size of "TVID" magic ID
            sizeof(version) +
            sizeof(flags) +
            sizeof(mWidth) +
            sizeof(mHeight) +
            sizeof(mUncompressedSize) +
            sizeof(mCompressedSize) +
            sizeof(mUncompressedCRC32) +
            sizeof(mCompressedCRC32) +
            sizeof(mSecsPerFrame) +
            sizeof(mHoldTime) +
            sizeof(numFrames);

    size_t dataSize = 0;
    for (const CompressedYUVFrame &frame : mFrames) {
        int frameSize = frame.size();
        dataSize += sizeof(frameSize) + frameSize;
    }

    out.resize(headerSize+dataSize);
    unsigned char *dataout = out.data();

    auto write = [](unsigned char *dataout, auto &value){
        memcpy(dataout, reinterpret_cast<const unsigned char*>(&value), sizeof(value));
        return dataout + sizeof(value);
    };

    // Write header information
    dataout = write(dataout, id);
    dataout = write(dataout, version);
    dataout = write(dataout, flags);
    dataout = write(dataout, mWidth);
    dataout = write(dataout, mHeight);
    dataout = write(dataout, mUncompressedSize);
    dataout = write(dataout, mCompressedSize);
    dataout = write(dataout, mUncompressedCRC32);
    dataout = write(dataout, mCompressedCRC32);
    dataout = write(dataout, mSecsPerFrame);
    dataout = write(dataout, mHoldTime);
    dataout = write(dataout, numFrames);

    for (const CompressedYUVFrame &frame : mFrames) {
        int frameSize = frame.size();
        dataout = write(dataout, frameSize);
        memcpy(dataout, frame.data(), frameSize);
        dataout += frameSize;
    }

    const auto elapsed = ms(clock::now() - startTime);
    LOGD("Saved video file in %fms", elapsed.count());

    return true;
}
bool Video::load(const std::vector<unsigned char> &in) {
    const auto startTime = clock::now();
    char id[4];
    int version;
    int flags;
    size_t numFrames;

    close();

    auto read = [](const unsigned char *datain, const unsigned char *dataend, auto &value) {
        if (!datain || (dataend-datain) < sizeof(value)) {
            LOGE("Cached file is too small");
            return static_cast<const unsigned char*>(nullptr);
        }
        memcpy(reinterpret_cast<unsigned char *>(&value), datain, sizeof(value));
        return datain + sizeof(value);
    };
    const unsigned char *datain = in.data();
    const unsigned char *dataend = datain + in.size();
    datain = read(datain, dataend, id);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, version);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, flags);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mWidth);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mHeight);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mUncompressedSize);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mCompressedSize);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mUncompressedCRC32);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mCompressedCRC32);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mSecsPerFrame);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, mHoldTime);
    if (datain == nullptr) { return false; }
    datain = read(datain, dataend, numFrames);
    if (datain == nullptr) { return false; }

    LOGD("Header info: id=\"%c%c%c%c\" version=%d flags=%d width=%d height=%d uncompressedsize=%zu compressedsize=%zu uncompressedcrc32=%lu compressedcrc32=%lu secsperframe=%f holdtime=%f numframes=%zu",
        id[0], id[1], id[2], id[3],
        version, flags, mWidth, mHeight,
        mUncompressedSize, mCompressedSize,
        mUncompressedCRC32, mCompressedCRC32,
        mSecsPerFrame, mHoldTime,
        numFrames);

    LOGD("Reading %zu cached frames", numFrames);

    for (size_t i=0; i != numFrames; ++i) {
        int frameSize;
        datain = read(datain, dataend, frameSize);
        if (datain == nullptr) { return false; }
        if ((dataend-datain) < frameSize) {
            LOGE("Cached file is too small, remaining size %zu, need %d", (dataend-datain), frameSize);
            return false;
        }

        CompressedYUVFrame frame;
        frame.resize(mWidth, mHeight, frameSize);
        memcpy(frame.data(), datain, frameSize);
        datain += frameSize;
        mFrames.emplace_back(std::move(frame));
    }

    const auto elapsed = ms(clock::now() - startTime);
    LOGD("Loaded video file in %fms", elapsed.count());

    return true;
}
int Video::width() const {
    return mWidth;
}
int Video::height() const {
    return mHeight;
}
int Video::numFrames() const {
    return static_cast<int>(mFrames.size());
}
size_t Video::compressedSize() const {
    return mCompressedSize;
}
size_t Video::uncompressedSize() const {
    return mUncompressedSize;
}
void Video::setFrameTime(double secsPerFrame) {
    mSecsPerFrame = secsPerFrame;
}
void Video::setHoldTime(double seconds) {
    mHoldTime = seconds;
}
void Video::resetTime() {
    mStartTime = std::chrono::high_resolution_clock::now();
}
const YUVFrame& Video::currentFrame(double *elapsedtime) {
    // Get current time/frame index
    int frameIndex = 0;
    auto now = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double, std::ratio<1,1>>(now-mStartTime).count();
    double totalTime = mSecsPerFrame * numFrames() + mHoldTime;
    if (time > totalTime) {
        resetTime();
        frameIndex = 0;
    } else {
        frameIndex = static_cast<int>(std::round(time/mSecsPerFrame));
        frameIndex = std::min(frameIndex, numFrames()-1);
    }

    // Decompress the current frame
    if (frameIndex != mCurrentFrameIndex) {
        mFrames[frameIndex].decompress(mCurrentFrame);
        mCurrentFrameIndex = frameIndex;
    }

    if (elapsedtime)
        *elapsedtime = time;

    return mCurrentFrame;
}