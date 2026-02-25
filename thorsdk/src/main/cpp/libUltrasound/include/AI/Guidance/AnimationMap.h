//
// Copyright 2022 EchoNous Inc.
//
#pragma once
#include "GuidanceTypes.h"
#include <Filesystem.h>
#include <ThorError.h>
#include <memory>

// Forward declarations
class Video;

class AnimationMap {
public:
    AnimationMap();
    ~AnimationMap();

    /// Adds an animation by file name, returning AnimationId
    /// This function de-duplicates animations with the same name
    /// Animation data is not actually loaded until `loadAnimations` is called
    AnimationId addAnimation(const char *name);

    /// Loads all animations added by `addAnimations`. This takes some time,
    /// so should be called on a non-critical thread.
    /// After this function is called, no new animations should be added.
    ThorStatus loadAnimations(Filesystem &filesystem);

    /// Gets loading progress, from 0 to 1, or negative if error
    /// Uses synchronization internally, so may be called from any thread
    float getLoadProgress() const;

    /// Gets the video for a given id
    Video& getAnimation(AnimationId animation);

private:
    struct Animation {
        std::string filename;
        std::unique_ptr<Video> video; // Using unique_ptr here so we can forward declare Video
    };

    /// AnimationId -> Animation
    std::vector<Animation> mAnimations;

    /// Mutex protecting mProgress
    mutable std::mutex mMutex;
    /// Loading progress from 0 to 1, or negative if error
    float mProgress = 0.0f;
};