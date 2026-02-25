#define LOG_TAG "PIP"

#include "ParallelImageProcessor.h"
#include <DauInputManager.h>

ParallelImageProcessor::ParallelImageProcessor(unsigned int numThreads) :
    mNumProcessThreads(numThreads),
    mInitialized(false),
    mShutdown(false),
    mFramesInProgress(0)
{
    mFrameToProcess.hasValue = false;
    mFrameToNotify.hasValue = false;

    mProfiler.addSection("enqueue dispatch");
    mProfiler.addSection("dequeue dispatch");
}

ParallelImageProcessor::~ParallelImageProcessor()
{
    terminate();
}

void ParallelImageProcessor::setCineBuffer(CineBuffer *cinebuffer)
{
    assert(!mInitialized);
    mCineBuffer = cinebuffer;
}

ThorStatus ParallelImageProcessor::init()
{
    assert(!mInitialized);
    ALOGD("ParallelImageProcessor::init(threads = %hhu)", mNumProcessThreads);

    auto l = lock();
    mInitialized = true;
    mShutdown = false;
    mFlush = false;

    // Create worker threads
    for (uint8_t tid=0; tid != mNumProcessThreads; ++tid)
    {
        // std::thread ctor can take arguments, so use this pointer and the thread id
        //   binding this pointer to an instance method calls that method on the given instance
        mThreads.emplace_back(&ParallelImageProcessor::processThread, this, tid);
    }

    mThreads.emplace_back(&ParallelImageProcessor::notifyThread, this);
    // non portable call to make the notify thread higher priority
    //auto pthread = mThreads.back().native_handle();
    //pthread_setschedparam(pthread, )

    for (auto &stagePtr : mStages)
    {
        ThorStatus status = stagePtr->init();
        if (IS_THOR_ERROR(status))
            return status;
        stagePtr->setFramesCompleted(0U-1U);
    }

    // Linked Processors
    mLinkedProcs.clear();

    mProfiler.addSection("enqueue notify");
    mProfiler.addSection("dequeue notify");
    mProfiler.addSection("perform notify");
    mProfiler.setMaxFrames(120);

    return THOR_OK;
}

void ParallelImageProcessor::flush()
{
    assert(mInitialized && !mShutdown);
    auto l = flushLock();
    mFlush = true;
    l.unlock();
}

void ParallelImageProcessor::setFramesCompleted(uint32_t frameNum)
{
    ALOGD("ParallelImageProcessor::setFramesCompleted(%u)", frameNum);
    assert(mInitialized && !mShutdown);
    auto l = flushLock();
    for (auto &stagePtr : mStages)
    {
        stagePtr->setFramesCompleted(frameNum);
    }
    mFlush = false;
}

void ParallelImageProcessor::terminate()
{
    ALOGD("ParallelImageProcessor::terminate()");
    auto l = flushLock();
    if (mShutdown)
    {
        return;
    }
    mShutdown = true;
    l.unlock();

    // These are the two condition variables which threads may be waiting on
    mFrameToProcess.added.notify_all();
    mFrameToNotify.added.notify_all();

    for (auto &thread : mThreads)
        thread.join();
    mThreads.clear();

    for (auto &stagePtr : mStages)
        stagePtr->terminate();

    mInitialized = false;
}

ThorStatus ParallelImageProcessor::setProcessParams(ImageProcess::ProcessParams &params)
{
    ALOGD("ParallelImageProcessor::setProcessParams({imagingCase=%u, imagingMode=%u, samples=%u, rays=%u})",
        params.imagingCaseId, params.imagingMode, params.numSamples, params.numRays);
    assert(!mShutdown);
    auto l = flushLock();
    ThorStatus status = THOR_OK;
    for (auto &stagePtr : mStages)
    {
        status = stagePtr->setProcessParams(params);
        if (IS_THOR_ERROR(status))
        {
            ALOGE("Error in setProcessParams: 0x%08x", status);
            return status;
        }
    }
    return status;
}

