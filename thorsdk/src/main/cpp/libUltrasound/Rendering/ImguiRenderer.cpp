//
// Copyright 2019 EchoNous Inc.
//

#define LOG_TAG "ImguiRenderer"
#define THOR_SHOW_NOT_FOR_CLINICAL_USAGE 0

#include <ThorUtils.h>
#include <ThorError.h>
#include <ImguiRenderer.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <ViewMatrix3x3.h>
#include <CardiacAnnotatorConfig.h>
#include <CineViewer.h>
#include <AnatomyClassifierConfig.h>
#include <AutoDepthGainPresetConfig.h>
#include <Filesystem.h>

#define FONT_NAME "Roboto-Bold.ttf"
#define SHOW_AUTOLABEL_CONFIG 0

ImguiRenderer::ImguiRenderer(CineViewer &cineviewer) :
    mCineViewer(cineviewer),
    mOpened(false),
    mIsA4C(true)
{
    // Always create ImGui context, since that doesn't touch opengl
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;

    auto &style = ImGui::GetStyle();
    style.ScrollbarSize = 30.0f;
    // Add additional padding to many elements to make them easier to select
    style.WindowPadding = ImVec2(8,8);
    style.FramePadding = ImVec2(8,8);
    style.ItemSpacing = ImVec2(8,8);
    style.GrabMinSize = 20.0f;

    mPresetChangeNeeded = false;
    mPresetClassId = 0;
}

void ImguiRenderer::addDrawable(ImGuiDrawable *drawable)
{
    mDrawables.push_back(drawable);
}

ThorStatus ImguiRenderer::load(Filesystem *filesystem) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    std::vector<unsigned char> fontData;
    ThorStatus result = filesystem->readAsset(FONT_NAME, fontData);
    if (IS_THOR_ERROR(result))
        return result;

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    for (auto *drawable : mDrawables)
        drawable->addTextRanges(builder);
    ImVector<ImWchar> ranges;
    builder.BuildRanges(&ranges);

    // Normal size text
    mNormalFont = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), 32.0f, &config, ranges.begin());
    // Large text
    mLargeFont = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), 64.0f, &config, ranges.begin());
    // Small text
    mSmallFont = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), 24.0f, &config, ranges.begin());

    // Load things like fonts, which must be done before font atlas is built
    LOGD("ImGuiRenderer::load - drawables.size = %zu", mDrawables.size());
    for (auto *drawable : mDrawables)
        drawable->loadDrawable(filesystem);


    // ImGUI no longer needs the font atlas to be built ahead of time??

//    // Build the font atlas before we have an opengl context
//    // The font atlas is just image data, no association with an opengl texture yet
//    // ImGui_ImplOpenGL3_Init actually creates the opengl texture
//    if (!io.Fonts->Build())
//    {
//        LOGE("Failed to build font atlas");
//        return THOR_ERROR;
//    }
//    LOGD("ImGui font atlas texture size = %dx%dx%d = %zu bytes",
//            io.Fonts->TexData->Width, io.Fonts->TexData->Height, 4,
//            (size_t)io.Fonts->TexData->GetSizeInBytes());

    return THOR_OK;
}

ThorStatus ImguiRenderer::open()
{
    ImGuiIO& io = ImGui::GetIO();
    LOGD("ImguiRenderer::open");
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        LOGE("Failed to init opengl3 impl");
        return THOR_ERROR;
    }
    LOGD("Setting Imgui display size to %d %d", getWidth(), getHeight());
    io.DisplaySize = ImVec2(getWidth(),getHeight());
    for (auto *drawable : mDrawables)
        drawable->openDrawable();
    mOpened = true;

    return THOR_OK;
}

void ImguiRenderer::close()
{
    // Close all drawables
    for (auto *drawable : mDrawables)
        drawable->close();

    if (mOpened)
    {
        ImGui_ImplOpenGL3_Shutdown();
        //ImGuiIO& io = ImGui::GetIO();
        //ImGui::DestroyContext();
    }
    mOpened = false;
}

bool ImguiRenderer::prepare() { return true; }


static const char *SubviewNames[] =
    {
         "A4C",  "Tilt Up",  "Tilt Down",  "Rotate clockwise",  "Rotate counter-clockwise",  "Slide lateral",  "Slide lateral and tilt down",  "Slide medial",  "Slide medial and tilt down",  "Slide 1 rib down",  "Slide lateral and slide 1 rib down",  "Slide medial and and slide 1 rib down",  "A2C",  "Unknown"
    };

