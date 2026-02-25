//
// Copyright 2018 EchoNous Inc.
//
#pragma once

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"
#include "media/NdkMediaFormat.h"
#include "media/NdkMediaMuxer.h"
#include "media/NdkMediaCrypto.h"

class ClipEncoderCore
{
public:     // Method
    ClipEncoderCore();
    virtual ~ClipEncoderCore();

    ClipEncoderCore(int width, int height, int bitRate, const char *outPath);

    ClipEncoderCore(int width, int height, int bitRate, float frameRate,
                    int IFrameInterval, const char* outPath, bool isAudioEnabled = false);

    ClipEncoderCore(int width, int height, int bitRate, float frameRate,
                    int IFrameInterval, int SampleRate, int audioBitRate,
                    const char* outPath, bool isAudioEnabled);

    void                drainEncoder(bool endOfStream);
    void                drainEncoder(AMediaCodec* encoder, ssize_t& trackIndex, bool updateBufferTime,
                                     bool endOfStream);
    void                drainAudioEncoder(bool endOfStream);

    // putAudioData calls drainEncoder as well
    void                putAudioData(short* inData, int lengthInByte, uint64_t presentationTime, uint64_t frameIntervalUs,
                        bool endOfStream);

    ANativeWindow*      getInputSurface();

private:
    void    initializeEncoder();

    void    cleanUpEncoder();

private:
    float                       mFrameRate;
    int                         mIFrameInterval;
    const char*                 mMimeType;
    const char*                 mAudioMimeType;
    int                         mWidth;
    int                         mHeight;
    int                         mBitRate;
    int                         mSampleRate;
    int                         mAudioBitRate;
    const char*                 mOutFilePath;

    ANativeWindow*              mInputSurface;
    AMediaCodec*                mEncoder;
    AMediaCodec*                mAudioEncoder;
    AMediaMuxer*                mMuxer;

    ssize_t                     mTrackIndex;
    ssize_t                     mAudioTrackIndex;
    int                         mTotalNumTracks;
    int                         mNumTracksAdded;
    bool                        mMuxerStarted;
    bool                        mIsAudioEnabled;

    int                         mOutputFd;
    int64_t                     mFrameIntervalUs;
    int64_t                     mPresentationTime;
    int64_t                     mStartPresentationTime;
};