void ParallelImageProcessor::processImageAsync(uint8_t *inputPtr, uint32_t frameNum, uint32_t dauDataType)
{
    if (frameNum % 120 == 0)
    {
        ALOGD("ParallelImageProcessor::processImageAsync(%p, frameNum=%u, imagingMode=%u)",
            inputPtr, frameNum, dauDataType);
    }

    mProfiler.startNextSection(frameNum); // enqueue dispatch

    assert(mInitialized && !mShutdown);
    auto l = lock();
    if (mFlush)
        return;

    while (mFrameToProcess.hasValue)
        mFrameToProcess.removed.wait(l);

    // Add the frame to the handoff/queue, and increment frames in progress
	mFrameToProcess.value = Frame{inputPtr, nullptr, frameNum, dauDataType};
	mFrameToProcess.hasValue = true;
    ++mFramesInProgress;
    mProfiler.startNextSection(frameNum); // dequeue dispatch
    l.unlock();

    mFrameToProcess.added.notify_one();
}

void ParallelImageProcessor::processImageSync(uint8_t *inputPtr, uint8_t *outputPtr,
                                                uint32_t frameNum, uint32_t dauDataType)
{
    if (frameNum % 120 == 0)
    {
        ALOGD("ParallelImageProcessor::processImageSync(%p, %p, frameNum=%u, imagingMode=%u)",
              inputPtr, outputPtr, frameNum, dauDataType);
    }

    mProfiler.startNextSection(frameNum); // enqueue dispatch

    assert(mInitialized && !mShutdown);
    auto l = lock();
    while (mFrameToProcess.hasValue)
        mFrameToProcess.removed.wait(l);

    // Add the frame to the handoff/queue, and increment frames in progress
    mFrameToProcess.value = Frame{inputPtr, outputPtr, frameNum, dauDataType};
    mFrameToProcess.hasValue = true;
    ++mFramesInProgress;
    mProfiler.startNextSection(frameNum); // dequeue dispatch
    l.unlock();

    mFrameToProcess.added.notify_one();

    // flushLock to wait until the frame process is complete.
    flushLock();
}

void ParallelImageProcessor::processImageAsyncLinked(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint32_t dauDataType)
{
    if (frameNum % 120 == 0)
    {
        ALOGD("ParallelImageProcessor::processImageSync(%p, %p, frameNum=%u, imagingMode=%u)",
              inputPtr, outputPtr, frameNum, dauDataType);
    }

    mProfiler.startNextSection(frameNum); // enqueue dispatch

    assert(mInitialized && !mShutdown);
    auto l = lock();
    while (mFrameToProcess.hasValue)
        mFrameToProcess.removed.wait(l);

    // Add the frame to the handoff/queue, and increment frames in progress
    mFrameToProcess.value = Frame{inputPtr, outputPtr, frameNum, dauDataType};
    mFrameToProcess.hasValue = true;
    ++mFramesInProgress;
    mProfiler.startNextSection(frameNum); // dequeue dispatch
    l.unlock();

    mFrameToProcess.added.notify_one();
}

void ParallelImageProcessor::linkParallelProcs(ParallelImageProcessor* linkProc,
        uint8_t *inputPtr, uint32_t frameNum, uint32_t dauDataType)
{
    std::pair<ParallelImageProcessor*, Frame> inputPair;
    inputPair.first = linkProc;
    inputPair.second = Frame{inputPtr, nullptr, frameNum, dauDataType};

    auto l = lock();
    mLinkedProcs.emplace_back(inputPair);
}

std::unique_lock<std::mutex> ParallelImageProcessor::lock()
{
    return std::unique_lock<std::mutex>(mMutex);
}

std::unique_lock<std::mutex> ParallelImageProcessor::flushLock()
{
    auto l = lock();
    while (mFramesInProgress != 0)
        mFrameCompleted.wait(l);
    return l;
}

