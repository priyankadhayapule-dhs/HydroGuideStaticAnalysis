//
// Copyright 2019 EchoNous Inc.
//

#pragma once

#include <atomic>
#include <utility>
#include <unordered_map>
#include <string>
#include <Renderer.h>
#include <CardiacAnnotatorConfig.h>
#include <mutex>
#include <vector>
#include <imgui.h>

class CineViewer;
class Filesystem;
class CardiacAnnotatorTask;

class ImGuiDrawable
{
public:
    /// Add necessary text ranges to builder for default font
    virtual void addTextRanges(ImFontGlyphRangesBuilder& builder) {}
    // Load any assets like font data into ImGui font atlas
    virtual void loadDrawable(Filesystem* filesystem) {}
    // Open: prepare any opengl data types
    virtual void openDrawable() {}
    virtual void draw(CineViewer &cineviewer, uint32_t frameNum) = 0;
    virtual void close() {}
};

class ImguiRenderer : public Renderer
{
public:
    ImguiRenderer(CineViewer &cineviewer);

    void addDrawable(ImGuiDrawable *drawable);

    ThorStatus load(Filesystem *filesystem);

    // Renderer functions
    virtual ThorStatus open() override;
    virtual void close() override;
    virtual bool prepare() override;
    virtual uint32_t draw() override;

    void setFrameNum(uint32_t frameNum);
    void setTouch(float x, float y, bool isDown);

    void setA4C(bool isA4C);
    void setPredictions(const std::vector<YoloPrediction> &preds);
    void setPredictionsCombined(const std::vector<YoloPrediction> &preds);
    void setAnnotatorView(std::pair<int, float> viewProb);

    void setAnatomyClassification(std::vector<float> preds);
    void setPresetChange(bool needed, int classId);

    void setGainPred(std::vector<float> preds);
    void setDepthPred(std::vector<float> preds);

private:
    CineViewer &mCineViewer;
    bool mOpened;
    std::atomic<bool> mIsA4C;
    uint32_t mFrameNum;

    std::mutex mMutex;
    std::vector<YoloPrediction> mPreds;
    std::vector<YoloPrediction> mPredsCombined;
    std::pair<int, float> mViewProb;
    std::vector<float> mAnatomyPreds;
    bool mPresetChangeNeeded;
    int  mPresetClassId;

    std::vector<float> mGainPreds;
    std::vector<float> mDepthPreds;

    std::vector<ImGuiDrawable*> mDrawables;

    ImFont *mNormalFont;
    ImFont *mLargeFont;
    ImFont *mSmallFont;
};
