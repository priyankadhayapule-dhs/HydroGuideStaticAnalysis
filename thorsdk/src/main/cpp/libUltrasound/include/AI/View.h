//
// Copyright 2022 EchoNous Inc.
//
#pragma once
#include <ThorUtils.h>

enum class View {
    A4C,
    A2C,
    A5C,
    PLAX,
    UNKNOWN
};

constexpr inline const char* ViewToString(View view) {
    switch(view) {
        case View::A4C:  return "A4C";
        case View::A2C:  return "A2C";
        case View::A5C:  return "A5C";
        case View::PLAX: return "PLAX";
        case View::UNKNOWN: return "UNKNOWN";
        default:
            LOGD("Bad view value does not match any know views: %d", static_cast<int>(view));
            return "UNKNOWN";
    }
}
inline View ViewFromString(const char *str) {
    if (0 == strcmp(str, "A4C")) return View::A4C;
    if (0 == strcmp(str, "A2C")) return View::A2C;
    if (0 == strcmp(str, "A5C")) return View::A5C;
    if (0 == strcmp(str, "PLAX")) return View::PLAX;

    LOGE("No view found named %s", str);
    return View::UNKNOWN;
}