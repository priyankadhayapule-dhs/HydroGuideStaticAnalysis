#pragma once

#include <cassert>
#include <thread>
#include <typeinfo>
#include <mutex>
#include <vector>

#include "ImageProcess.h"
#include "CineBuffer.h"
#include "ParallelPipelineProfiler.h"

class ParallelImageProcessor
{
private:
    class IStage;

public: // Public classes

// Templated "stage" class allows clients to call arbitrary functions on ImageProcesses
template <typename Proc>
class StageHandle
{
public:
    StageHandle() : mStage(nullptr) {}

    // Invoke a function on each ImageProcess operation in this stage
    // Fn may be a member function using the syntax:
    //   Stage<MyProcess> s = processor.addParallelStage<MyProcess>();
    //   s.each(&MyProcess::setGain, 100);
    // WARNING: This method does NO synchronization. The ImageProcess instance may be processing an image on another thread
    template <typename Fn, typename ...Args>
    void each(Fn&& fn, Args&&... args);

    void setEnabled(bool enabled);
    bool isEnabled() const { return mStage->isEnabled(); }

    operator bool() const { return mStage != nullptr; }

private:
    std::vector<Proc*> mProcesses; // non owning pointers
    IStage *mStage;
    friend class ParallelImageProcessor;
};

public: // Methods

    ParallelImageProcessor(unsigned int numThreads = std::thread::hardware_concurrency());
    ~ParallelImageProcessor();

    // Adds Proc to the pipeline as a serial processing stage.
    //  A single instance of Proc will be created, and that instance will
    //  only process one image at a time.
    template <typename Proc, typename ...Args>
    StageHandle<Proc> addSequentialStage(Args ...args);

    // Adds Proc to the pipeline as a parallel proccessing stage.
    //  Potentially multiple instances of Proc will be created. Each instance
    //  will only process one image at a time, but multiple instances may run
    //  concurrently.
    //  This way, each instance may have instance state such as memory, so long
    //  as there is no temporal dependency.
    template <typename Proc, typename ...Args>
    StageHandle<Proc> addParallelStage(Args ...args);

    // Set the output CineBuffer for this processor. Must be called before init.
    void setCineBuffer(CineBuffer *cinebuffer);

    // Initialize all processor stages
    //   After initialization, stages cannot be added, and the cinebuffer cannot be changed
    ThorStatus init();

    // Flush the pipeline, blocks until all processing operations are complete
    void flush();

    // Sets the pipeline to consider all frames up to and including frameNum as complete.
    //   Triggers an implicit flush. Can be called any time (before or after init)
    void setFramesCompleted(uint32_t frameNum);

    // Terminates the pipeline. Existing frames finish processing, then all ImageProcess instances
    //   are shut down, and all worker threads are terminated.
    void terminate();

    // Sets the process params for all processor stages
    //   This triggers an implicit flush of the pipeline
    ThorStatus setProcessParams(ImageProcess::ProcessParams &params);

    // Process an image asynchronously. This function blocks until the frame is dispatched
    //   to a worker thread.
    void processImageAsync(uint8_t *inputPtr, uint32_t frameNum, uint32_t dauDataType);

    // Process an image synchronously.  This function blocks until the frame is processed.
    void processImageSync(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint32_t dauDataType);

    // Process an image asynchronously.
    void processImageAsyncLinked(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint32_t dauDataType);

    // Link another parallel image processor to ProcessImageAsync
    void linkParallelProcs(ParallelImageProcessor* procPrt, uint8_t* inputPtr, uint32_t frameNum, uint32_t dauDataType);

    template <typename Proc>
    StageHandle<Proc> getStage(); // Gets the stage handle for a given ImageProcess type

private: // Internal Classes/structs

    // Stage interface
    class IStage
    {
    public:
        IStage(bool isParallel);
        virtual ~IStage() = default;

        // Forward the given params to all processes (no locking/thread safety)
        virtual ThorStatus setProcessParams(ImageProcess::ProcessParams &params) = 0;

        virtual ThorStatus init() = 0;
        virtual void terminate() = 0;

