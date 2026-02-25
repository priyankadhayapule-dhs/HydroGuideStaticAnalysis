/**
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
 //
 // Modified for Echonous Thor prototype
 //

#define LOG_TAG "ThorAudioPlayer"
#define MAX_BUFFER_UNDERRUN_COUNT 5

#define START_BUFFER_SIZE_IN_TIME 0.11f     // ~ 3 frames for 25 fps

#define FRAME_SKIPPING_THRESHOLD 2

#include <assert.h>
#include <inttypes.h>
#include "ThorAudioPlayer.h"
#include <string.h>
#include <ThorUtils.h>

/**
 * Every time the playback stream requires data this method will be called.
 *
 * @param stream the audio stream which is requesting data, this is the playStream_ object
 * @param userData the context in which the function is being called, in this case it will be the
 * ThorAudioPlayer instance
 * @param audioData an empty buffer into which we can write our audio data
 * @param numFrames the number of audio frames which are required
 * @return Either AAUDIO_CALLBACK_RESULT_CONTINUE if the stream should continue requesting data
 * or AAUDIO_CALLBACK_RESULT_STOP if the stream should stop.
 *
 * @see ThorAudioPlayer#dataCallback
 */
aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData,
                                           void *audioData, int32_t numFrames)
{
    assert(userData && audioData);

    ThorAudioPlayer *audioEngine = reinterpret_cast<ThorAudioPlayer *>(userData);
    return audioEngine->dataCallback(stream, audioData, numFrames);
}

/**
 * If there is an error with a stream this function will be called. A common example of an error
 * is when an audio device (such as headphones) is disconnected. In this case you should not
 * restart the stream within the callback, instead use a separate thread to perform the stream
 * recreation and restart.
 *
 * @param stream the stream with the error
 * @param userData the context in which the function is being called, in this case it will be the
 * ThorAudioPlayer instance
 * @param error the error which occured, a human readable string can be obtained using
 * AAudio_convertResultToText(error);
 *
 * @see ThorAudioPlayer#errorCallback
 */
void errorCallback(AAudioStream *stream, void *userData,
                   aaudio_result_t error)
{
    assert(userData);
    ThorAudioPlayer *audioEngine = reinterpret_cast<ThorAudioPlayer *>(userData);
    audioEngine->errorCallback(stream, error);
}

ThorAudioPlayer::ThorAudioPlayer(CineBuffer* cineBufferPtr, int32_t desiredSampleRate, int32_t inputFrameSize,
                                 uint32_t dataType)
{
    ALOGD("ThorAudioPlayer - Constructor");

    mPlaybackDeviceId = 0;
    mSampleChannels = kStereoChannelCount;
    mSampleFormat = AAUDIO_FORMAT_PCM_FLOAT;
    mDesiredSampleRate = desiredSampleRate;
    mCineBufferPtr = cineBufferPtr;
    mPrevFrameNum = -1;
    mInputFrameSize = inputFrameSize;
    mDataType = dataType;

    // start buffer size
    mStartBufferSize = ceil( ((float)desiredSampleRate) * START_BUFFER_SIZE_IN_TIME );

    if (mStartBufferSize < 3 * inputFrameSize)
        mStartBufferSize = 3 * inputFrameSize;

    ALOGD("ThorAudioPlayer - startBufferSize: %d", mStartBufferSize);

    mAudioBuffer = new AudioBuffer(desiredSampleRate, inputFrameSize);

    mStartRequested = false;
    mBufferUnderrunCnt = 0;
    mInterleavedStereo = false;

    mPlayStream = nullptr;
}

ThorAudioPlayer::~ThorAudioPlayer()
{
    ALOGD("Closing output stream.");
    requestPause();
    closeOutputStream();

    delete mAudioBuffer;
}

ThorStatus ThorAudioPlayer::init()
{
    return createPlaybackStream();
}

