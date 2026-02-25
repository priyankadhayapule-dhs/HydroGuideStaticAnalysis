//
// Copyright 2016 EchoNous, Inc.
//
//
// This class is for signaling between two threads of a single process.
// Both threads will utilize a shared Event instance.
//
#pragma once

enum class EventMode : bool
{
    AutoReset,      // The Event will reset automatically after a wait() completion
    ManualReset,    // The Event will remain signaled after a wait() completion
};

class Event
{
public: // Methods
    Event();
    ~Event();

    //-------------------------------------------------------------------------
    // Will initialize the event and set initial state.  Calling multiple times
    // will have no affect and will return true if already initialized.
    // 
    // eventMode:       If AutoReset then the state will be set to nonsignaled
    //                  at successful conclusion of wait().
    //
    // isSet:           If true then the initial state will be signaled.
    //-------------------------------------------------------------------------
    bool init(const EventMode& eventMode, bool isSet);

    //-------------------------------------------------------------------------
    // Set the event to the signaled state.  Setting an already set event has
    // no effect.
    //-------------------------------------------------------------------------
    void set();

    //-------------------------------------------------------------------------
    // Reset the event to the nonsignaled state.
    //-------------------------------------------------------------------------
    void reset();

    //-------------------------------------------------------------------------
    // Wait for event to be set.  Default wait time is infinite.
    //
    // Returns true if the event was signaled before the timeout expires.  Will
    // reset the event if it was initialized with AutoReset.
    //
    // Returns false if the timeout expired.
    //-------------------------------------------------------------------------
    bool wait(int msecTimeout = -1);

    //-------------------------------------------------------------------------
    // Returned fd can be used in poll(2) or select(2) to wait on multiple objects.
    //
    // Note: AutoReset flag has no affect if this fd is used outside of this class.
    //-------------------------------------------------------------------------
    int getFd() { return mFd; }

private: // Properties
    int         mFd;
    EventMode   mEventMode;
}; 