        virtual const char* name() const = 0; // Gets the name of the process

        void setEnabled(bool enabled);
        bool isEnabled() const;

        ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint8_t threadNum, ParallelPipelineProfiler &profiler);

        virtual void setFramesCompleted(uint32_t frameNum) {UNUSED(frameNum);}

        // Get a client facing handle for this stage
        template <typename Proc>
        StageHandle<Proc> handle();

    protected:
        virtual ThorStatus processImpl(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint8_t threadNum, ParallelPipelineProfiler &profiler) = 0;
    private:
        bool mEnabled;
        bool mIsParallel;
    };

    // Implementation of stage for parallel stages
    template <typename Proc>
    class ParallelStage : public IStage
    {
    public:
        template <typename ...Args>
        ParallelStage(uint8_t numThreads, Args&&... args);

        virtual ThorStatus setProcessParams(ImageProcess::ProcessParams &params) override;
        virtual ThorStatus init() override;
        virtual void terminate() override;

        virtual const char* name() const override { return Proc::name(); }

        // Get a client facing handle for this stage
        StageHandle<Proc> handle();

    protected:
        virtual ThorStatus processImpl(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint8_t threadNum, ParallelPipelineProfiler &profiler) override;

    private:
        std::vector<Proc> mProcesses;
    };

    // Implementation of stage for sequential stages
    template <typename Proc>
    struct SequentialStage : public IStage
    {
    public:
        template <typename ...Args>
        SequentialStage(Args&&... args);

        virtual ThorStatus setProcessParams(ImageProcess::ProcessParams &params) override;
        virtual ThorStatus init() override;
        virtual void terminate() override;

        virtual const char* name() const override { return Proc::name(); }

        // Get a client facing handle for this stage
        StageHandle<Proc> handle();

        virtual void setFramesCompleted(uint32_t frameNum) override;

    protected:
        virtual ThorStatus processImpl(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint8_t threadNum, ParallelPipelineProfiler &profiler) override;

    private:
        Proc mProcess;
        std::mutex mMutex;
        std::condition_variable mCV; // triggered when the ImageProcess is released back to this Stage struct
        bool mProcessAvailable; // is the process currently availabel for use?
        uint32_t mFrame; // index of next frame
    };

    struct Frame
    {
        uint8_t *inputPtr;
        uint8_t *outputPtr = nullptr;
        uint32_t frameNum;
        uint32_t dauDataType;
    };

    // A queue of length 1 between M producers and N consumers
    //   Because the exact enqueue and dequeue operations vary (some threads need to check mShutdown,
    //   some threads need to change mFramesInProgress), this is just a plain struct and threads
    //   can use the members directly.
    template <typename T>
    struct Handoff
    {
        T value;
        bool hasValue;
        std::condition_variable added, removed;
    };

private: // Internal methods
    
    // Acquire a unique lock on mMutex
    std::unique_lock<std::mutex> lock();

    // Flush the pipeline and return the lock on mMutex
    std::unique_lock<std::mutex> flushLock();

    // Process an image on this thread, using two temporary buffers for the image data
    ThorStatus processImage(Frame &frame, uint8_t threadNum, uint8_t *tmpA, uint8_t *tmpB);

    // Worker function for processing threads
    void processThread(uint8_t threadNum);

    // worker function for notify thread
    void notifyThread();

private: // Instance data

    uint8_t mNumProcessThreads;
    std::vector<std::thread> mThreads;

    CineBuffer *mCineBuffer;

    // Pipeline of stages
    std::vector<std::unique_ptr<IStage>> mStages;

    // Single mutex protects all instance variables
    std::mutex mMutex;
    bool mInitialized; // true if init has been called
    bool mShutdown;    // true if terminate has been called
    bool mFlush;       // true if flush has been called

    // Handoff from dispatcher -> processor threads
    Handoff<Frame> mFrameToProcess;

    // Handoff from processor threads -> notifier thread
    Handoff<Frame> mFrameToNotify;

    // Total number of frames in the pipeline
    unsigned int mFramesInProgress;
    std::condition_variable mFrameCompleted;

    ParallelPipelineProfiler mProfiler;

    // linked process
    std::vector<std::pair<ParallelImageProcessor*, Frame>> mLinkedProcs;
};

