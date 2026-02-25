//
// Copyright 2016 EchoNous, Inc.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <ThorUtils.h>
#include <utils/Log.h>
#include <sys/param.h>
#include <PciDauMemory.h>
#include <DauRegisters.h>
#include <Event.h>
#include <ThorConstants.h>
#include <BitfieldMacros.h>

struct ThreadData
{
    uint32_t    iterations;
    Event*      eventPtr;
};


void* threadEntry(void* paramPtr)
{
    ThreadData*     threadData = (ThreadData*) paramPtr;
    PciDauMemory    dauMem;
    uint32_t        data;

    if (!dauMem.map())
    {
        goto err_exit;
    }

    for (; threadData->iterations > 0; threadData->iterations--)
    {
        usleep(16000);

        // Trigger an interrupt
        data = BIT(SYS_INT0_SE_FRAME_BIT + DAU_DATA_TYPE_B);
        dauMem.write(&data, SYS_SW_INT0_ADDR, 1);

        usleep(16000);

        // Clear source of interrupt
        data = 0;
        dauMem.write(&data, SYS_SW_INT0_ADDR, 1);
        dauMem.read(SYS_INT0_ADDR, &data, 1);
        dauMem.write(&data, SYS_INT0_ADDR, 1);
    }

    dauMem.unmap();

    usleep(32000);
    printf("Thread exiting...\n");
    threadData->eventPtr->set();
err_exit:
    return(NULL);
}


int main(int argc, char *argv[])
{
    int             ret = -1;
    struct pollfd   pollFd[3];
    int             frameFd = -1;
    const int       bufSize = 10;
    char            buf[bufSize];
    bool            keepLooping = true;
    pthread_t       threadId;
    uint32_t        iterations = 60;
    Event           exitEvent;
    ThreadData      threadData;

    frameFd = open("/sys/bus/pci/drivers/dau_pci/0000:01:00.0/frame_irq", O_RDONLY);
    if (-1 == frameFd)
    {
        printf("Unable to open frame_irq\n");
        goto err_ret;
    }

    exitEvent.init(EventMode::AutoReset, false);

    pollFd[0].fd = fileno(stdin);
    pollFd[0].events = POLLIN;
    pollFd[0].revents = 0;

    pollFd[1].fd = frameFd;
    pollFd[1].events = POLLPRI;
    pollFd[1].revents = 0;

    pollFd[2].fd = exitEvent.getFd();
    pollFd[2].events = POLLIN;
    pollFd[2].revents = 0;

    memset(buf, 0, bufSize);
    read(frameFd, &buf, bufSize);
    printf("%3s\n", buf);

    if (2 == argc)
    {
        iterations = strtoul(argv[1], 0, 0);
    }

    threadData.iterations = iterations;
    threadData.eventPtr = &exitEvent;

    ret = pthread_create(&threadId, NULL, threadEntry, &threadData);
    if (0 != ret)
    {
        printf("Failed to create thread\n");
        goto err_ret;
    }

    while (keepLooping)
    {
        ret = poll(pollFd, 3, 2000);
        if (-1 == ret)
        {
            printf("poll failed\n");
            keepLooping = false;
        }
        else if (POLLIN == (pollFd[0].revents & POLLIN))
        {
            keepLooping = false;
        }
        else if (POLLPRI == (pollFd[1].revents & POLLPRI))
        {
            lseek(frameFd, 0, SEEK_SET);
            read(frameFd, &buf, bufSize);
            printf("%3s\n", buf);

            iterations--;
            if (0 >= iterations)
            {
                printf("Received expected interrupts\n");
                keepLooping = false;
            }
        }
        else if (POLLIN == (pollFd[2].revents & POLLIN))
        {
            printf("Did not receive expected interrupts\n");
            keepLooping = false;
        }
    }

    pthread_join(threadId, NULL);
    ret = 0;

err_ret:
    if (-1 != frameFd)
    {
        ::close(frameFd);
    }
    return(ret);
}
