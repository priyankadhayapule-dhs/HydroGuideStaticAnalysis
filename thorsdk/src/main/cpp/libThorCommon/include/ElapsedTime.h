//
// Copyright 2017 EchoNous, Inc.
//
#pragma once

// A set of macros for measuring execution time of a function or block of code.
// The elapsed time in milliseconds will be sent to logcat.
//
// Note: It is *not* intended that the class ElapsedTime be used directly.
//
// Define ENABLE_ELAPSED_TIME before including this header file to enable this
// code.
//
// There are 3 ways to use this with the following macros.
//
// 1)
// ELAPSED_FUNC()       Place at beginning of function to measure function
//                      execution.  Will output name of function.
//
// 2)
// ELAPSED_SCOPE(msg)   Place at beginning of block scope.  Will print msg.
//
//
// 3)
// ELAPSED_BEGIN(msg)   Place before code to be executed.  Will print msg. A
//                      block scope is created between this and ELAPSED_END.
//
// ELAPSED_END()        Place after code to be executed.
//
//
//
// Example usage:
//
//  void doSomething()
//  {
//      ELAPSED_FUNC();       // Will measure time to execute this function
//
//      int     x = 50;
//
//      doSomething(x);
//
//      ELAPSED_BEGIN("Some Operation"); // Start timing from here to ELAPSED_END
//
//      for (int Y = 0; y < 4; y++)
//      {
//          ELAPSED_SCOPE("Loop iteration");  // Will measure time for each iteration
//          doSomethingElse(x, y);
//      }
//
//      finishUp();
//
//      ELAPSED_END(); // Will report the timing from ELAPSED_BEGIN above
//  }
//
//  Sample output from above example:
//
//  Loop iteration Elapsed Time: 4ms
//  Loop iteration Elapsed Time: 5ms
//  Loop iteration Elapsed Time: 4ms
//  Loop iteration Elapsed Time: 6ms
//  Some Operation Elapsed Time: 40ms
//  void doSomething() Elapsed Time: 60ms
//

// Define this to enable the measuring code and macros
#ifdef ENABLE_ELAPSED_TIME

#ifndef LOG_TAG
#define LOG_TAG "ElapsedTime"
#endif

#include <unistd.h>
#include <time.h>
#include <ThorUtils.h>

class ElapsedTime
{
public: // Methods
    ElapsedTime(const char* logTag, const char* message, const char* functionName) :
        mLogTag(logTag),
        mMessage(message),
        mFunctionName(functionName)
    {
        mStartTime = getTimeMsec();
    }

    ~ElapsedTime()
    {
        if (nullptr == mFunctionName)
        {
            __android_log_print(ANDROID_LOG_DEBUG,
                                mLogTag,
                                mMessage,
                                getTimeMsec() - mStartTime);
        }
        else
        {
            __android_log_print(ANDROID_LOG_DEBUG,
                                mLogTag,
                                mMessage,
                                mFunctionName,
                                getTimeMsec() - mStartTime);
        }
    }

    ElapsedTime() = delete;
    ElapsedTime(const ElapsedTime&) = delete;
    ElapsedTime& operator=(const ElapsedTime&) = delete;

private: // Methods
    double getTimeMsec()
    {
        struct timespec now;

#ifdef ELAPSED_TIME_USE_THREAD_TIME
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now);
#else
        clock_gettime(CLOCK_MONOTONIC, &now);
#endif
        return ((double) now.tv_sec * 1000.0L) + ((double) now.tv_nsec / 1000000.0L);
    }

private: // Properties
    const char*     mLogTag;
    const char*     mMessage;
    const char*     mFunctionName;
    double          mStartTime;
};


#define CAT_(a, b) a ## b
#define CAT(a, b) CAT_(a, b)

#define ELAPSED_FUNC() ::ElapsedTime CAT(__etime, __COUNTER__)(LOG_TAG, "%s Elapsed Time: %.3fms", __func__)
#define ELAPSED_SCOPE(str) ::ElapsedTime CAT(__etime, __COUNTER__)(LOG_TAG, str" Elapsed Time: %.3fms", nullptr)
#define ELAPSED_BEGIN(str) do { ::ElapsedTime CAT(__etime, __COUNTER__)(LOG_TAG, str" Elapsed Time: %.3fms", nullptr)
#define ELAPSED_END() } while(0)

#else

#define ELAPSED_FUNC()
#define ELAPSED_SCOPE(str)
#define ELAPSED_BEGIN(str)
#define ELAPSED_END()

#endif