void TweakFloat(const char *label, float &f)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s.L", label);
    if (ImGui::ArrowButton(buffer, ImGuiDir_Left))
        f -= 0.01f;
    ImGui::SameLine();
    snprintf(buffer, sizeof(buffer), "%s.R", label);
    if (ImGui::ArrowButton(buffer, ImGuiDir_Right))
        f += 0.01f;
    ImGui::SameLine();
    ImGui::SliderFloat(label, &f, 0.0f, 1.0f, "%0.2f");
}

uint32_t ImguiRenderer::draw()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO(); ((void)io);

    for (auto *drawable : mDrawables)
        drawable->draw(mCineViewer, mFrameNum);

    // Get conversion matrix to screen space coords
    float physToScreen[9];
    mCineViewer.queryPhysicalToPixelMap(physToScreen, 9);

    // Annotations are shown if any are present
    {
        std::unique_lock<std::mutex> lock(mMutex);
        ImU32 labelColor = ImColor(1.0f, 1.0f, 0.0f);

        for (auto &p : mPreds)
        {
            char labelBuffer[256];
            snprintf(labelBuffer, sizeof(labelBuffer), "%s",
                CARDIAC_ANNOTATOR_LABEL_NAMES[p.class_pred]);

            float posPhys[3] = {p.x, p.y, 1.0f};
            float posScreen[3];
            ViewMatrix3x3::multiplyMV(posScreen, physToScreen, posPhys);

            auto *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddCircleFilled(ImVec2(posScreen[0], posScreen[1]), 7.0f, labelColor);
            drawList->AddText(ImVec2(posScreen[0]+20.0f, posScreen[1]-18.0f), labelColor, labelBuffer);
        }

        // TODO: REMOVE - consolidate
        labelColor = ImColor(1.0f, 0.65f, 0.0f);

        for (auto &p : mPredsCombined)
        {
            char labelBuffer[256];
            snprintf(labelBuffer, sizeof(labelBuffer), "%s",
                     CARDIAC_ANNOTATOR_LABEL_NAMES[p.class_pred]);

            float posPhys[3] = {p.x, p.y, 1.0f};
            float posScreen[3];
            ViewMatrix3x3::multiplyMV(posScreen, physToScreen, posPhys);

            auto *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddCircleFilled(ImVec2(posScreen[0], posScreen[1]), 7.0f, labelColor);
            drawList->AddText(ImVec2(posScreen[0]+20.0f, posScreen[1]-18.0f), labelColor, labelBuffer);
        }

        // Anatomy Classifier
        float x_loc, y_loc, y_space;
        int cnt = 0;
        x_loc = 100.0f;
        y_loc = 100.0f;
        y_space = 50.0f;

        labelColor = ImColor(0.0f, 1.0f, 0.0f);

        for (auto &pr: mAnatomyPreds)
        {
            char labelBuffer[256];
            snprintf(labelBuffer, sizeof(labelBuffer), "%s: %f",
                     ANATOMY_CLASSES[cnt++], pr);

            auto *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddText(ImVec2(x_loc, y_loc), labelColor, labelBuffer);
            y_loc += y_space;
        }

        if (mPresetChangeNeeded)
        {
            char labelBuffer[256];
            x_loc = 500.0f;
            y_loc = 100.0f;

            snprintf(labelBuffer, sizeof(labelBuffer), "%s %s",
                     "NEED TO CHANGE PRESET / ORGAN TO ", ANATOMY_CLASSES[mPresetClassId]);

            auto *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddText(ImVec2(x_loc, y_loc), labelColor, labelBuffer);
        }

        // For Debugging - gain
        x_loc = 100.0f;
        y_loc = 500.0f;
        cnt = 0;
        for (auto &pr: mGainPreds)
        {
            char labelBuffer[256];
            snprintf(labelBuffer, sizeof(labelBuffer), "%s: %f",
                     GAIN_CLASSES[cnt++], pr);

            auto *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddText(ImVec2(x_loc, y_loc), labelColor, labelBuffer);
            y_loc += y_space;
        }


        // For Debugging - Depth
        y_loc = 800.0f;
        cnt = 0;
        for (auto &pr: mDepthPreds)
        {
            char labelBuffer[256];
            snprintf(labelBuffer, sizeof(labelBuffer), "%s: %f",
                     DEPTH_CLASSES[cnt++], pr);

            auto *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddText(ImVec2(x_loc, y_loc), labelColor, labelBuffer);
            y_loc += y_space;
        }

#if SHOW_AUTOLABEL_CONFIG
        ImGui::SetNextWindowPos(ImVec2(20,20));
        ImGui::SetNextWindowSize(ImVec2(500,-1));
        if (ImGui::Begin("Autolabel view", NULL, ImGuiWindowFlags_NoDecoration))
        {
            if (mViewProb.first <= CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES)
                ImGui::Text("View: %s (%0.3f)", CARDIAC_ANNOTATOR_VIEW_NAMES[mViewProb.first], mViewProb.second);
            else
                ImGui::Text("View index: %d", mViewProb.first);
        }
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(1000, -1));
        if (ImGui::Begin("Autolabel Config"))
        {
            float base_thr = CARDIAC_ANNOTATOR_PARAMS.base_label_threshold;
            float tweak_thrs[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
            std::copy(
                std::begin(CARDIAC_ANNOTATOR_PARAMS.label_thresholds),
                std::end(CARDIAC_ANNOTATOR_PARAMS.label_thresholds),
                std::begin(tweak_thrs));
            float view_thr = CARDIAC_ANNOTATOR_PARAMS.view_threshold;
            float label_frac = CARDIAC_ANNOTATOR_PARAMS.label_buffer_frac;

            TweakFloat("Base Label Thr", base_thr);
            if (ImGui::CollapsingHeader("Per-label Thresholds"))
            {
                for (int label=0; label != CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES; ++label)
                {
                    tweak_thrs[label] += base_thr;
                    TweakFloat(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[label], tweak_thrs[label]);
                    tweak_thrs[label] -= base_thr;
                }
            }
            TweakFloat("View Thr", view_thr);
            TweakFloat("Label Frac", label_frac);
            if (ImGui::Button("Reset to defaults"))
            {
                CARDIAC_ANNOTATOR_PARAMS.reset();
            }
            else
            {
                CARDIAC_ANNOTATOR_PARAMS.base_label_threshold = base_thr;
                std::copy(
                    std::begin(tweak_thrs), std::end(tweak_thrs),
                    std::begin(CARDIAC_ANNOTATOR_PARAMS.label_thresholds));
                CARDIAC_ANNOTATOR_PARAMS.view_threshold = view_thr;
                CARDIAC_ANNOTATOR_PARAMS.label_buffer_frac = label_frac;
            }

        }
        ImGui::End();
#endif
        
    }
