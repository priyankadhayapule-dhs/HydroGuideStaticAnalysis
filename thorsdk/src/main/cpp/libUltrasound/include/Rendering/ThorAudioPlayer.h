//
// Copyright 2018 EchoNous Inc.
//
// Based on AOSP
//
/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef THOR_AUDIOPLAYER_H
#define THOR_AUDIOPLAYER_H

#include <thread>
#include <mutex>
#include <CineBuffer.h>
#include "AudioCommon.h"
#include "AudioBuffer.h"

#define BUFFER_SIZE_AUTOMATIC 0

class ThorAudioPlayer {

public:
    ThorAudioPlayer(CineBuffer* mCineBufferPtr, int32_t desiredSampleRate, int32_t inputFrameSize,
            uint32_t dataType);
    ~ThorAudioPlayer();

    ThorStatus init();
    void setDeviceId(int32_t deviceId);    
    void setInputFrameSize(int32_t inputFrameSize);
    void setBufferSizeInBursts(int32_t numBursts);
    aaudio_data_callback_result_t dataCallback(AAudioStream *stream,
                                             void *audioData,
                                             int32_t numFrames);
    void errorCallback(AAudioStream *stream,
                     aaudio_result_t  __unused error);

    double getCurrentOutputLatencyMillis();

    void putDataFloat(float* sigL, float* sigR, int idxStep = 1);
    void putDataFloat(float* sigLR);        // for mono
    void setFrameNum(int frameNum);
    void setInterleavedStereo(bool isTrue);
    void pausePlayback();

    int32_t  getDesiredSampleRate() { return mDesiredSampleRate; };
    int32_t  getInputFrameSize() { return mInputFrameSize; };
    uint32_t getInputDateType() { return mDataType; };

private:
    std::mutex mLock;
    std::mutex mRestartingLock;

    ThorStatus createPlaybackStream();
    void closeOutputStream();
    void restartStream();

    void requestStart();
    void requestPause();

    AAudioStreamBuilder* createStreamBuilder();
    void setupPlaybackStreamParameters(AAudioStreamBuilder *builder);

    aaudio_result_t calculateCurrentOutputLatencyMillis(AAudioStream *stream, double *latencyMillis);


private:
    int32_t         mPlaybackDeviceId = AAUDIO_UNSPECIFIED;
    int32_t         mSampleRate;
    int32_t         mDesiredSampleRate;
    int16_t         mSampleChannels;
    aaudio_format_t mSampleFormat;
    int32_t         mBufferUnderrunCnt;

    AAudioStream*   mPlayStream;

    int32_t         mPlayStreamUnderrunCount;
    int32_t         mBufSizeInFrames;
    int32_t         mFramesPerBurst;
    double          mCurrentOutputLatencyMillis = 0;
    int32_t         mBufferSizeSelection = BUFFER_SIZE_AUTOMATIC;

    int32_t         mInputFrameSize;
    int32_t         mPrevFrameNum;
    bool            mStartRequested = false;
    uint32_t        mDataType = 0;
    int32_t         mStartBufferSize = 2004;
    bool            mInterleavedStereo = false;

    AudioBuffer*    mAudioBuffer;
    CineBuffer*     mCineBufferPtr;

};


#endif //THOR_AUDIOPLAYER_H
