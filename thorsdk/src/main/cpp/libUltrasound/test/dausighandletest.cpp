#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <poll.h>
#include <DauSignalHandler.h>
#include <DauContext.h>

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    DauContext                          dauContext;
    DauSignalHandler                    dauSigHandler(dauContext);
    DauSignalHandler::SignalCallback    callback(nullptr);
    bool                                keepGoing = true;
    int                                 choice;
    int                                 timerFd = -1;
    struct itimerspec                   timerSpec;
    struct timespec                     now;

    timerFd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (-1 == timerFd)
    {
        printf("Unable to create timer\n");
        goto err_ret;
    }

    if (-1 == clock_gettime(CLOCK_REALTIME, &now))
    {
        printf("Unable to get time\n");
        goto err_ret;
    }

    dauContext.dataPollFd.initialize(PollFdType::Timer, timerFd);

    while (keepGoing)
    {
        printf("\t(i)nit\n");
        printf("\t(s)tart\n");
        printf("\tsto(p)\n");
        printf("\t(t)erminate\n");
        printf("\t(e)xit\n");
        printf("Select an option: ");

        choice = getchar();
        getchar(); // For newline char
        printf("\n");

        switch (choice)
        {
            case 'i':
                if (!dauSigHandler.init(&callback))
                {
                    printf("*** Unable to initialize ***\n\n");
                    goto err_ret;
                }

                break;

            case 's':
                if (!dauSigHandler.start())
                {
                    printf("*** Unable to start ***\n\n");
                    goto err_ret;
                }

                timerSpec.it_value.tv_sec = now.tv_sec;
                timerSpec.it_value.tv_nsec = 0;
                timerSpec.it_interval.tv_sec = 1;
                timerSpec.it_interval.tv_nsec = 0;
                
                if (-1 == timerfd_settime(timerFd, TFD_TIMER_ABSTIME, &timerSpec, NULL))
                {
                    printf("Unable to set timer\n");
                    goto err_ret;
                }
                break;

            case 'p':
                timerSpec.it_value.tv_sec = 0;
                timerSpec.it_value.tv_nsec = 0;
                timerSpec.it_interval.tv_sec = 0;
                timerSpec.it_interval.tv_nsec = 0;
                
                if (-1 == timerfd_settime(timerFd, TFD_TIMER_ABSTIME, &timerSpec, NULL))
                {
                    printf("Unable to clear timer\n");
                    goto err_ret;
                }

                dauSigHandler.stop();
                break;

            case 't':
                dauSigHandler.terminate();
                break;

            case 'e':
                keepGoing = false;
                break;

            default:
                printf("Invalid option: 0x%X '%c'\n\n", choice, choice);
                break;
        }
    }

err_ret:
    if (-1 != timerFd)
    {
        close(timerFd);
    }

    return(0); 
}
