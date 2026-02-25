//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "ClipEncoderCore"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include "ThorUtils.h"
#include "ClipEncoderCore.h"

//-----------------------------------------------------------------------------
ClipEncoderCore::ClipEncoderCore() :
    mFrameRate(30.0),
    mIFrameInterval(5),
    mMimeType("video/avc"),
    mAudioMimeType("audio/mp4a-latm"),
    mOutFilePath("/sdcard/echonous/test/clipEncoderTestOut.mp4"),    
    mWidth(1280),
    mHeight(720),
    mBitRate(5000000),
    mSampleRate(8000),
    mAudioBitRate(96000),
    mInputSurface(nullptr),
    mIsAudioEnabled(false),
    mFrameIntervalUs(33000),
    mPresentationTime(0),
    mStartPresentationTime(0)
{
    ALOGD("ClipEncoderCore instance created");

    initializeEncoder();
}

//-----------------------------------------------------------------------------
ClipEncoderCore::~ClipEncoderCore()
{
    // clean up stuff
    if (mEncoder != nullptr) {
        AMediaCodec_stop(mEncoder);
        AMediaCodec_delete(mEncoder);
        mEncoder = nullptr;
    }

    if (mAudioEncoder != nullptr) {
        AMediaCodec_stop(mAudioEncoder);
        AMediaCodec_delete(mAudioEncoder);
        mAudioEncoder = nullptr;
    }

    if (mMuxer != nullptr)
    {
        AMediaMuxer_stop(mMuxer);
        AMediaMuxer_delete(mMuxer);
        mMuxer = nullptr;
    }

    // close file
    if (mOutputFd)
    {
        close(mOutputFd);
    }

    ALOGD("%s", __func__);
}

//-----------------------------------------------------------------------------
ClipEncoderCore::ClipEncoderCore(int width, int height, int bitRate, const char* outPath) :
        mFrameRate(30.0),
        mIFrameInterval(5),
        mMimeType("video/avc"),
        mAudioMimeType("audio/mp4a-latm"),
        mSampleRate(8000),
        mAudioBitRate(96000),
        mInputSurface(nullptr),
        mIsAudioEnabled(false)
{
    mWidth = width;
    mHeight = height;
    mBitRate = bitRate;

    mOutFilePath = outPath;

    ALOGD("ClipEncoderCore instance created");

    initializeEncoder();
}

//-----------------------------------------------------------------------------
ClipEncoderCore::ClipEncoderCore(int width, int height, int bitRate, float frameRate,
                                 int IFrameInterval, const char* outPath, bool isAudioEnabled) :
        mMimeType("video/avc"),
        mAudioMimeType("audio/mp4a-latm"),
        mSampleRate(8000),
        mAudioBitRate(96000),
        mInputSurface(nullptr)
{
    mWidth = width;
    mHeight = height;
    mBitRate = bitRate;
    mFrameRate = frameRate;
    mIFrameInterval = IFrameInterval;
    mIsAudioEnabled = isAudioEnabled;

    mOutFilePath = outPath;

    ALOGD("ClipEncoderCore instance created");

    initializeEncoder();
}

//-----------------------------------------------------------------------------
ClipEncoderCore::ClipEncoderCore(int width, int height, int bitRate, float frameRate,
                                 int IFrameInterval, int sampleRate, int audioBitRate,
                                 const char* outPath, bool isAudioEnabled) :
        mMimeType("video/avc"),
        mAudioMimeType("audio/mp4a-latm"),
        mInputSurface(nullptr)
{
    mWidth = width;
    mHeight = height;
    mBitRate = bitRate;
    mFrameRate = frameRate;
    mIFrameInterval = IFrameInterval;
    mSampleRate = sampleRate;
    mAudioBitRate = audioBitRate;
    mIsAudioEnabled = isAudioEnabled;

    mOutFilePath = outPath;

    ALOGD("ClipEncoderCore instance created");

    initializeEncoder();
}

