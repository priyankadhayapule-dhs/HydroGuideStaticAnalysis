#include <cassert>
#include <vector>

template <class T>
class CircularBuffer {
public:
    CircularBuffer(size_t size) : mBuffer(size), mIndex(0) {}
    size_t size() const { return mBuffer.size(); }
    void resize(size_t size) { mBuffer.resize(size); mIndex = 0; }

    size_t dataIndex(size_t index) const {
        //assert(index < size());
        size_t i = mIndex+index;
        if (i < size())
            return i;
        return i-size();
    }

    const T& operator[](size_t index) const { return mBuffer[dataIndex(index)]; }
    T& operator[](size_t index) { return mBuffer[dataIndex(index)]; }

    void push(const T& value) {
        mBuffer[mIndex++] = value;
        if (mIndex == size())
            mIndex = 0;
    }
    void push(T&& value) {
        mBuffer[mIndex++] = value;
        if (mIndex == size())
            mIndex = 0;
    }

private:
    std::vector<T> mBuffer;
    size_t mIndex;
};