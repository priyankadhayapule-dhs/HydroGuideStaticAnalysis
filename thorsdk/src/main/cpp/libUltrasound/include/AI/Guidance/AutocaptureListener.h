//
// Copyright 2022 EchoNous Inc.
//
#pragma once

class AutocaptureListener {
public:
    virtual ~AutocaptureListener() = default;
    virtual void onAutocaptureStart() = 0;
    virtual void onAutocaptureProgress(float progress) = 0;
    virtual void onAutocaptureSuccess() = 0;
    virtual void onAutocaptureFail() = 0;
};