/**
 * Set the audio device which should be used for playback. Can be set to AAUDIO_UNSPECIFIED if
 * you want to use the default playback device (which is usually the built-in speaker if
 * no other audio devices, such as headphones, are attached).
 *
 * @param deviceId the audio device id, can be obtained through an {@link AudioDeviceInfo} object
 * using Java/JNI.
 */
void ThorAudioPlayer::setDeviceId(int32_t deviceId)
{
    std::lock_guard<std::mutex> lock(mLock);
    mPlaybackDeviceId = deviceId;

    // If this is a different device from the one currently in use then restart the stream
    int32_t currentDeviceId = AAudioStream_getDeviceId(mPlayStream);
    if (deviceId != currentDeviceId)
    {
        ALOGD("Current device ID (%d) is not the same as the assigned device ID (%d)", currentDeviceId, deviceId);
        if (mStartRequested)
        {
            ALOGD("Request restartStream due to the device Id mismatch");
            restartStream();
        }
    }
}

void ThorAudioPlayer::setInputFrameSize(int32_t inputFrameSize)
{
    mInputFrameSize = inputFrameSize;
    ALOGD("Input Frame Size: %d", mInputFrameSize);
}

void ThorAudioPlayer::setInterleavedStereo(bool isIntStereo)
{
    mInterleavedStereo = isIntStereo;
    ALOGD("InterleavedStereo: %d", mInterleavedStereo);
}

/**
 * Creates a stream builder which can be used to construct streams
 * @return a new stream builder object
 */
AAudioStreamBuilder* ThorAudioPlayer::createStreamBuilder()
{
    AAudioStreamBuilder *builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
      ALOGE("Error creating stream builder: %s", AAudio_convertResultToText(result));
    }
    return builder;
}

/**
 * Creates an audio stream for playback. The audio device used will depend on playbackDeviceId_.
 * IMPORTANT: unlike Google's example, this routine creates a stream, but it does not start it.
 */
ThorStatus ThorAudioPlayer::createPlaybackStream()
{
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("CreatePlaybackStream - Start");

    ThorStatus status = THOR_OK;
    AAudioStreamBuilder* builder = createStreamBuilder();

    if (builder != nullptr)
    {
        setupPlaybackStreamParameters(builder);

        aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &mPlayStream);

        if (result == AAUDIO_OK && mPlayStream != nullptr)
        {
            // check that we got PCM_FLOAT format
            if (mSampleFormat != AAudioStream_getFormat(mPlayStream))
            {
                ALOGW("Sample format is not PCM_FLOAT");
            }

            mSampleRate = AAudioStream_getSampleRate(mPlayStream);
            mFramesPerBurst = AAudioStream_getFramesPerBurst(mPlayStream);

            ALOGD("ThorAudioPlayer - DesiredSampleRate: %d, ActualSampleRate: %d", mDesiredSampleRate, mSampleRate);

            // Set the buffer size to the burst size - this will give us the minimum possible latency
            AAudioStream_setBufferSizeInFrames(mPlayStream, mFramesPerBurst);
            mBufSizeInFrames = mFramesPerBurst;

            ALOGD("ThorAudioPlayer - FramesPerBurst: %d, BufferSizeInFrames: %d", mFramesPerBurst, mBufSizeInFrames);

            PrintAudioStreamInfo(mPlayStream);
        }
        else
        {
            ALOGE("Failed to create stream. Error: %s", AAudio_convertResultToText(result));
            status =TER_MISC_ABORT;
        }

        AAudioStreamBuilder_delete(builder);

    }
    else
    {
        ALOGE("Unable to obtain an AAudioStreamBuilder object");
        status = TER_MISC_ABORT;
    }

    ALOGD("PlaybackStream created.");

    return status;
}

void ThorAudioPlayer::pausePlayback()
{
    requestPause();
}