// Template function definitions

template <typename Proc, typename ...Args>
ParallelImageProcessor::StageHandle<Proc> ParallelImageProcessor::addParallelStage(Args ...args)
{
    // Must not be initialized yet
    assert(!mInitialized);

    ParallelStage<Proc> *stage = new ParallelStage<Proc>(mNumProcessThreads, std::forward<Args>(args)...);
    mStages.emplace_back(stage);
    mProfiler.addSection("process p stage");
    return stage->handle();
}

template <typename Proc, typename ...Args>
ParallelImageProcessor::StageHandle<Proc> ParallelImageProcessor::addSequentialStage(Args ...args)
{
    // Must not be initialized yet
    assert(!mInitialized);

    SequentialStage<Proc> *stage = new SequentialStage<Proc>(std::forward<Args>(args)...);
    mStages.emplace_back(stage);

    mProfiler.addSection("await s stage");
    mProfiler.addSection("process s stage");
    return stage->handle();
}

template<typename Proc>
ParallelImageProcessor::StageHandle<Proc> ParallelImageProcessor::getStage()
{
    const char *name = Proc::name();
    for (auto &istage : mStages)
    {
        if (0 == strcmp(istage->name(), name))
        {
            return istage->handle<Proc>();
        }
    }
    ALOGE("Error, no stage matches name %s", name);
    return StageHandle<Proc>();
}

template <typename Proc>
ParallelImageProcessor::StageHandle<Proc> ParallelImageProcessor::IStage::handle()
{
    if (mIsParallel)
        return static_cast<ParallelStage<Proc>*>(this)->handle();
    else
        return static_cast<SequentialStage<Proc>*>(this)->handle();
}

template <typename Proc>
template <typename ...Args>
ParallelImageProcessor::ParallelStage<Proc>::ParallelStage(uint8_t numThreads, Args&&... args) :
    IStage(true)
{
    // Seems some ImageProcess types don't destruct properly if init is never called...
    mProcesses.reserve(numThreads);
    for (uint8_t tid=0; tid != numThreads; ++tid)
    {
        mProcesses.emplace_back(std::forward<Args>(args)...);
    }
}

template <typename Proc>
template <typename ...Args>
ParallelImageProcessor::SequentialStage<Proc>::SequentialStage(Args&&... args) :
    IStage(false),
    mProcess(std::forward<Args>(args)...),
    mProcessAvailable(true),
    mFrame(0u-1u) // UINT32_T max
{
    LOGD("Created sequential stage, mFrame = %u, mFrame+1 = %u", mFrame, mFrame+1);
}

template <typename Proc>
ThorStatus ParallelImageProcessor::ParallelStage<Proc>::setProcessParams(ImageProcess::ProcessParams &params)
{
    ThorStatus status = THOR_OK;
    for (auto &proc : mProcesses)
    {
        status = proc.setProcessParams(params);
        if (IS_THOR_ERROR(status))
            return status;
    }
    return status;
}

template <typename Proc>
ThorStatus ParallelImageProcessor::ParallelStage<Proc>::init()
{
    ThorStatus status = THOR_OK;
    for (auto &proc : mProcesses)
    {
        status = proc.init();
        if (IS_THOR_ERROR(status))
            return status;
    }
    return status;
}

template <typename Proc>
void ParallelImageProcessor::ParallelStage<Proc>::terminate()
{
    for (auto &proc : mProcesses)
    {
        proc.terminate();
    }
}

template <typename Proc>
ParallelImageProcessor::StageHandle<Proc>
ParallelImageProcessor::ParallelStage<Proc>::handle()
{
    StageHandle<Proc> h;
    for (auto &proc : mProcesses)
        h.mProcesses.push_back(&proc);
    h.mStage = this;
    return h;
}

