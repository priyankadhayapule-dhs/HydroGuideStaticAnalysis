//
//  ThorRingBuffer.hpp
//  EchoNous
//
//  Created by Michael May on 1/3/24.
//
#include <array>
#include <numeric>
#include <functional>
#ifndef ThorRingBuffer_hpp
#define ThorRingBuffer_hpp
template <typename T, size_t S>
class ThorRingBuffer
{
public:
    ThorRingBuffer() {mIndex = 0; mFilled = false;}
    const size_t size() { return mBuffer.size();}
    void insert(T input) {
        mBuffer[mIndex++] = input;
        if(mIndex >= mBuffer.size()) {
            mIndex = 0;
            mFilled = true;
        }
    }
    const T& elementAt(int index)
    {
        return mBuffer[index];
    }
    const bool isFull()
    {
        return mFilled;
    }
    const int currentIndex() {return mIndex;}
    void clear()
    {
        mIndex = 0;
        mFilled = false;
    }

private:
    std::array<T, S> mBuffer;
    int mIndex;
    bool mFilled;
};
#endif /* ThorRingBuffer_hpp */
