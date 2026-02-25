#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <Event.h>
#include <ThorUtils.h>
#include <CriticalSection.h>
#include <sys/param.h>

#include <gtest/gtest.h>

namespace android
{

struct ThreadData 
{
    Event*  eventPtr;
    int     msecTimeout;
    bool    threadExiting;
    bool    waitSucceeded;
};

CriticalSection dataLock;

//-------------------------------------------------------------------------
void* threadEntry(void *paramPtr)
{
    ThreadData* threadDataPtr = (ThreadData *)paramPtr;

    if (threadDataPtr)
    {
        bool succeeded;

        succeeded = threadDataPtr->eventPtr->wait(threadDataPtr->msecTimeout); 

        dataLock.enter();
        threadDataPtr->threadExiting = true;
        threadDataPtr->waitSucceeded = succeeded;
        dataLock.leave();
    }

    return(NULL);
}

//-------------------------------------------------------------------------
bool hasThreadExited(ThreadData& threadData)
{
    bool threadExitingCopy = false;

    dataLock.enter();
    threadExitingCopy = threadData.threadExiting;
    dataLock.leave();
    
    return(threadExitingCopy);
}

//-------------------------------------------------------------------------
// Test to validate basic functionality triggering an Event.
// Uses ManualReset type.
//-------------------------------------------------------------------------
TEST(ThorEventTest, TriggerTest)
{
    pthread_t   threadId;
    Event       event;
    ThreadData  threadData;

    // Initialization
    threadData.eventPtr = &event;
    threadData.msecTimeout = -1; // Infinite timeout
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(event.init(EventMode::ManualReset, false));
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    // Ensure thread is waiting for event
    sleep(1);

    // Thread should not have exiting since we haven't set the Event
    EXPECT_FALSE(hasThreadExited(threadData));

    // Set event and make sure thread exits
    event.set();
    pthread_join(threadId, NULL);
    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_TRUE(threadData.waitSucceeded);
    EXPECT_TRUE(-1 != event.getFd());
}

//-------------------------------------------------------------------------
// Similar to above but also tests that the Event remains set until reset.
// This tests both the set() and reset() methods.
//-------------------------------------------------------------------------
TEST(ThorEventTest, ManualResetTest)
{
    pthread_t   threadId;
    Event       event;
    ThreadData  threadData;

//
// Phase 1
//
    // Initialization
    threadData.eventPtr = &event;
    threadData.msecTimeout = -1; // Infinite timeout
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(event.init(EventMode::ManualReset, false));
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    // Ensure thread is waiting for event and doesn't exit prematurely
    sleep(1);
    EXPECT_FALSE(hasThreadExited(threadData));

    // Set event and make sure thread exits
    event.set();
    pthread_join(threadId, NULL);
    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_TRUE(threadData.waitSucceeded);

//
// Phase 2
//
    // Create another thread, but this time we expect it to immediately
    // exit because the Event has not been reset.
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    sleep(1);
    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_TRUE(threadData.waitSucceeded);
    pthread_join(threadId, NULL);

//
// Phase 3
//
    // Create another thread, but this time we expect it to behave same
    // as the original thread since we will manually reset the Event first.
    event.reset();
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    // Ensure thread is waiting for event and doesn't exit prematurely
    sleep(1);
    EXPECT_FALSE(hasThreadExited(threadData));

    // Set event and make sure thread exits
    event.set();
    pthread_join(threadId, NULL);
    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_TRUE(threadData.waitSucceeded);
}

//-------------------------------------------------------------------------
// Test that the wait timeout value works correctly
//-------------------------------------------------------------------------
TEST(ThorEventTest, WaitTimeoutTest)
{
    pthread_t   threadId;
    Event       event;
    ThreadData  threadData;

    // Initialization
    threadData.eventPtr = &event;
    threadData.msecTimeout = 1000;
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(event.init(EventMode::ManualReset, false));
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    // Just wait for thread to exit (via timeout) without signalling it
    pthread_join(threadId, NULL);

    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_FALSE(threadData.waitSucceeded);
}

//-------------------------------------------------------------------------
// Test the isSet flag
//-------------------------------------------------------------------------
TEST(ThorEventTest, IsSetFlagTest)
{
    pthread_t   threadId;
    Event       event;
    ThreadData  threadData;

    // Initialization
    threadData.eventPtr = &event;
    threadData.msecTimeout = -1;
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(event.init(EventMode::ManualReset, true));
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    // Just wait for thread to exit as it should already be signaled
    pthread_join(threadId, NULL);

    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_TRUE(threadData.waitSucceeded);
}

//-------------------------------------------------------------------------
// Test the AutoReset mode
//-------------------------------------------------------------------------
TEST(ThorEventTest, AutoResetTest)
{
    pthread_t   threadId;
    Event       event;
    ThreadData  threadData;

//
// Phase 1
//
    // Initialization
    threadData.eventPtr = &event;
    threadData.msecTimeout = -1;
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(event.init(EventMode::AutoReset, false));
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    // Ensure thread is waiting for event and doesn't exit prematurely
    sleep(1);
    EXPECT_FALSE(hasThreadExited(threadData));

    // Set event and make sure thread exits
    event.set();
    pthread_join(threadId, NULL);
    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_TRUE(threadData.waitSucceeded);

//
// Phase 2
//
    // Create another thread.  Expect same behavior as before without explicitly
    // resetting the event.
    threadData.threadExiting = false;
    threadData.waitSucceeded = false;
    ASSERT_TRUE(0 == pthread_create(&threadId, NULL, threadEntry, &threadData));

    // Ensure thread is waiting for event and doesn't exit prematurely
    sleep(1);
    EXPECT_FALSE(hasThreadExited(threadData));

    // Set event and make sure thread exits
    event.set();
    pthread_join(threadId, NULL);
    EXPECT_TRUE(hasThreadExited(threadData));
    EXPECT_TRUE(threadData.waitSucceeded);
}

}