//-----------------------------------------------------------------------------
void ClipEncoderCore::initializeEncoder()
{
    AMediaFormat *format = AMediaFormat_new();

    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mMimeType);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, mWidth);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, mHeight);

    //AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, colorFormat);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, 2130708361);       // Color_FormatSurface
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, mBitRate);
    AMediaFormat_setFloat(format, AMEDIAFORMAT_KEY_FRAME_RATE, mFrameRate);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, mIFrameInterval);

    AMediaFormat_setInt32(format, "profile", 8);        // AVCProfileHigh
    AMediaFormat_setInt32(format, "level", 4096);       // AVCLevel41

    const char *s = AMediaFormat_toString(format);    

    const char *mime;
    AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime);    

    mEncoder = AMediaCodec_createEncoderByType(mime);

    auto status = AMediaCodec_configure(mEncoder, format, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);

    if (status != AMEDIA_OK) {
        ALOGE("failed to configure encoder");
        return;
    }

    AMediaCodec_createInputSurface(mEncoder, &mInputSurface);

    mTotalNumTracks = 1;

    if (mIsAudioEnabled)
    {
        AMediaFormat *formatAudio = AMediaFormat_new();

        AMediaFormat_setString(formatAudio, AMEDIAFORMAT_KEY_MIME, mAudioMimeType);
        AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_AAC_PROFILE, 2);     // LC
        AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_SAMPLE_RATE, mSampleRate);
        AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_CHANNEL_COUNT, 2);   // Stereo
        AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_BIT_RATE, mAudioBitRate);

        const char *audioMime;
        AMediaFormat_getString(formatAudio, AMEDIAFORMAT_KEY_MIME, &audioMime);

        mAudioEncoder = AMediaCodec_createEncoderByType(audioMime);

        auto statusAE = AMediaCodec_configure(mAudioEncoder, formatAudio, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);

        if (statusAE == AMEDIA_OK) {
            // AudioEncoder configure successful
            ALOGD("AudioEncoder configured successfully");
            mTotalNumTracks++;
        }
        else
        {
            ALOGE("failed to configure audio encoder");
            if (mAudioEncoder != nullptr) {
                AMediaCodec_delete(mAudioEncoder);
                mAudioEncoder = nullptr;
            }
        }
    }
    else
    {
        mAudioEncoder = nullptr;
    }

    // reset num of tracks added.
    mNumTracksAdded = 0;

    // reset start presentation time
    mStartPresentationTime = 0;

    // start
    AMediaCodec_start(mEncoder);            // codec / encoder
    if (mAudioEncoder != nullptr)
    {
        AMediaCodec_start(mAudioEncoder);
    }
    ALOGD("Encoder Started");

    mOutputFd = open(mOutFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    // Muxer
    mMuxer = AMediaMuxer_new(mOutputFd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);

    // initialize params
    mTrackIndex = -1;
    mAudioTrackIndex = -1;
    mMuxerStarted = false;

    // frameRate and frameInterval
    mFrameIntervalUs = (int64_t) (1000000/mFrameRate);

    if (mFrameIntervalUs < 1000)
    {
        // FrameRate is too high.
        // change to default 33000 to prevent potential crashes
        ALOGE("FrameIntervalUs is too low: %jd", mFrameIntervalUs);
        mFrameIntervalUs = 33000;
    }
}

//-----------------------------------------------------------------------------
ANativeWindow* ClipEncoderCore::getInputSurface()
{
    return mInputSurface;
}

//-----------------------------------------------------------------------------
void ClipEncoderCore::cleanUpEncoder() 
{
    // placeholder for clean up
    if (mEncoder != nullptr) {
        AMediaCodec_stop(mEncoder);
        AMediaCodec_delete(mEncoder);
    }

    if (mAudioEncoder != nullptr) {
        AMediaCodec_stop(mAudioEncoder);
        AMediaCodec_delete(mAudioEncoder);
    }
}

//-----------------------------------------------------------------------------
void ClipEncoderCore::drainEncoder(bool endOfStream)
{
    drainEncoder(mEncoder, mTrackIndex, true, endOfStream);
}

//-----------------------------------------------------------------------------
void ClipEncoderCore::drainAudioEncoder(bool endOfStream)
{
    if (mAudioEncoder != nullptr)
        drainEncoder(mAudioEncoder, mAudioTrackIndex, false, endOfStream);
}

//-----------------------------------------------------------------------------
void ClipEncoderCore::putAudioData(short *inData, int lengthInByte, uint64_t presentationTime, uint64_t frameIntervalUs,
                                    bool endOfStream)
{
    int         TIMEOUT_USEC = 10000;
    ssize_t     bufidx = -1;
    size_t      bufsize;
    uint32_t    flags = 0;
    int         remLengthInByte = 0;
    int         inputLengthInByte;
    bool        encodeEndOfStream;

    AMediaCodecBufferInfo bufInfo;

    // previously encoded data
    drainEncoder(mAudioEncoder, mAudioTrackIndex, false, false);

    bufidx  = AMediaCodec_dequeueInputBuffer(mAudioEncoder, TIMEOUT_USEC);
    uint8_t* buf = AMediaCodec_getInputBuffer(mAudioEncoder, bufidx, &bufsize);

    inputLengthInByte = lengthInByte;
    encodeEndOfStream = endOfStream;

    if (lengthInByte > bufsize)
    {
        ALOGD("InputData is too big - bufsize: %d, lengthInByte: %d", bufsize, lengthInByte);
        inputLengthInByte = bufsize;
        remLengthInByte = lengthInByte - inputLengthInByte;
        encodeEndOfStream = false;

        ALOGD("Adjusted Input lengthIbByte to: %d", inputLengthInByte);
    }

    // put data into buffer
    std::memcpy(buf, (uint8_t*) inData, inputLengthInByte);

    if (encodeEndOfStream)
    {
        flags = AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM;
    }

    AMediaCodec_queueInputBuffer(mAudioEncoder, bufidx, 0, inputLengthInByte, mStartPresentationTime + presentationTime, flags);

    if (remLengthInByte > 0)
    {
        uint8_t* inPtr;
        uint64_t updatedPresentationTime;
        uint64_t unpdatedFrameIntervalUs;

        inPtr = ((u_int8_t *) inData) + inputLengthInByte;
        updatedPresentationTime = presentationTime + frameIntervalUs * inputLengthInByte / lengthInByte;
        unpdatedFrameIntervalUs = frameIntervalUs * remLengthInByte / lengthInByte;

        putAudioData((short*) inPtr, remLengthInByte, updatedPresentationTime, unpdatedFrameIntervalUs, endOfStream);
    }
}

