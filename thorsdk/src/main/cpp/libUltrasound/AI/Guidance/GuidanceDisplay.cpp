//
// Copyright 2022 EchoNous Inc.
//
#define LOG_TAG "GuidanceDisplay"
#include <Guidance/GuidanceDisplay.h>
#include <Media/Video.h>
#include <CineViewer.h>
#include <ScanConverterParams.h>
#include <ViewMatrix3x3.h>
#include <Filesystem.h>
#include <UltrasoundManager.h>

GuidanceDisplay::GuidanceDisplay() :
    mAnimations(),
    mA4CAnimationIDs(),
    mA2CAnimationIDs(),
    mPLAXAnimationIDs(),
    mA4CPhrases(),
    mA2CPhrases(),
    mPLAXPhrases(),
    mCurrentSubview(Subview::Invalid()),
    mCurrentQuality(1),
    mAnimationsLoaded(false),
    mDrawGuidance(false),
    mDrawGrading(false),
    mVideoPlayer()
{}
GuidanceDisplay::~GuidanceDisplay() = default;

ThorStatus GuidanceDisplay::init(UltrasoundManager *usm) {
    ThorStatus status;
    Filesystem *fs = usm->getFilesystem();
    Json::Value locale;
    Json::Value config;

    status = fs->readAssetJson("trio_locale.json", locale);
    if (IS_THOR_ERROR(status)) {
        LOGE("Error %08x in %s:%d", status, __FILE__, __LINE__);
        return status;
    }
    mPhrases.loadLocaleMap(locale["guidance"]);

    status = fs->readAssetJson("mlconfig.json", config);
    if (IS_THOR_ERROR(status)) {
        LOGE("Error %08x in %s:%d", status, __FILE__, __LINE__);
        return status;
    }
    readViewConfig(config["guidance"]["views"]["A4C"], mA4CAnimationIDs, mA4CPhrases);
    readViewConfig(config["guidance"]["views"]["A2C"], mA2CAnimationIDs, mA2CPhrases);
    readViewConfig(config["guidance"]["views"]["PLAX"], mPLAXAnimationIDs, mPLAXPhrases);

    return THOR_OK;
}

ThorStatus GuidanceDisplay::loadAnimations(Filesystem &filesystem) {
    ThorStatus status = mAnimations.loadAnimations(filesystem);
    if (IS_THOR_ERROR(status))
        return status;
    mAnimationsLoaded = true;
    return THOR_OK;
}

void GuidanceDisplay::setEnableGuidance(bool enable) {
    mDrawGuidance = enable;
}

void GuidanceDisplay::setEnableQuality(bool enable) {
    mDrawGrading = enable;
}

void GuidanceDisplay::setLocale(const char *locale) {
    mPhrases.setLocale(locale);
}

void GuidanceDisplay::setSubview(Subview subview) {
    if (mCurrentSubview != subview) {
        char cur[Subview::MAX_SUBVIEW_NAME_LENGTH];
        char next[Subview::MAX_SUBVIEW_NAME_LENGTH];
        LOGD("%s: Changing subview from %s to %s",
             __FUNCTION__,
             mCurrentSubview.formatString(cur),
             subview.formatString(next));

        mCurrentSubview = subview;
        View targetView = subview.targetView();

        AnimationId animationId;
        switch (subview.targetView()) {
        case View::A4C:
            animationId = mA4CAnimationIDs[subview.index()];
            mCurrentPhrase = mA4CPhrases[subview.index()];
            break;
        case View::A2C:
            animationId = mA2CAnimationIDs[subview.index()];
            mCurrentPhrase = mA2CPhrases[subview.index()];
            break;
        case View::PLAX:
            animationId = mPLAXAnimationIDs[subview.index()];
            mCurrentPhrase = mPLAXPhrases[subview.index()];
            break;
        default:
            LOGE("Unsupported target view: %s", ViewToString(subview.targetView()));
            return;
        }

        Video *video = &mAnimations.getAnimation(animationId);
        mVideoPlayer.play(video);
    }
}

void GuidanceDisplay::setQuality(Quality quality) {
    mCurrentQuality = quality;
}

void GuidanceDisplay::draw(CineViewer &cineviewer, const ScanConverterParams &params) {
    if (mDrawGuidance) {
        if (!mAnimationsLoaded) {
            drawLoadingMessage();
        } else {
            mVideoPlayer.draw();
            drawPhrase(mCurrentPhrase);
        }
    }

    if (mDrawGrading) {
        drawQuality(cineviewer, params);
    }
}