void ThorAudioPlayer::requestPause()
{
    std::lock_guard<std::mutex> lock(mLock);
    if (!mStartRequested)
        return;

    ALOGD("Request to pause the stream");

    if (mPlayStream != nullptr)
    {
        aaudio_stream_state_t streamState = AAudioStream_getState(mPlayStream);

        // if in OPEN, PAUSED, PAUSING states, bypass requestPause
        if ( (streamState != AAUDIO_STREAM_STATE_OPEN) && (streamState != AAUDIO_STREAM_STATE_PAUSING)
                && (streamState != AAUDIO_STREAM_STATE_PAUSED) ) {
            // Pause the stream - the dataCallback function will start being called
            aaudio_result_t result = AAudioStream_requestPause(mPlayStream);
            if (result != AAUDIO_OK) {
                ALOGE("Error pausing stream. %s", AAudio_convertResultToText(result));
            }
        }
    }

    // toggle startRequested flag
    mStartRequested = false;

    // reset audioBuffer indices
    mAudioBuffer->resetIndices();
}

void ThorAudioPlayer::requestStart()
{
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("Request to start the stream");

    // Start the stream - the dataCallback function will start being called
    aaudio_result_t result = AAudioStream_requestStart(mPlayStream);
    if (result != AAUDIO_OK)
    {
      ALOGE("Error starting stream. %s", AAudio_convertResultToText(result));
    }
    // Store the underrun count so we can tune the latency in the dataCallback
    mPlayStreamUnderrunCount = AAudioStream_getXRunCount(mPlayStream);

    // Update the flag to indicate player start has been requested.
    mStartRequested = true;
    mBufferUnderrunCnt = 0;
}

/**
 * Sets the stream parameters which are specific to playback, including device id and the
 * dataCallback function, which must be set for low latency playback.
 * @param builder The playback stream builder
 */
void ThorAudioPlayer::setupPlaybackStreamParameters(AAudioStreamBuilder *builder)
{
    AAudioStreamBuilder_setDeviceId(builder, mPlaybackDeviceId);
    AAudioStreamBuilder_setFormat(builder, mSampleFormat);
    AAudioStreamBuilder_setChannelCount(builder, mSampleChannels);

    // to change sample Rate
    AAudioStreamBuilder_setSampleRate(builder, mDesiredSampleRate);
    ALOGD("Desired Sample Rate: %d", mDesiredSampleRate);

    // We request EXCLUSIVE mode since this will give us the lowest possible latency.
    // If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode.
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setDataCallback(builder, ::dataCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, ::errorCallback, this);
}

void ThorAudioPlayer::closeOutputStream()
{
    std::lock_guard<std::mutex> lock(mLock);
    if (mPlayStream != nullptr)
    {
        aaudio_result_t result = AAudioStream_requestStop(mPlayStream);
        if (result != AAUDIO_OK)
        {
            ALOGE("Error stopping output stream. %s", AAudio_convertResultToText(result));
        }

        // wait until the stream stops
        aaudio_stream_state_t streamState = AAUDIO_STREAM_STATE_UNKNOWN;
        AAudioStream_waitForStateChange(mPlayStream, AAUDIO_STREAM_STATE_STOPPING,
                                        &streamState, 1000 * NANOS_PER_MILLISECOND);

        if (streamState != AAUDIO_STREAM_STATE_STOPPED)
            ALOGD("PlayStream is not in Stopped State.  State: %d", streamState);

        result = AAudioStream_close(mPlayStream);
        if (result != AAUDIO_OK)
        {
            ALOGE("Error closing output stream. %s", AAudio_convertResultToText(result));
        }
    }

    ALOGD("AudioStream Closed");
}

// putting audio signal data
void ThorAudioPlayer::putDataFloat(float* sigL, float* sigR, int idxStep)
{
    // put data into audioBuffer
    int numStored = mAudioBuffer->putDataFloat(sigL, sigR, mInputFrameSize, idxStep);

    if (!mStartRequested)
    {
        if (numStored > mStartBufferSize)
        {
            requestStart();
            ALOGD("Player Start Requested - store buffer size: %d", numStored);
        }
    }
}