//-----------------------------------------------------------------------------
void ClipEncoderCore::drainEncoder(AMediaCodec* encoder, ssize_t& trackIndex, bool updateBufferTime, bool endOfStream)
{
    int TIMEOUT_USEC = 10000;

    if (endOfStream) {
        AMediaCodec_signalEndOfInputStream(encoder);
    }

    size_t bufsize;

    // loop
    while (true) {
        AMediaCodecBufferInfo bufInfo;
        bufsize = 0;

        ssize_t status = AMediaCodec_dequeueOutputBuffer(encoder, &bufInfo, TIMEOUT_USEC);

        if (mStartPresentationTime == 0)
        {
            mPresentationTime = bufInfo.presentationTimeUs;
            mStartPresentationTime = mPresentationTime;

            if (mStartPresentationTime < 1)
            {
                // Something is wrong in presentationTime value. This should not happen.
                ALOGW("presentationTimeUs value does not look right!");
                // setting up the presentationTime with a random but legit value.
                mPresentationTime = 528326050128;
                mStartPresentationTime = mPresentationTime;
            }

            ALOGD("StartTimeStampUs: %jd", mPresentationTime);
        }

        if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            if (!endOfStream) {
                break;      // getting out of while
            } else
            {
                ALOGD("no output available, spinning to await EOS");
            }
        }
        else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            // not expected for an encoder
            ALOGW("MediaCodec info output buffer changed occured.  This is not expected for an encoder");
            auto buf = AMediaCodec_getOutputBuffer(encoder, status, &bufsize);
        }
        else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            // should only happen once for each encoder
            if (mMuxerStarted) {
                // TODO: handle error
                ALOGE("THIS SHOULD NOT HAPPEN");
            }

            AMediaFormat* newformat = AMediaFormat_new();
            newformat = AMediaCodec_getOutputFormat(encoder);

            const char *s = AMediaFormat_toString(newformat);
            ALOGD("Encoder output format changed: %s", s);

            trackIndex = AMediaMuxer_addTrack(mMuxer, newformat);
            mNumTracksAdded++;

            if (mNumTracksAdded == mTotalNumTracks)
            {
                auto muxStatus = AMediaMuxer_start(mMuxer);
                mMuxerStarted = true;
                ALOGD("All tracks added.  Muxer started - status: %d", muxStatus);
            }
        }
        else if (status < 0) {
            ALOGW("unexpected result from encoder dequeueoutput buffer");
        }
        else {
            // encoder data available
            uint8_t* buf = AMediaCodec_getOutputBuffer(encoder, status, &bufsize);

            if (buf == NULL) {
                // TODO: handle error
                ALOGE("Encoder output buffer is NULL");
            }

            if (bufInfo.size != 0) {
                if (!mMuxerStarted) {
                    // This could happen when Audio Data present.
                    // First 2 bytes of audio data should be skipped.
                    ALOGW("Muxer has not started - skipping output %d byte(s)", bufInfo.size);
                    continue;
                }

                if (bufInfo.presentationTimeUs != 0 && updateBufferTime)
                {
                    bufInfo.presentationTimeUs = mPresentationTime;
                    mPresentationTime += mFrameIntervalUs;
                }

                auto muxStatus = AMediaMuxer_writeSampleData(mMuxer, trackIndex, buf, &bufInfo);
            }

            // writing to an output file
            auto mcStatus = AMediaCodec_releaseOutputBuffer(encoder, status, false);

            if ((bufInfo.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) {
                if (!endOfStream) {
                    ALOGW("Reached End of Stream unexpectedly!!!");
                } else
                {
                    // verbose
                    ALOGD("End of stream reached");
                }
                break;  // out of while
            }
        }
    } // end of while

}