void GuidanceDisplay::loadFont(Filesystem &filesystem) {
    auto *imguiFonts = ImGui::GetIO().Fonts;

    ImFontGlyphRangesBuilder builder;
    mPhrases.addPhraseTextRanges(&builder);

    mTextRanges.clear();
    builder.BuildRanges(&mTextRanges);
    const ImWchar *ranges = mTextRanges.begin();

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;

    const char * fonts[] = {
            "Roboto-Medium.ttf",
            "SourceHanSerifTC-Medium.otf"
    };
    for (const char* font : fonts)
    {
        std::vector<unsigned char> fontData;
        ThorStatus result = filesystem.readAsset(font, fontData);
        if (IS_THOR_ERROR(result)) {
            LOGE("Failed to load font file %s, skipping", font);
            continue;
        }
        LOGD("Loading font %s size %zu", font, fontData.size());

        mFont = imguiFonts->AddFontFromMemoryTTF(
                (void*)fontData.data(),
                fontData.size(),
                32,
                &config,
                ranges);

        // Turn on MergeMode after the first font has been added
        config.MergeMode = true;
    }
    LOGD("mFont = %p", mFont);
}

void GuidanceDisplay::openDrawable() {
    // reset current subview, to force reset the subview next call to `setSubview`
    mCurrentSubview = Subview::Invalid();
    mVideoPlayer.init();
    ImGuiIO& io = ImGui::GetIO();
    mVideoPlayer.setScreenSize(io.DisplaySize.x, io.DisplaySize.y);
    mVideoPlayer.setLocation(0,io.DisplaySize.y-295,395,295);
}
void GuidanceDisplay::closeDrawable() {
    mCurrentSubview == Subview::Invalid();
    mVideoPlayer.uninit();
}

void GuidanceDisplay::readViewConfig(const Json::Value &view, std::vector<AnimationId> &animIds, std::vector<PhraseId> &phraseIds) {
    animIds.clear();
    phraseIds.clear();
    const Json::Value &subviews = view["subviews"];
    const Json::Value &animations = view["guidanceAnimations"];
    const Json::Value &phrases = view["guidancePhrases"];

    for (const Json::Value &subview : subviews) {
        const Json::Value &animation = animations[subview.asString()];
        const Json::Value &phrase = phrases[subview.asString()];
        animIds.push_back(mAnimations.addAnimation(animation.asCString()));
        phraseIds.push_back(mPhrases.addPhrase(phrase.asCString()));

        //LOGD("Animation %s (for subview %s) has id %u", animation.asCString(), subview.asCString(), animIds.back().id);
        //LOGD("Phrase %s (for subview %s) has id %u", phrase.asCString(), subview.asCString(), phraseIds.back().id);
    }
}

void GuidanceDisplay::drawLoadingMessage() const {
    // We can assume the font is loaded, since that happens when display opens
    ImDrawList *background = ImGui::GetBackgroundDrawList();

    float progress = mAnimations.getLoadProgress();

    // Using same values as drawPhrase
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::SetNextWindowPos(ImVec2(65.0f, 5.0f));
    ImGui::SetNextWindowSize(ImVec2(550.0f, 0.0f));
    ImVec2 windowPos;
    ImVec2 windowSize;
    if (ImGui::Begin("Guidance Text", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
    {
        ImGui::PushFont(mFont);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 234, 0, 255));
        if (progress >= 0.0f) {
            ImGui::Text("Animations loading, progress = %0.0f%%", progress * 100.0f);
        } else {
            ImGui::TextUnformatted("Error loading animations...");
        }
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::PopFont();

        // Window position *should* be equal to mTextX, mTextY. But, read it here anyway to be sure.
        windowPos = ImGui::GetWindowPos();
        windowSize = ImGui::GetWindowSize();
    }
    ImGui::End();
    ImGui::PopStyleVar(2); // ImGuiStyleVar_WindowRounding and ImGuiStyleVar_WindowPadding

    ImVec2 bkgndMin   = ImVec2(windowPos.x-70, windowPos.y-5);
    ImVec2 bkgndMax   = ImVec2(windowPos.x+windowSize.x+5, windowPos.y+windowSize.y+5);

    background->AddRectFilled(bkgndMin, bkgndMax, IM_COL32(1,1,1,153));
}

