//
// Copyright 2022 EchoNous Inc.
//
#define LOG_TAG "RealtimeModule"
#include <RealtimeModule.h>
#include <ThorUtils.h>

RealtimeModule::RealtimeModule() :
    mWorkerControl(),
    mWorkerThread(),
    mWorkerNotify()
{
}

RealtimeModule::~RealtimeModule() {
    // Tell worker thread to exit
    onTerminate();
    // Wait for worker to finish
    if (mWorkerThread.joinable()) {
        mWorkerThread.join();
    }
}

ThorStatus RealtimeModule::onInit() {
    // If previous worker is alive and not joined, join with it
    LOGD("RealtimeModule::onInit");
    if (mWorkerThread.joinable()) {
        LOGD("Joining worker thread");
        mWorkerThread.join();
    } else {
        LOGD("Worker thread not joinable");
    }

    // Set initial state
    LockedPtr<WorkerControl> ctrl = mWorkerControl.lock();
    ctrl->exit = false;
    ctrl->cineFrameNum = 0xFFFFFFFFu;
    ctrl->dauDataTypes = 0;
    ctrl.unlock();

    // Start worker
    LOGD("Starting worker thread");
    mWorkerThread = std::thread(&RealtimeModule::workerFunc, this);

    return THOR_OK;
}

void RealtimeModule::onTerminate() {
    LOGD("RealtimeModule::onTerminate");

    // Tell worker to exit
    LockedPtr<WorkerControl> ctrl = mWorkerControl.lock();
    ctrl->exit = true;
    ctrl.unlock();
    // Notify worker that control state has changed
    mWorkerNotify.notify_one();

    // wait for worker to end
    if (mWorkerThread.joinable()) {
        LOGD("Joining worker thread");
        mWorkerThread.join();
        mWorkerThread = std::thread();
        LOGD("worker thread successfully joined");

    } else {
        LOGD("Worker thread not joinable");
    }
}

void RealtimeModule::onData(uint32_t frameNum, uint32_t dauDataTypes) {
    // Update cineFrameNum in control
    LockedPtr<WorkerControl> ctrl = mWorkerControl.lock();
    ctrl->cineFrameNum = frameNum;
    ctrl->dauDataTypes = dauDataTypes;
    ctrl.unlock();
    // Notify worker that control state has changed
    mWorkerNotify.notify_one();
}

void RealtimeModule::onSave(uint32_t startFrame, uint32_t endFrame, echonous::capnp::ThorClip::Builder &builder)
{
    LOGD("[ LABELS ] RealtimeModule::onSave(startFrame(%u), endFrame(%u)", startFrame, endFrame);
}

void RealtimeModule::onLoad(echonous::capnp::ThorClip::Reader &reader)
{
    LOGD("[ LABELS ] RealtimeModule::onLoad");
}

uint32_t RealtimeModule::getProcessedFrame() const {
    return mProcessedFrame;
}

void RealtimeModule::setWorkerEnabled(bool enabled) {
    // Set worker enabled status
    LockedPtr<WorkerControl> ctrl = mWorkerControl.lock();
    ctrl->enabled = enabled;
    ctrl.unlock();
    // Notify worker that control state has changed
    mWorkerNotify.notify_one();
}

ThorStatus RealtimeModule::workerInit() {
    return THOR_OK;
}

ThorStatus RealtimeModule::workerTerminate() {
    return THOR_OK;
}
void RealtimeModule::workerFramesSkipped(uint32_t first, uint32_t last) {
    ((void)first);
    ((void)last);
    // No-op
}

bool RealtimeModule::workerShouldAwaken(const WorkerControl& ctrl) {
    uint32_t processedFrame = mProcessedFrame;
    // Always should awaken if told to exit
    if (ctrl.exit)
        return true;
    // Otherwise, awaken if new frame and we are enabled
    
    return ctrl.enabled && (ctrl.cineFrameNum > processedFrame || processedFrame == 0xFFFFFFFFu);
}

void RealtimeModule::workerFunc() {
    ThorStatus status;
    mProcessedFrame = 0xFFFFFFFFu;

    status = workerInit();
    if (IS_THOR_ERROR(status)) {
        LOGE("workerInit failed with error code %08x", status);
        workerTerminate();
        return;
    }

    while(true) {
        // Lock control state
        LockedPtr<WorkerControl> ctrl = mWorkerControl.lock();

        // Wait until something to do
        while(!workerShouldAwaken(*ctrl))
            mWorkerNotify.wait(ctrl.get_lock());

        // Read control state
        WorkerControl copyControl = *ctrl;
        ctrl.unlock();

        // Exit if we are told to
        if (copyControl.exit) {
            LOGD("Worker told to exit, stopping...");
            break;
        }

        // Check for skipped frames
        uint32_t prevFrame = mProcessedFrame;
        if (prevFrame != 0xFFFFFFFFu && prevFrame+1 != copyControl.cineFrameNum) {
            //LOGD("workerFramesSkipped(%u, %u)", prevFrame+1, copyControl.cineFrameNum-1);
            workerFramesSkipped(prevFrame+1, copyControl.cineFrameNum-1);
        }

        // Process the new frame (without holding a lock on mWorkerControl)
        //LOGD("workerProcess(%u, 0x%08x)", copyControl.cineFrameNum, copyControl.dauDataTypes);
        status = workerProcess(copyControl.cineFrameNum, copyControl.dauDataTypes);
        if (IS_THOR_ERROR(status)) {
            LOGE("workerProcess failed with error code %08x", status);
            break;
        }
        // Update processedFrame
        mProcessedFrame = copyControl.cineFrameNum;
    }

    status = workerTerminate();
    if (IS_THOR_ERROR(status)) {
        LOGE("workerTerminate failed with error code %08x", status);
        return;
    }
}