ThorStatus ParallelImageProcessor::processImage(Frame &frame, uint8_t threadNum, uint8_t *tmpA, uint8_t *tmpB)
{
    if (frame.frameNum % 120 == 0)
    {
        ALOGD("ParallelImageProcessor::processImage({%p, frameNum=%u, dauDataType=%u}, thread=%hhu, tmps=[%p,%p])",
            frame.inputPtr, frame.frameNum, frame.dauDataType, threadNum, tmpA, tmpB);
    }

    // Get CineBuffer output ptr, set the frameNum and timestamp fields
    uint8_t *outputPtr = mCineBuffer->getFrame(frame.frameNum, frame.dauDataType, CineBuffer::FrameContents::IncludeHeader);

    if (frame.outputPtr != nullptr) {
        outputPtr = frame.outputPtr;
    }

    CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr);
    cineHeaderPtr->frameNum = frame.frameNum;
    cineHeaderPtr->timeStamp = reinterpret_cast<const DauInputManager::FrameHeader *>(frame.inputPtr)->timeStamp;

    // tmpA & B header
    CineBuffer::CineFrameHeader* cineHeaderPtrA = reinterpret_cast<CineBuffer::CineFrameHeader*>(tmpA - sizeof(CineBuffer::CineFrameHeader));
    CineBuffer::CineFrameHeader* cineHeaderPtrB = reinterpret_cast<CineBuffer::CineFrameHeader*>(tmpB - sizeof(CineBuffer::CineFrameHeader));
    cineHeaderPtrA->frameNum = frame.frameNum;
    cineHeaderPtrB->frameNum = frame.frameNum;

    outputPtr += sizeof(CineBuffer::CineFrameHeader);

    size_t numActiveStages = 0;
    for (auto &s : mStages)
    {
        if (s->isEnabled())
            ++numActiveStages;
    }

    if (0 == numActiveStages)
    {
        // Trivial case, why are we even using this processor in the first place?
        LOGW("ParallelImageProcessor has no stages, is this intended?");
        // Do nothing, we don't know how large the image is, so no action is safe.
        return TER_IE_NO_ACTIVE_STAGES;
    }
    else if (1 == numActiveStages) // If only one operation, go directly from inputPtr to CineBuffer
    {
        auto stageIt = mStages.begin();
        while (!(*stageIt)->isEnabled())
            ++stageIt;

        ThorStatus status = (*stageIt)->process(frame.inputPtr, outputPtr, frame.frameNum, threadNum, mProfiler);

        if (IS_THOR_ERROR(status))
            return status;
    }
    else
    {
        // first operation goes from inputPtr to tmpA
        auto stageIt = mStages.begin();
        while (!(*stageIt)->isEnabled())
            ++stageIt;
        ThorStatus status = (*stageIt)->process(frame.inputPtr, tmpA, frame.frameNum, threadNum, mProfiler);
        ++stageIt;

        if (IS_THOR_ERROR(status))
            return status;

        // Middle operations go from tmpA -> tmpB, then swaps so that tmpA always holds the last output
        for (size_t numProcessedStages=1; numProcessedStages != numActiveStages-1; ++stageIt)
        {
            if (!(*stageIt)->isEnabled())
                continue;

            status = (*stageIt)->process(tmpA, tmpB, frame.frameNum, threadNum, mProfiler);
            ++numProcessedStages;
            std::swap(tmpA, tmpB);

            if (IS_THOR_ERROR(status))
                return status;
        }

        // last operation goes from tmpA to outputPtr
        while (!(*stageIt)->isEnabled())
            ++stageIt;
        status = (*stageIt)->process(tmpA, outputPtr, frame.frameNum, threadNum, mProfiler);

        if (IS_THOR_ERROR(status))
            return status;
    }

    return THOR_OK;
}

