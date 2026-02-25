//
// Copyright 2017 EchoNous, Inc.
//
//

#define LOG_TAG "DauRegRWService"

#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <ThorUtils.h>
#include <ThorConstants.h>
#include <UsbDataHandler.h>
#include <libusb.h>
#include <ThorError.h>
#include <UsbDauMemory.h>
#include <DauFwUpgrade.h>
#include <UsbDauError.h>
#include <DauRegRWService.h>
#include <string>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>   //strlen
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include "include/DauRegRWService.h"

#define  MAX_BUF_SZ 16384
//-----------------------------------------------------------------------------
DauRegRWService::DauRegRWService(const std::shared_ptr<DauMemory>& daum) :
                                mServerFd(-1),
                                mClientFd(-1),
                                mActiveConnections(0),
                                mService(true),
                                mDaumSPtr(daum)
{
    ALOGD("DauRegRWService Constructor");
}

//-----------------------------------------------------------------------------
DauRegRWService::~DauRegRWService()
{
    ALOGD("DauRegRWService Destructor");
    terminate();
}

//-----------------------------------------------------------------------------
bool DauRegRWService::init()
{
    ALOGD("DauRegRWService : %s",__FUNCTION__ );
    int ret = 0, opt=1;
    bool    retVal = false;

    mAckEvent.init(EventMode::AutoReset, false);
    mStartEvent.init(EventMode::ManualReset, false);
    mExitEvent.init(EventMode::ManualReset, false);

    // Creating socket file descriptor
    mServerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mServerFd == 0)
    {
        ALOGE("socket creation is failed");
        goto err_ret;
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(mServerFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)) < 0)
    {
        ALOGE("setsockopt");
        goto err_ret;
    }

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    ret = ioctl(mServerFd, FIONBIO, (char *) &opt);
    if (ret < 0) {
        perror("ioctl() failed");
        close(mServerFd);
        exit(-1);
    }

    /*************************************************************/
    /* Bind the socket                                           */
    /*************************************************************/
    memset(&mAddress, 0, sizeof(mAddress));
    mAddress.sin_family = AF_INET;
    mAddress.sin_addr.s_addr = INADDR_ANY;
    mAddress.sin_port = htons( PORT );

    if (bind(mServerFd, (struct sockaddr *)&mAddress, sizeof(mAddress)) < 0)
    {
        ALOGE("bind failed");
        close(mServerFd);
        goto err_ret;
    }

    ALOGD("Listener on port %d \n", PORT);

    /*************************************************************/
    /* Set the listen back log                                   */
    /*************************************************************/
    ret = listen(mServerFd, 32);
    if (ret < 0) {
        perror("listen() failed");
        close(mServerFd);
        exit(-1);
    }

    ret = pthread_create(&mThreadId, NULL, workerThreadEntry, this);
    if (0 != ret) {
        ALOGE("Failed to create worker thread: ret = %d", ret);
        goto err_ret;
    }

    mAckEvent.wait(2000);
    ALOGD("create worker  thread: ret = %d mThreadID:%d", ret,mThreadId);
    mIsInitialized = true;
    retVal = true;
    start();
