//
// Copyright 2022 EchoNous Inc.
//

#pragma once
#include <cassert>
#include <mutex>
#include <condition_variable>

// Sync<T> is a wrapper around some type T which prevents accessing the wrapped instance without holding
// an exclusive lock on a mutex. Example use:
//
// void DoTheThing(Sync<int> &sync) {
//     auto ptr = sync.lock();
//     // In here, no other thread may access sync
//     // Safe to read and write *ptr
//     if (*ptr == 0) {
//         *ptr = 10;
//     } else {
//         *ptr += 5;
//     }
//     // Sync is unlocked when ptr goes out of scope here
// }


/// Smart pointer type to an instance of type T. This class ensures that an exclusive lock is owned
/// while manipulating the instance.
///
/// Class invariant: either mPtr is nullptr, or mLock owns the lock on a mutex protecting mPtr.
template <class T>
class LockedPtr {
public:
    typedef T* pointer;
    typedef T  element_type;

    /// Creates a null LockedPtr (does not hold any lock)
    explicit LockedPtr(std::nullptr_t) : mLock(), mPtr(nullptr) {}
    /// Creates a LockedPtr holding a lock to the given mutex
    LockedPtr(std::mutex &mutex, T* ptr) :
        mLock(mutex),
        mPtr(ptr)
    {
        // This constructor must be called with a valid ptr
        //assert(mPtr);
    }
    /// Move constructor
    LockedPtr(LockedPtr &&rhs) :
        mLock(std::move(rhs.mLock)),
        mPtr(std::exchange(rhs.mPtr, nullptr))
    {}

    /// Move assign operator
    LockedPtr& operator=(LockedPtr &&rhs)
    {
        using std::swap;
        swap(mLock, rhs.mLock);
        swap(mPtr, rhs.mPtr);
        return *this;
    }

    /// Check that we own the lock on the mutex.
    /// This should always be true, unless this LockedPtr has no value (is a nullptr)
    bool owns_lock() const { return mLock.owns_lock(); }

    /// Check that this LockedPtr points to a value
    operator bool() const { return mPtr != nullptr; }

    /// Access pointed to value
    T* operator->() const { /*assert(mPtr); assert(owns_lock());*/ return mPtr; }
    /// Access pointed to value
    T& operator*() const { /*assert(mPtr); assert(owns_lock());*/ return *mPtr; }

    void lock() {
        mLock.lock();
    }
    void unlock() {
        mLock.unlock();
    }

    std::unique_lock<std::mutex>& get_lock() {
        return mLock;
    }

    /// Wait on a condition_variable until predicate(ptr) returns true
    /// Signature of predicate should be `bool predicate(T*)`
    template <class Predicate>
    void await(std::condition_variable &cv, Predicate predicate) {
        while (!predicate(mPtr)) {
            cv.wait(mLock);
        }
    }

private:
    std::unique_lock<std::mutex> mLock;
    T* mPtr;
};

template <class T>
class Sync {
public:
    /// Construct a Sync instance, passing args to the constructor of T
    template <class... Args>
    explicit Sync(Args &&...args) : mMutex(), mValue(std::forward<Args>(args)...) {}

    // Note: Copy contructor and move constructor are automatically deleted, since std::mutex type
    // cannot be copied or moved.

    /// Lock this instance, returning a smart pointer that can access the contained value
    LockedPtr<T> lock()
    {
        return LockedPtr<T>(mMutex, &mValue);
    }

    // Could add try_lock, lock with timeout, etc if needed

private:
    std::mutex mMutex;
    T mValue;
};