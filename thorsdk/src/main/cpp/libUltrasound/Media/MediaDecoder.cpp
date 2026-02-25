//
// Copyright 2022 EchoNous Inc.
//
#define LOG_TAG "MediaDecoder"
#include <Media/MediaDecoder.h>
#include <Media/YUVFrame.h>
#include <ThorUtils.h>
#include <android/asset_manager.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#include <cstring>
#include <cassert>
#include <cinttypes>
#include <utility>

MediaDecoder::MediaDecoder() :
        mAsset(nullptr),
        mExtractor(nullptr),
        mFormat(nullptr),
        mCodec(nullptr),
        mExtractorEOF(true),
        mCodecEOF(true)
{}
MediaDecoder::~MediaDecoder() { close(); }
MediaDecoder::MediaDecoder(MediaDecoder &&rhs) :
        mAsset(std::exchange(rhs.mAsset, nullptr)),
        mExtractor(std::exchange(rhs.mExtractor, nullptr)),
        mFormat(std::exchange(rhs.mFormat, nullptr)),
        mCodec(std::exchange(rhs.mCodec, nullptr)),
        mExtractorEOF(std::exchange(rhs.mExtractorEOF, true)),
        mCodecEOF(std::exchange(rhs.mCodecEOF, true))
{}
MediaDecoder& MediaDecoder::operator=(MediaDecoder &&rhs) {
    close();
    mAsset = std::exchange(rhs.mAsset, nullptr);
    mExtractor = std::exchange(rhs.mExtractor, nullptr);
    mFormat = std::exchange(rhs.mFormat, nullptr);
    mCodec = std::exchange(rhs.mCodec, nullptr);
    mExtractorEOF = std::exchange(rhs.mExtractorEOF, true);
    mCodecEOF = std::exchange(rhs.mCodecEOF, true);
    return *this;
}

