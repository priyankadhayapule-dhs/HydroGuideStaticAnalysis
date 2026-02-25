//
// Copyright 2022 EchoNous Inc.
//
#include <Guidance/GuidanceTypes.h>
#include <cstdio>

char* Subview::formatString(char buffer[Subview::MAX_SUBVIEW_NAME_LENGTH]) const {
    if (!isValid()) {
        strncpy(buffer, "Invalid", MAX_SUBVIEW_NAME_LENGTH);
        return buffer;
    }
    //assert(0 <= mIndex);
    //assert(mIndex < 100);
    std::snprintf(buffer, Subview::MAX_SUBVIEW_NAME_LENGTH, "%s(%d)", ViewToString(mTargetView), mIndex);
    return buffer;
}