err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
bool DauRegRWService::start()
{
    bool retVal = false;
    int ret;

    ALOGD("DauRegRWService : %s",__FUNCTION__ );
    mService = true;
    mStartEvent.set();
    retVal = true;

    err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void DauRegRWService::terminate() {
    int ret;

    ALOGD("DauRegRWService : %s",__FUNCTION__ );
    if (!mIsInitialized) {
        return;
    }

    if(mServerFd)
    {
        ::close(mServerFd);
    }

    mService = false;
    mExitEvent.set();
    pthread_join(mThreadId, NULL);

    mIsInitialized = false;
}

//-----------------------------------------------------------------------------
void *DauRegRWService::workerThreadEntry(void *thisPtr)
{
    ((DauRegRWService *) thisPtr)->threadLoop();
    ALOGD("DAU Register Read/Write Service thread - leave");
    return (nullptr);
}

void DauRegRWService::handleMessage(unsigned char *recvBuffer) {

    uint32_t len = 0;
    int ret = -1;
    uint32_t* sendBuffer = nullptr;

    ALOGD("DauRegRW regAddr:0x%x length:0x%x flag:%d", mDauRegRW.regAddr,
          mDauRegRW.numWords, mDauRegRW.flag);

    switch (mDauRegRW.flag) {
        case READ: {

            sendBuffer = (uint32_t *) malloc(mDauRegRW.numWords * sizeof(uint32_t));
            memset(sendBuffer, '\0', sizeof(mDauRegRW.numWords * sizeof(uint32_t)));
            mDaumSPtr->read(mDauRegRW.regAddr, &sendBuffer[0], mDauRegRW.numWords);
            len = mDauRegRW.numWords * sizeof(uint32_t);

            //The data back to the client
            ret = send(mClientFd, sendBuffer, len, 0);
            if (ret < 0)
            {
                ALOGE("  send() failed");
                mActiveConnections =0;
                mClientFd =-1;
            }

            break;
           }
           case WRITE: {
               ALOGD("USB Service write transaction regAddr:0x%x numWords:0x%x val:0x%x",mDauRegRW.regAddr,mDauRegRW.numWords, *(recvBuffer + sizeof(DauRegRW)));

               mDaumSPtr->write((uint32_t *)(recvBuffer + sizeof(DauRegRW)), mDauRegRW.regAddr, mDauRegRW.numWords);
               ALOGD("USB Service write transaction finished");
               break;
           }

        default: {
            ALOGE("Invalid Operation");
        }
    }
}

//-----------------------------------------------------------------------------
void DauRegRWService::threadLoop()
{
    struct pollfd pollFd[POLL_SIZE];
    int ret = -1, len, new_sd = -1;
    bool keepLooping = true;
    unsigned char recvBuffer[MAX_BUF_SZ];

    const int exitEvent = 0;
    const int startEvent = 1;
    const int numFd = 2;

    ALOGD("DauRegRWService processing thread - enter");

    pollFd[exitEvent].fd = mExitEvent.getFd();
    pollFd[exitEvent].events = POLLIN;
    pollFd[exitEvent].revents = 0;

    pollFd[startEvent].fd = mStartEvent.getFd();
    pollFd[startEvent].events = POLLIN;
    pollFd[startEvent].revents = 0;

    mAckEvent.set();

    while (keepLooping) {
        ret = poll(pollFd, numFd, -1);
        if (-1 == ret) {
            ALOGE("threadLoop: Error occurred during poll(). errno = %d", errno);
            keepLooping = false;
        } else if (pollFd[exitEvent].revents & POLLIN) {
            keepLooping = false;
            mExitEvent.reset();
        } else if (pollFd[startEvent].revents & POLLIN) {
            mStartEvent.reset();
            while ( mService ) {
                int fd = accept(mServerFd, NULL, NULL);
                if (fd < 0) {
                    if (errno != EWOULDBLOCK) {
                        ALOGE("  accept() failed");
                    }
                } else {
                    ALOGD("New incoming connection - FD:%d\n", fd);
                    mClientFd = fd;
                    mActiveConnections++;
                }

                if(mClientFd > 0) {
                    ret = recv(mClientFd, recvBuffer, sizeof(recvBuffer), 0);
                    if (ret < 0) {
                        if (errno != EWOULDBLOCK) {
                            ALOGE("  recv() failed");
                            mActiveConnections = 0;
                            mClientFd = -1;
                        }
                    } else if (ret == 0) {
                        ALOGE("  Connection closed threadLoop\n");
                        mActiveConnections = 0;
                        mClientFd = -1;
                    } else {
                        // Data was received
                        len = ret;
                        ALOGD("Data Length received from client:%d", len);
                        // Received DauRegRW Structure from the client
                        memcpy(&mDauRegRW, &recvBuffer[0], sizeof(DauRegRW));
                        handleMessage(&recvBuffer[0]);
                    }
                }
            }
        }
    }
}