// putting mono audio signal data
void ThorAudioPlayer::putDataFloat(float *sigLR)
{
    // put data into audioBuffer
    int numStored = mAudioBuffer->putDataFloat(sigLR, sigLR, mInputFrameSize);

    if (!mStartRequested)
    {
        if (numStored > mStartBufferSize)
        {
            requestStart();
            ALOGD("Player Start Requested - store buffer size: %d", numStored);
        }
    }
}

// setFrameNum
void ThorAudioPlayer::setFrameNum(int frameNum)
{
    // When in the live mode, frameNum always increases.  So, too much jump or reverse meaning
    // browsing in the freeze mode
    if ((frameNum - mPrevFrameNum > FRAME_SKIPPING_THRESHOLD) || (frameNum <= mPrevFrameNum))
    {
        mPrevFrameNum = frameNum;
        // request pause if playing
        if (mStartRequested)
        {
            requestPause();
        }

        return;
    }

    for (int i = mPrevFrameNum+1; i < frameNum + 1; i++)
    {
        if (mInterleavedStereo)
            putDataFloat((float*) mCineBufferPtr->getFrame(i, mDataType),
                         ((float*) (mCineBufferPtr->getFrame(i, mDataType) + sizeof(float))), 2);
        else
            putDataFloat((float*) mCineBufferPtr->getFrame(i, mDataType));
    }

    mPrevFrameNum = frameNum;
}

/**
 * @see dataCallback function at top of this file
 */
