//
// Copyright 2022 EchoNous Inc.
//
#define LOG_TAG "AnimationMap"
#include <AI/Guidance/AnimationMap.h>
#include <ThorUtils.h>
#include <Media/Video.h>
AnimationMap::AnimationMap() = default;
AnimationMap::~AnimationMap() = default;

AnimationId AnimationMap::addAnimation(const char *name) {
    for (unsigned int i=0; i != mAnimations.size(); ++i) {
        if (mAnimations[i].filename == name) {
            return AnimationId{i};
        }
    }
    mAnimations.push_back({name, std::unique_ptr<Video>(new Video)});
    return AnimationId{static_cast<unsigned int>(mAnimations.size()-1)};
}

ThorStatus AnimationMap::loadAnimations(Filesystem &filesystem) {
    std::unique_lock<std::mutex> lock{mMutex, std::defer_lock};

    size_t count = mAnimations.size();
    for (size_t i=0; i != count; ++i) {
        lock.lock();
        mProgress = static_cast<float>(i)/count;
        lock.unlock();

        Animation &anim = mAnimations[i];
        std::string cacheName = anim.filename + ".cache";
        std::vector<unsigned char> cached;
        ThorStatus status = filesystem.readCache(cacheName.c_str(), cached);
        if (!IS_THOR_ERROR(status)) {
            bool ok = anim.video->load(cached);
            if (ok) {
                continue;
            } else {
                LOGE("Failed to load cached video %s, loading from assets", anim.filename.c_str());
            }
        }

        bool ok = anim.video->decode(filesystem, anim.filename.c_str());
        if (!ok) {
            LOGE("Failed to decode video %s", anim.filename.c_str());
            lock.lock();
            mProgress = -1.0f;
            lock.unlock(); // don't need to unlock here, as return also unlocks, but just in case we add anything later
            return THOR_ERROR;
        }
        // Save cached video file (skipped if we read from the cache above)
        anim.video->save(cached);
        filesystem.writeCache(cacheName.c_str(), cached);
    }
    lock.lock();
    mProgress = 1.0f;
    lock.unlock(); // don't need to unlock here, as return also unlocks, but just in case we add anything later

    return THOR_OK;
}

float AnimationMap::getLoadProgress() const {
    std::unique_lock<std::mutex> lock{mMutex};
    return mProgress;
}

Video &AnimationMap::getAnimation(AnimationId animation) {
    static Video EMPTY;
    if (animation == AnimationId::none()) {
        return EMPTY;
    }
    if (mAnimations.size() <= animation.id) {
        LOGE("AnimationId %u out of range", animation.id);
        return EMPTY;
    }
    return *mAnimations[animation.id].video;
}