void ParallelImageProcessor::processThread(uint8_t threadNum)
{
    // Create temporary buffers for this thread
    // TODO: Make these an appropriate size??
    uint32_t cineFrameHeaderSize = sizeof(CineBuffer::CineFrameHeader);
    std::vector<uint8_t> tmpA(MAX_B_FRAME_SIZE + cineFrameHeaderSize);
    std::vector<uint8_t> tmpB(MAX_B_FRAME_SIZE + cineFrameHeaderSize);

    while (true)
    {
        // Get the next frame
        Frame frame;
        {
            auto l = lock();
            while (!mShutdown && !mFrameToProcess.hasValue)
                mFrameToProcess.added.wait(l);
            if (mShutdown)
            {
                ALOGD("Shutting down processor thread %hhu\n", threadNum);
                return;
            }
            // remove the frame from the handoff/queue
            frame = mFrameToProcess.value;
            mFrameToProcess.hasValue = false;
        }
        mFrameToProcess.removed.notify_one();

        // Process the frame
        ThorStatus status = processImage(frame, threadNum, &tmpA.at(cineFrameHeaderSize), &tmpB.at(cineFrameHeaderSize));
        if (IS_THOR_ERROR(status))
        {
            LOGE("Error during image processing: Frame %u, threadNum %hhu, error 0x%08X\n",
                frame.frameNum, threadNum, status);
            // TODO handle this better?
        }

        mProfiler.startNextSection(frame.frameNum); // enqueue notify

        // Hand the frame off to the notifier
        {
            auto l = lock();
            while (mFrameToNotify.hasValue)
                mFrameToNotify.removed.wait(l);
            mFrameToNotify.value = frame;
            mFrameToNotify.hasValue = true;
            mProfiler.startNextSection(frame.frameNum); // dequeue notify
        }
        mFrameToNotify.added.notify_one();
    }
}

void ParallelImageProcessor::notifyThread()
{
    while (true)
    {
        // Get the next frame
        Frame frame;
        {
            auto l = lock();
            while (!mShutdown && !mFrameToNotify.hasValue)
                mFrameToNotify.added.wait(l);
            if (mShutdown)
            {
                ALOGD("Shutting down notify thread\n");
                return;
            }
            // decrement frames in progress, and remove the frame from the handoff/queue
            --mFramesInProgress;
            frame = mFrameToNotify.value;
            mFrameToNotify.hasValue = false;
        }

        // Notfy the CineBuffer that this frame is complete
        // Note: this should occur BEFORE the two notify calls below,
        // or else some other thread may get to run instead of notifying the CineBuffer as soon as possible
        mProfiler.startNextSection(frame.frameNum); // perform notify

        if (frame.outputPtr == nullptr) {
            mCineBuffer->setFrameComplete(frame.frameNum, frame.dauDataType);
        }

        mProfiler.frameComplete(frame.frameNum);

        mFrameToNotify.removed.notify_one();

        // Linked Processors
        {
            auto l = lock();
            if (!mLinkedProcs.empty()) {
                auto procIt = mLinkedProcs.begin();

                while (procIt != mLinkedProcs.end()) {
                    Frame linkedFrame = procIt->second;
                    ParallelImageProcessor *proc = procIt->first;

                    if (linkedFrame.frameNum == frame.frameNum) {
                        proc->processImageAsync(linkedFrame.inputPtr, linkedFrame.frameNum,
                                                linkedFrame.dauDataType);
                        mLinkedProcs.erase(procIt);
                    } else {
                        ++procIt;
                    }
                }
            }   //end of if
        }

        // notify frame completed.
        mFrameCompleted.notify_one(); // Notify this right away, as processing for that frame has completed.
    }
}

ParallelImageProcessor::IStage::IStage(bool isParallel) :
    mEnabled(true),
    mIsParallel(isParallel)
{}

void ParallelImageProcessor::IStage::setEnabled(bool enabled)
{
    mEnabled = enabled;
    LOGD("Setting stage enabled: %s", enabled ? "true": "false");
}

bool ParallelImageProcessor::IStage::isEnabled() const
{
    return mEnabled;
}

ThorStatus ParallelImageProcessor::IStage::process(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint8_t threadNum, ParallelPipelineProfiler &profiler)
{
    if (mEnabled)
        return processImpl(inputPtr, outputPtr, frameNum, threadNum, profiler);
    else
    {
        // copy from inputPtr to outputPtr??
        return TER_IE_OPERATION_STOPPED;
    }
}