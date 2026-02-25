#define LOG_TAG "ThorClip"
#include <ThorClip.h>

ThorStatus ThorClip::openFile(const char *path, ThorClip *clip) {
    return TER_MISC_NOT_IMPLEMENTED;
}

ThorClip::ThorClip(echonous::capnp::ThorClip::Reader reader) : mReader(reader) {}