//
// Copyright 2022 EchoNous Inc.
//

#pragma once
#include <CineBuffer.h>
#include <Sync.h>
#include <thread>

class RealtimeModule : public CineBuffer::CineCallback {
public:
    RealtimeModule();
    virtual ~RealtimeModule();

    virtual ThorStatus  onInit() override;
    virtual void        onTerminate() override;
    virtual void        onData(uint32_t frameNum, uint32_t dauDataTypes) override;
    virtual void        onSave(uint32_t startFrame, uint32_t endFrame, echonous::capnp::ThorClip::Builder &builder) override;
    virtual void        onLoad(echonous::capnp::ThorClip::Reader &reader) override;
    /// Get the latest processed frame
    /// May be outdated after this function returns.
    uint32_t getProcessedFrame() const;
protected:
    void setWorkerEnabled(bool enabled);

    virtual ThorStatus workerInit();
    virtual ThorStatus workerTerminate();
    virtual void workerFramesSkipped(uint32_t first, uint32_t last);
    virtual ThorStatus workerProcess(uint32_t frameNum, uint32_t dauDataTypes) = 0;

private:
    struct WorkerControl {
        bool enabled = false;
        bool exit = false;
        uint32_t cineFrameNum = 0xFFFFFFFFu;
        uint32_t dauDataTypes = 0;
    };

    bool workerShouldAwaken(const WorkerControl& ctrl);
    void workerFunc(/*this*/);

    Sync<WorkerControl> mWorkerControl;
    std::atomic<uint32_t> mProcessedFrame;
    std::thread mWorkerThread;
    std::condition_variable mWorkerNotify;
};
