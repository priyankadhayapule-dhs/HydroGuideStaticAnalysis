//
// Copyright 2022 EchoNous Inc.
//
#pragma once
#include "AnimationMap.h"
#include "PhraseMap.h"
#include <VideoPlayerGL.h>
#include <ThorError.h>
#include <imgui.h>

class UltrasoundManager;
class Video;
class CineViewer;
class Filesystem;
struct ScanConverterParams;

class GuidanceDisplay {
public:
    GuidanceDisplay();
    ~GuidanceDisplay();

    ThorStatus init(UltrasoundManager *usm);
    ThorStatus loadAnimations(Filesystem &filesystem);

    /// Set display of guidance (animation and phrase) on or off
    void setEnableGuidance(bool enable);
    /// Set display of quality lines on or off
    void setEnableQuality(bool enable);
    /// Set current locale for text (like 'en')
    void setLocale(const char *locale);
    /// Set current subview to display
    void setSubview(Subview subview);
    /// Set current quality to display
    void setQuality(Quality quality);
    /// Draw enabled features
    void draw(CineViewer &cineviewer, const ScanConverterParams &params);

    void loadFont(Filesystem &filesystem);
    void openDrawable();
    void closeDrawable();

private:
    void readViewConfig(const Json::Value &view, std::vector<AnimationId> &animIds, std::vector<PhraseId> &phraseIds);

    /// Draw the "Animations loading" message
    void drawLoadingMessage() const;

    /// Draw a guidance phrase
    void drawPhrase(PhraseId phrase) const;

    /// Draw quality lines
    void drawQuality(CineViewer &cineviewer, const ScanConverterParams &params) const;

    /// All animations used for all target views
    /// This is a single map so we de-duplicate across views
    AnimationMap mAnimations;

    /// Map of PhraseId to phrase text, supporting localization
    PhraseMap mPhrases;

    /// "Map" of A4C subview index to A4C animation id
    std::vector<AnimationId> mA4CAnimationIDs;
    /// "Map" of A2C subview index to A2C animation id
    std::vector<AnimationId> mA2CAnimationIDs;
    /// "Map" of PLAX subview index to PLAX animation id
    std::vector<AnimationId> mPLAXAnimationIDs;

    /// A4C subview index -> guidance phrase id
    std::vector<PhraseId> mA4CPhrases;
    /// A2C subview index -> guidance phrase id
    std::vector<PhraseId> mA2CPhrases;
    /// PLAX subview index -> guidance phrase id
    std::vector<PhraseId> mPLAXPhrases;

    /// Current subview. Initially set to invalid
    Subview mCurrentSubview;
    /// Current quality. Initially set to 1
    Quality mCurrentQuality;
    /// Current guidance phrase
    PhraseId mCurrentPhrase;

    /// True once animations are loaded
    bool mAnimationsLoaded;
    /// Whether guidance instructions should be drawn
    bool mDrawGuidance;
    /// Whether grading quality lines should be drawn
    bool mDrawGrading;

    /// Video renderer object
    VideoPlayerGL mVideoPlayer;

    /// ImGUI Font
    ImFont *mFont;
    /// Text ranges needed for font loading
    ImVector<ImWchar> mTextRanges;
};