#if THOR_SHOW_NOT_FOR_CLINICAL_USAGE
    ImDrawList *foreground = ImGui::GetForegroundDrawList();
    ImGui::PushFont(mNormalFont);
//void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
    const char *text = "NOT FOR DIAGNOSTIC USE";
    ImVec2 textSize = ImGui::CalcTextSize(text);
    foreground->AddText(ImVec2(getWidth()-20.0f-textSize.x, getHeight()-110.0f-textSize.y), IM_COL32(239, 45, 58, 255), text);
    ImGui::PopFont();
#endif /* THOR_SHOW_NOT_FOR_CLINICAL_USAGE */

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return 0;
}

void ImguiRenderer::setTouch(float x, float y, bool isDown)
{
    if (mOpened)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos.x = x;
        io.MousePos.y = y;
        io.MouseDown[0] = isDown;
    }
}

void ImguiRenderer::setA4C(bool isA4C)
{
    mIsA4C = isA4C;
}

void ImguiRenderer::setPredictions(const std::vector<YoloPrediction> &preds)
{
    std::unique_lock<std::mutex> lock(mMutex);
    //LOGD("Setting predictions to vector of size %lu", preds.size());
    mPreds = preds;
    /*
    mPreds.push_back(YoloPrediction{
        500,500,
        10,10,
        0.5f, 0.5f,
        1
    });
    */
}

void ImguiRenderer::setPredictionsCombined(const std::vector<YoloPrediction> &preds)
{
    std::unique_lock<std::mutex> lock(mMutex);
    mPredsCombined = preds;
}

void ImguiRenderer::setAnnotatorView(std::pair<int, float> viewProb)
{
    mViewProb = viewProb;
}

void ImguiRenderer::setAnatomyClassification(std::vector<float> pred)
{
    mAnatomyPreds = pred;
}

void ImguiRenderer::setGainPred(std::vector<float> pred)
{
    mGainPreds = pred;
}

void ImguiRenderer::setDepthPred(std::vector<float> pred)
{
    mDepthPreds = pred;
}

void ImguiRenderer::setPresetChange(bool needed, int classId)
{
    mPresetChangeNeeded = needed;
    mPresetClassId = classId;
}

void ImguiRenderer::setFrameNum(uint32_t frameNum)
{
    mFrameNum = frameNum;
}