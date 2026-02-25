#pragma once
#include <ThorError.h>
#include <ThorCapnpTypes.capnp.h>

class ThorClip {
    echonous::capnp::ThorClip::Reader mReader;
public:
    static ThorStatus openFile(const char *path, /* non-null */ ThorClip *clip);

    ThorClip(echonous::capnp::ThorClip::Reader reader);
};