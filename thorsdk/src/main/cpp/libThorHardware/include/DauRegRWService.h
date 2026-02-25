//
// Copyright 2017 EchoNous, Inc.
//

#pragma once

#include <pthread.h>
#include <memory>
#include <libusb/libusb.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <DauMemory.h>

#define PORT 8080
#define POLL_SIZE 200

struct libusb_device_handle;
struct libusb_transfer;

typedef enum
{
    READ = 0,
    WRITE
} DauRegRWFlags;

typedef struct
{
    uint32_t regAddr;
    uint32_t numWords;
    uint32_t flag;
} DauRegRW;

class DauRegRWService
{
public: // Methods
    DauRegRWService(const std::shared_ptr<DauMemory>& daum);
    ~DauRegRWService();

    bool init();
    bool start();
    void stop();
    void terminate();
    void handleMessage(unsigned char *recvBuffer);

private:
    // Methods
    static void*    workerThreadEntry(void* thisPtr);
    void            threadLoop();

private:
    // Properties
    Event                           mAckEvent;
    Event                           mStartEvent;
    Event                           mExitEvent;
    bool                            mIsInitialized;
    pthread_t                       mThreadId;
    struct sockaddr_in              mAddress;
    int                             mServerFd;
    int                             mClientFd;
    int                             mActiveConnections;
    DauRegRW                        mDauRegRW;
    bool                            mService;
    std::shared_ptr<DauMemory>      mDaumSPtr;
};