aaudio_data_callback_result_t ThorAudioPlayer::dataCallback(AAudioStream *stream,
                                                        void *audioData, int32_t numFrames)
{
    assert(stream == mPlayStream);

    int32_t underrunCount = AAudioStream_getXRunCount(mPlayStream);
    aaudio_result_t bufferSize = AAudioStream_getBufferSizeInFrames(mPlayStream);
    bool hasUnderrunCountIncreased = false;
    bool shouldChangeBufferSize = false;

    if (underrunCount > mPlayStreamUnderrunCount)
    {
        mPlayStreamUnderrunCount = underrunCount;
        hasUnderrunCountIncreased = true;
    }

    if (hasUnderrunCountIncreased && mBufferSizeSelection == BUFFER_SIZE_AUTOMATIC)
    {
        /**
         * This is a buffer size tuning algorithm. If the number of underruns (i.e. instances where
         * we were unable to supply sufficient data to the stream) has increased since the last callback
         * we will try to increase the buffer size by the burst size, which will give us more protection
         * against underruns in future, at the cost of additional latency.
         */
        bufferSize += mFramesPerBurst; // Increase buffer size by one burst
        shouldChangeBufferSize = true;
    }
    else if (mBufferSizeSelection > 0 && (mBufferSizeSelection * mFramesPerBurst) != bufferSize)
    {
        // If the buffer size selection has changed then update it here
        bufferSize = mBufferSizeSelection * mFramesPerBurst;
        shouldChangeBufferSize = true;
    }

    if (shouldChangeBufferSize)
    {
        ALOGD("Setting buffer size to %d", bufferSize);
        bufferSize = AAudioStream_setBufferSizeInFrames(stream, bufferSize);
        if (bufferSize > 0)
        {
            mBufSizeInFrames = bufferSize;
        }
        else
        {
            ALOGE("Error setting buffer size: %s", AAudio_convertResultToText(bufferSize));
        }
    }

    // get Data
    if (!mAudioBuffer->getData(static_cast<float *>(audioData), numFrames))
    {
        memset(static_cast<float *>(audioData), 0, sizeof(float) * 2 * numFrames);

        // buffer underrun - not enough data stored in the buffer
        mBufferUnderrunCnt++;

        if (mBufferUnderrunCnt > MAX_BUFFER_UNDERRUN_COUNT)
        {
            if (mBufferUnderrunCnt%100 == 0)
                ALOGD("Buffer Underrun Count: %d", mBufferUnderrunCnt);
        }
        else
        {
            ALOGD("Buffer Underrun Count: %d", mBufferUnderrunCnt);
        }
    }
    else
    {
        mBufferUnderrunCnt = 0;
    }

    //calculateCurrentOutputLatencyMillis(stream, &mCurrentOutputLatencyMillis);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

/**
 * Calculate the current latency between writing a frame to the output stream and
 * the same frame being presented to the audio hardware.
 *
 * Here's how the calculation works:
 *
 * 1) Get the time a particular frame was presented to the audio hardware
 * @see AAudioStream_getTimestamp
 * 2) From this extrapolate the time which the *next* audio frame written to the stream
 * will be presented
 * 3) Assume that the next audio frame is written at the current time
 * 4) currentLatency = nextFramePresentationTime - nextFrameWriteTime
 *
 * @param stream The stream being written to
 * @param latencyMillis pointer to a variable to receive the latency in milliseconds between
 * writing a frame to the stream and that frame being presented to the audio hardware.
 * @return AAUDIO_OK or a negative error. It is normal to receive an error soon after a stream
 * has started because the timestamps are not yet available.
 */
aaudio_result_t
ThorAudioPlayer::calculateCurrentOutputLatencyMillis(AAudioStream *stream, double *latencyMillis)
{
    // Get the time that a known audio frame was presented for playing
    int64_t existingFrameIndex;
    int64_t existingFramePresentationTime;
    aaudio_result_t result = AAudioStream_getTimestamp(stream,
                                                       CLOCK_MONOTONIC,
                                                       &existingFrameIndex,
                                                       &existingFramePresentationTime);

    if (result == AAUDIO_OK)
    {
        // Get the write index for the next audio frame
        int64_t writeIndex = AAudioStream_getFramesWritten(stream);

        // Calculate the number of frames between our known frame and the write index
        int64_t frameIndexDelta = writeIndex - existingFrameIndex;

        // Calculate the time which the next frame will be presented
        int64_t frameTimeDelta = (frameIndexDelta * NANOS_PER_SECOND) / mSampleRate;
        int64_t nextFramePresentationTime = existingFramePresentationTime + frameTimeDelta;

        // Assume that the next frame will be written at the current time
        int64_t nextFrameWriteTime = get_time_nanoseconds(CLOCK_MONOTONIC);

        // Calculate the latency
        *latencyMillis = (double) (nextFramePresentationTime - nextFrameWriteTime) / NANOS_PER_MILLISECOND;
    }
    else
    {
        ALOGE("Error calculating latency: %s", AAudio_convertResultToText(result));
    }

    return result;
}

/**
 * @see errorCallback function at top of this file
 */
void ThorAudioPlayer::errorCallback(AAudioStream *stream,
                   aaudio_result_t error)
{
    UNUSED(stream);

    ALOGD("ThorAudioPlayer - errorCallBack");
    assert(stream == mPlayStream);
    ALOGD("errorCallback result: %s", AAudio_convertResultToText(error));

    aaudio_stream_state_t streamState = AAudioStream_getState(mPlayStream);
    if (streamState == AAUDIO_STREAM_STATE_DISCONNECTED)
    {
        // Handle stream restart on a separate thread
        std::function<void(void)> restartStream = std::bind(&ThorAudioPlayer::restartStream, this);
        std::thread streamRestartThread(restartStream);
        streamRestartThread.detach();
    }
}

// restarting the stream
// when the deviceID is not matched with specified deviceID, this happens.
void ThorAudioPlayer::restartStream()
{
    ALOGI("Restarting stream");
    if (mRestartingLock.try_lock())
    {
        closeOutputStream();
        createPlaybackStream();
        requestStart();
        mRestartingLock.unlock();
    }
    else
    {
      ALOGW("Restart stream operation already in progress - ignoring this request");
      // We were unable to obtain the restarting lock which means the restart operation is currently
      // active. This is probably because we received successive "stream disconnected" events.
      // Internal issue b/63087953
    }
}

double ThorAudioPlayer::getCurrentOutputLatencyMillis() {
    return mCurrentOutputLatencyMillis;
}

void ThorAudioPlayer::setBufferSizeInBursts(int32_t numBursts) {
    ThorAudioPlayer::mBufferSizeSelection = numBursts;
}