template <typename Proc>
ThorStatus ParallelImageProcessor::ParallelStage<Proc>::processImpl(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint8_t threadNum, ParallelPipelineProfiler &profiler)
{
    profiler.startNextSection(frameNum); // process p stage
    return mProcesses[threadNum].process(inputPtr, outputPtr);
}

template <typename Proc>
ThorStatus ParallelImageProcessor::SequentialStage<Proc>::setProcessParams(ImageProcess::ProcessParams &params)
{
    return mProcess.setProcessParams(params);
}

template <typename Proc>
ThorStatus ParallelImageProcessor::SequentialStage<Proc>::init()
{
    return mProcess.init();
}

template <typename Proc>
void ParallelImageProcessor::SequentialStage<Proc>::terminate()
{
    return mProcess.terminate();
}

template <typename Proc>
ParallelImageProcessor::StageHandle<Proc>
ParallelImageProcessor::SequentialStage<Proc>::handle()
{
    StageHandle<Proc> h;
    h.mProcesses.push_back(&mProcess);
    h.mStage = this;
    return h;
}

template <typename Proc>
ThorStatus ParallelImageProcessor::SequentialStage<Proc>::processImpl(uint8_t *inputPtr, uint8_t *outputPtr, uint32_t frameNum, uint8_t threadNum, ParallelPipelineProfiler &profiler)
{
    UNUSED(threadNum);
    // Acquire the process instance
    {
        profiler.startNextSection(frameNum); // await s stage
        std::unique_lock<std::mutex> l(mMutex);
        // Wait until the process is available AND the frame is exactly one less than frameNum
        while (!(mProcessAvailable && mFrame+1 == frameNum))
        {
            if (frameNum <= mFrame && mFrame != (0u-1u))
            {
                LOGE("SequentialStage error: This stage is trying to process frame %u but frame %u is already completed", frameNum, mFrame);
                std::terminate();
            }
            mCV.wait(l);
        }
        mProcessAvailable = false;
    }

    profiler.startNextSection(frameNum); // process s stage
    ThorStatus status = mProcess.process(inputPtr, outputPtr);

    // Return the process instance
    {
        std::unique_lock<std::mutex> l(mMutex);
        mProcessAvailable = true;
        mFrame = frameNum;
        l.unlock();
        mCV.notify_all(); // Have to notify all, as multiple frames may be waiting here.
    }
    return status;
}

template <typename Proc>
void ParallelImageProcessor::SequentialStage<Proc>::setFramesCompleted(uint32_t frameNum)
{
    // Shouldn't technically need to lock, as the caller must hold the mMutex
    //   of the ParallelImageProcessor, and the pipeline must be flushed.
    // Still, best practice to do so anyway, and this method is called infrequently
    std::unique_lock<std::mutex> l(mMutex);
    mFrame = frameNum;
}


namespace detail
{
    // poor mans std::invoke replacement, does not work with std::reference_wrapper, t must be a reference (non-const), and does not work with member data pointers
    template <typename T1, typename T2, typename Type, typename ...Args>
    auto cpp11_invoke(Type T1::* fn, T2 &&t, Args &&...args) -> decltype((std::forward<T2>(t).*fn)(std::forward<Args>(args)...))
    {
        return (std::forward<T2>(t).*fn)(std::forward<Args>(args)...);
    }

    template <typename Fn, typename ...Args>
    auto cpp11_invoke(Fn &&fn, Args &&...args) -> decltype(std::forward<Fn>(fn)(std::forward<Args>(args)...))
    {
        return std::forward<Fn>(fn)(std::forward<Args>(args)...);
    }
}

template <typename T>
template <typename Fn, typename ...Args>
void ParallelImageProcessor::StageHandle<T>::each(Fn &&fn, Args &&...args)
{
    for (auto *proc : mProcesses)
    {
        ::detail::cpp11_invoke(std::forward<Fn>(fn), *proc, std::forward<Args>(args)...);
    }
}

template <typename T>
void ParallelImageProcessor::StageHandle<T>::setEnabled(bool enabled)
{
    mStage->setEnabled(enabled);
}