void GuidanceDisplay::drawPhrase(PhraseId phrase) const {
    ImDrawList *background = ImGui::GetBackgroundDrawList();

    const std::string &text = mPhrases.getPhraseText(phrase);
    const char *startText = text.c_str();
    const char *endText = startText + text.size();

    //LOGD("Drawing phrase id %d, \"%s\"", phrase.id, startText);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::SetNextWindowPos(ImVec2(80.0f, 5.0f));
    ImGui::SetNextWindowSize(ImVec2(550.0f, 0.0f));
    ImVec2 windowPos;
    ImVec2 windowSize;
    if (ImGui::Begin("Guidance Text", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
    {
        ImGui::PushFont(mFont);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 234, 0, 255));
        ImGui::TextUnformatted(startText, endText);
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::PopFont();

        // Window position *should* be equal to mTextX, mTextY. But, read it here anyway to be sure.
        windowPos = ImGui::GetWindowPos();
        windowSize = ImGui::GetWindowSize();
    }
    ImGui::End();
    ImGui::PopStyleVar(2); // ImGuiStyleVar_WindowRounding and ImGuiStyleVar_WindowPadding

    ImVec2 bkgndMin   = ImVec2(windowPos.x-70, windowPos.y-5);
    ImVec2 bkgndMax   = ImVec2(windowPos.x+windowSize.x+5, windowPos.y+windowSize.y+5);

    background->AddRectFilled(bkgndMin, bkgndMax, IM_COL32(1,1,1,153));
}

void GuidanceDisplay::drawQuality(CineViewer &cineviewer, const ScanConverterParams &params) const {
    // TODO refactor this so it doesn't need the conversion to physical coords??
    float depth = params.startSampleMm +
                  (params.sampleSpacingMm * (params.numSamples - 1));
    float lineLen = depth / 5.0f; // length of each line in physical space
    float physToScreen[9];
    cineviewer.queryPhysicalToPixelMap(physToScreen, 9);
    float screenHeight = cineviewer.getImguiRenderer().getHeight();
    float screenToPhysScale = depth/screenHeight;

    float lineX = lineLen * cos(-params.startRayRadian);
    float lineY = lineLen * sin(-params.startRayRadian);

    // The values below used to be expressed in screen space coordinates,
    // but that led to issues when the image was flipped/not flipped. So instead, specify them
    // in physical coordinates and let physToScreen handle the flipping.
    // The odd values here approximately equal to the old values of 10 pixels and 5 pixels.
    float xDisplacement = 10.0f * screenToPhysScale; // physical space distance to offset lines so they don't overlap the cone
    float lineTrim = 5.0f * screenToPhysScale; // physical space distance that lines are shortened by on either end

    ImU32 qualityColor;
    if (mCurrentQuality.score() <= 2)
        qualityColor = ImColor(1.0f, 0.0f, 0.0f);
    else
        qualityColor =  ImColor(0.0f, 1.0f, 0.0f);
    // Quality lines on sides
    for (int l = 0; l < mCurrentQuality.score(); ++l) {
        // Physical coords
        float phys1[3] = {l * lineX + lineTrim + xDisplacement, l * lineY + lineTrim, 1.0f};
        float phys2[3] = {(l + 1) * lineX - lineTrim + xDisplacement, (l + 1) * lineY - lineTrim, 1.0f};

        float screen1[3];
        float screen2[3];
        ViewMatrix3x3::multiplyMV(screen1, physToScreen, phys1);
        ViewMatrix3x3::multiplyMV(screen2, physToScreen, phys2);

        ImVec2 o = ImVec2(screen1[0], screen1[1]);
        ImVec2 d = ImVec2(screen2[0], screen2[1]);
        ImGui::GetBackgroundDrawList()->AddLine(o, d, qualityColor, 10.0f);

        // draw flipped around x axis
        phys1[0] = -phys1[0];
        phys2[0] = -phys2[0];
        ViewMatrix3x3::multiplyMV(screen1, physToScreen, phys1);
        ViewMatrix3x3::multiplyMV(screen2, physToScreen, phys2);
        o = ImVec2(screen1[0], screen1[1]);
        d = ImVec2(screen2[0], screen2[1]);
        ImGui::GetBackgroundDrawList()->AddLine(o, d, qualityColor, 10.0f);
    }
}