bool MediaDecoder::open(AAssetManager *assets, const char *path) {
    // Always close extractor
    if (mExtractor)
        AMediaExtractor_delete(std::exchange(mExtractor, nullptr));

    // Close existing asset, if any
    if (mAsset)
        AAsset_close(std::exchange(mAsset, nullptr));
    mAsset = AAssetManager_open(assets, path, AASSET_MODE_STREAMING);
    if (!mAsset) { LOGE("Failed to open asset"); return false; }

    // Create extractor
    mExtractor = AMediaExtractor_new();
    if (!mExtractor) { LOGE("Failed to create extractor"); return false; }

    off64_t offset, len;
    int fd = AAsset_openFileDescriptor64(mAsset, &offset, &len);
    if (fd < 0) { LOGE("Failed to open file descriptor"); return false; }
    LOGD("Opened file descriptor to asset %s = %d (offset=%" PRId64 ", len=%" PRId64 ")", path, fd, offset, len);

    media_status_t status = AMediaExtractor_setDataSourceFd(mExtractor, fd, offset, len);
    if (status != AMEDIA_OK) {
        LOGE("Failed to set extractor data source, error = %d", static_cast<int>(status));
        return false;
    }

    // Search for video track
    size_t numTracks = AMediaExtractor_getTrackCount(mExtractor);
    for (size_t i=0; i != numTracks; ++i) {
        AMediaFormat *format = AMediaExtractor_getTrackFormat(mExtractor, i);
        const char *mime;
        if (!AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) continue;
        if (strncmp(mime, "video", 5) == 0) {
            status = AMediaExtractor_selectTrack(mExtractor, i);
            if (status != AMEDIA_OK) {
                LOGE("Failed to set select track, error = %d", static_cast<int>(status));
                return false;
            }
            mFormat = format;

            // Only create new codec if the MIME type is different from what we have
            if (mCodec) {
                if (mMIME == mime)
                    AMediaCodec_stop(mCodec);
                else
                    AMediaCodec_delete(std::exchange(mCodec, nullptr));
            }
            if (!mCodec) {
                mMIME = mime;
                mCodec = AMediaCodec_createDecoderByType(mime);
                LOGD("Created codec for MIME type %s", mime);
            }
            break;
        }
    }
    // No suitable track found (or codec creation failed?)
    if (!mFormat || !mCodec) { LOGE("No suitable video track found, or codec creation failed."); return false; }

    status = AMediaCodec_configure(mCodec, mFormat, nullptr, nullptr, 0);
    if (status != AMEDIA_OK) { LOGE("Failed to configure codec, error = %d", static_cast<int>(status)); return false; }

    status = AMediaCodec_start(mCodec);
    if (status != AMEDIA_OK) { LOGE("Failed to start codec, error = %d", static_cast<int>(status)); return false; }

    mExtractorEOF = false;
    mCodecEOF = false;
    LOGD("Media format is %s\n", AMediaFormat_toString(mFormat));
    mFrames = -1;
    mFormatChangeFrameNumber = -1;

    return true;
}
void MediaDecoder::close() {
    mCodecEOF = true;
    mExtractorEOF = true;
    if (mCodec) {
        AMediaCodec_delete(mCodec);
        mCodec = nullptr;
    }
    if (mFormat) {
        // Format is not allocated by us but by the extractor.
        // We do not have to free the format
        mFormat = nullptr;
    }
    if (mExtractor) {
        AMediaExtractor_delete(mExtractor);
        mExtractor = nullptr;
    }
    if (mAsset) {
        AAsset_close(mAsset);
        mAsset = nullptr;
    }
}
bool MediaDecoder::eof() const {
    return mExtractorEOF && mCodecEOF;
}
int MediaDecoder::width() const {
    int width = 0;
    if (!mFormat || !AMediaFormat_getInt32(mFormat, AMEDIAFORMAT_KEY_WIDTH, &width)) return -1;
    return width;
}
int MediaDecoder::height() const {
    int height = 0;
    if (!mFormat || !AMediaFormat_getInt32(mFormat, AMEDIAFORMAT_KEY_HEIGHT, &height)) return -1;
    return height;
}
int MediaDecoder::colorFormat() const {
    int colorFormat = 0;
    if (!mFormat || !AMediaFormat_getInt32(mFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, &colorFormat)) return -1;
    return colorFormat;
}
bool MediaDecoder::nextFrame(YUVFrame& frame) {
    bool status;
    frame.clear();
    while (frame.empty()) {
        if (!mExtractorEOF) {
            status = readFromExtractor();
            if (!status) return false;
        }
        if (!mCodecEOF) {
            status = readFromCodec(frame);
            if (!status) return false;
        }
    }
    return true;
}
bool MediaDecoder::readFromExtractor() {
    //assert(mExtractor);
    //assert(mCodec);
    //assert(!mExtractorEOF);

    size_t bufSize = 0;
    ssize_t bufIndex = AMediaCodec_dequeueInputBuffer(mCodec, -1);
    if (bufIndex < 0) {
        LOGE("Error dequeueing input buffer from codec, error code = %d\n", static_cast<int>(bufIndex));
        return false;
    }
    unsigned char *buffer = AMediaCodec_getInputBuffer(mCodec, bufIndex, &bufSize);
    ssize_t sampleSize = AMediaExtractor_readSampleData(mExtractor, buffer, bufSize);
    uint32_t flags = 0;
    if (sampleSize < 0) {
        mExtractorEOF = true;
        sampleSize = 0;
        flags = AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM;
    }
    int64_t sampleTime = AMediaExtractor_getSampleTime(mExtractor);
    media_status_t status = AMediaCodec_queueInputBuffer(mCodec, bufIndex, 0, sampleSize, sampleTime, flags);
    if (status != AMEDIA_OK) {
        LOGE("Failed to queue input buffer, error = %d", static_cast<int>(status));
        return false;
    }
    AMediaExtractor_advance(mExtractor);
    return true;
}
bool MediaDecoder::readFromCodec(YUVFrame& frame) {
    //assert(!mCodecEOF);
    size_t bufSize;
    AMediaCodecBufferInfo info;

    ssize_t bufIndex = AMediaCodec_dequeueOutputBuffer(mCodec, &info, -1);
    if (bufIndex >= 0) {
        int colorFormat = this->colorFormat();
        int width = this->width();
        int height = this->height();
        if (colorFormat == 21) {
            // Happy path!
        } else if (colorFormat == 19) {
            LOGW("Color format is not NV21, using slow conversion");
        } else {
            LOGE("Unknown color format %d, cannot convert to YUV", colorFormat);
            return false;
        }

        if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) mCodecEOF = true;
        unsigned char *buffer = AMediaCodec_getOutputBuffer(mCodec, bufIndex, &bufSize);
        frame.resize(width, height);
        mFrames++;

        if (colorFormat == 19) {
#ifdef HAS_OPENCV
            cv::Mat srcY(height, width, CV_8UC1, buffer);
            cv::Mat srcU(height/2, width/2, CV_8UC1, buffer + width*height);
            cv::Mat srcV(height/2, width/2, CV_8UC1, buffer + width*height + width*height/4);
            cv::Mat destY(height, width, CV_8UC1, frame.data());
            cv::Mat destUV(height/2, width/2, CV_8UC2, frame.data() + width*height);

            cv::Mat srcs[] = {srcY, srcU, srcV};
            cv::Mat dests[] = {destY, destUV};
            int from_to[] = {0,0, 1,2, 2,1};
            cv::mixChannels(srcs, 3, dests, 2, from_to, 3);
#else
            // Copy y channel unchanged
            std::copy_n(buffer, width*height, frame.data());
            // Interlace U and V channels (V first, then U)
            const unsigned char* srcU = buffer+width*height;
            const unsigned char* srcV = srcU + width*height/4;
            unsigned char* destUV = frame.data() + width*height;
            size_t offsetv = frame.uvwidth()*frame.uvheight();
            for (int r=0; r != frame.uvheight(); ++r) {
                for (int c=0; c != frame.uvwidth(); ++c) {
                    destUV[r*width + c*2 + 0] = srcU[r*width/2 + c];
                    destUV[r*width + c*2 + 1] = srcV[r*width/2 + c];
                }
            }
#endif
        } else {
            std::copy_n(buffer, frame.size(), frame.data());
        }

        media_status_t status = AMediaCodec_releaseOutputBuffer(mCodec, bufIndex, false);
        if (status != AMEDIA_OK) {
            LOGE("Failed to release output buffer, error = %d", static_cast<int>(status));
            return false;
        }
    } else if (bufIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        mFormat = AMediaCodec_getOutputFormat(mCodec);
        LOGD("Codec format changed to %s\n", AMediaFormat_toString(mFormat));
        // store frame number when output format is changed.
        mFormatChangeFrameNumber = mFrames;
    } else if (bufIndex == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        LOGD("Codec buffers changed\n");
    } else if (bufIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
        LOGD("Codec try again later\n");
    } else {
        LOGE("Unexpected info code: %zd\n", bufIndex);
        return false;
    }
    return true;
}