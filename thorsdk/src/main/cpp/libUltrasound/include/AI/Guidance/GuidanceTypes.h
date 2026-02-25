//
// Copyright 2022 EchoNous Inc.
//
#pragma once
#include <string>
#include <View.h>
#include <cassert>

class Subview {
public:
    static constexpr size_t MAX_SUBVIEW_NAME_LENGTH = 16;
    static constexpr Subview Invalid() { return Subview(nullptr); }
    constexpr Subview(int index, View targetView) : mIndex(index), mTargetView(targetView) {
        assert(0 <= index);
    }
    constexpr bool isValid() const { return mIndex >= 0; }
    constexpr operator bool() const { return mIndex >= 0; }
    constexpr int index() const { assert(isValid()); return mIndex; }
    constexpr View targetView() const { assert(isValid()); return mTargetView; }
    constexpr bool operator==(const Subview &rhs) const { return isValid() && mIndex == rhs.mIndex && mTargetView == rhs.mTargetView; }
    constexpr bool operator!=(const Subview &rhs) const { return !(*this == rhs);}
    char* formatString(char buffer[MAX_SUBVIEW_NAME_LENGTH]) const;
private:
    constexpr Subview(std::nullptr_t) : mIndex(-1), mTargetView(View::A4C) {}

    int mIndex;
    View mTargetView;
};

class Quality {
public:
    constexpr Quality(int score) : mScore(score) {
        assert(1 <= score && score <= 5);
    }
    constexpr int score() const { return mScore; }
private:
    int mScore;
};

//struct SubviewId {
//    unsigned int id;
//    inline bool operator==(SubviewId rhs) const { return id == rhs.id; }
//    inline bool operator!=(SubviewId rhs) const { return id != rhs.id; }
//};
struct PhraseId {
    unsigned int id;
    inline static PhraseId none() { return PhraseId{0xFFFFFFFFu}; }
    inline bool operator==(PhraseId rhs) const { return id == rhs.id; }
    inline bool operator!=(PhraseId rhs) const { return id != rhs.id; }
};
struct AnimationId {
    unsigned int id;
    inline static AnimationId none() { return AnimationId{0xFFFFFFFFu}; }
    inline bool operator==(AnimationId rhs) const { return id == rhs.id; }
    inline bool operator!=(AnimationId rhs) const { return id != rhs.id; }
};
