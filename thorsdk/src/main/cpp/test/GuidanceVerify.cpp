#include <UltrasoundManager.h>
#include <EFQualitySubviewConfig.h>
#include <EFQualitySubviewTask.h>
#include <CineBuffer.h>
#include <fstream>
#include <json/json.h>
#include <dirent.h>
#include <cstdio>

#define THOR_TRY(expr) ThorAssertOk(expr, #expr, __FILE__, __LINE__)
void ThorAssertOk(ThorStatus result, const char *expr, const char *file, int line)
{
    if (IS_THOR_ERROR(result))
    {
        fprintf(stderr, "Thor Error code 0x%08x in \"%s\" (%s:%d)\n", static_cast<uint32_t>(result), expr, file, line);
        fflush(stdout);
        std::abort();
    }
}

void fwriteall(const void *ptr, size_t size, size_t count, FILE *stream)
{
    while (count)
    {
        size_t written = fwrite(ptr, size, count, stream);
        if (written == 0)
        {
            perror("fwrite failed");
            std::abort();
        }
        ptr = (const void*)((const char*)ptr + written*size);
        count -= written;
    }
}

template <typename Iterable>
Json::Value json_list(Iterable &&iterable) {
    using std::begin;
    using std::end;
    auto first = begin(iterable);
    auto last  = end(iterable);

    Json::Value list;
    while (first != last) {
        list.append(*first++);
    }
    return list;
}

Json::Value json_pred(const EFQualitySubviewPred &pred) {
    Json::Value root;
    root["subview"]    = json_list(pred.subview);
    root["a4cQuality"] = json_list(pred.a4cQuality);
    root["a2cQuality"] = json_list(pred.a2cQuality);
    return root;
}

Json::Value RunGuidanceFrame(EFQualitySubviewTask &guidance, CineBuffer &cinebuffer, uint32_t frameNum, FILE *raw) {
    // All intermediate values
    std::vector<float> scanframe;
    EFQualitySubviewPred rawPred;
    EFQualitySubviewPred procPred;
    EFQualitySubviewPred smoothPred;

    // Scan convert frame
    THOR_TRY(guidance.scanConvert(cinebuffer, frameNum, scanframe));

    // Output scan converted frame
    fwriteall(scanframe.data(), sizeof(float), scanframe.size(), raw);

    // Run model
    THOR_TRY(guidance.runModel(scanframe, rawPred));

    // Post process and smooth prediction
    procPred = rawPred;
    guidance.postProcessPred(procPred);
    guidance.enqueuePred(procPred);
    smoothPred = guidance.computeMean();

    // Get thresholded subview and rescaled quality
    auto subview = std::max_element(std::begin(smoothPred.subview), std::end(smoothPred.subview));
    auto subviewIndex = std::distance(std::begin(smoothPred.subview), subview);
    int resultSubview;
    if (*subview > guidance.mConfig.subviewTau[subviewIndex])
        resultSubview = subviewIndex;
    else
        resultSubview = 17; // uncertain

    int resultQuality = guidance.rescaleQuality(resultSubview, smoothPred, guidance.mConfig);

    // Jsonify output
    Json::Value root;
    root["rawPred"]      = json_pred(rawPred);
    root["procPred"]     = json_pred(procPred);
    root["smoothPred"]   = json_pred(smoothPred);
    root["subviewIndex"] = resultSubview;
    root["qualityScore"] = resultQuality+1;

    return root;
}

void RunGuidanceClip(UltrasoundManager& manager, const char *clip) {
    auto &cinebuffer = manager.getCineBuffer();
    auto &cineplayer = manager.getCinePlayer();
    auto &ai = manager.getAIManager();
    auto &guidance = *(ai.getEFQualityTask());

    printf("Running guidance on clip %s\n", clip);

    int clipIDLen = static_cast<int>(strchr(clip, '.')-clip);

    // Load clip into cinebuffer
    char path[128];
    snprintf(path, sizeof(path), "guidance/thor/%s", clip);
    THOR_TRY(cineplayer.openRawFile(path));
    guidance.setParams(cinebuffer.getParams());

    snprintf(path, sizeof(path), "guidance/raw/%.*s.raw", clipIDLen, clip);
    FILE *raw = fopen(path, "wb");

    Json::Value root;
    uint32_t maxFrame = cinebuffer.getMaxFrameNum();
    for (uint32_t frameNum = 0; frameNum != maxFrame; ++frameNum) {
        root["frames"].append(RunGuidanceFrame(guidance, cinebuffer, frameNum, raw));
    }

    fclose(raw);

    snprintf(path, sizeof(path), "guidance/json/%.*s.json", clipIDLen, clip);
    std::ofstream out(path);
    out << root << "\n";
}

int main()
{
    auto manager = std::unique_ptr<UltrasoundManager>(new UltrasoundManager);
    THOR_TRY(manager->openDirect("assets"));
    auto &cinebuffer = manager->getCineBuffer();
    auto &cineplayer = manager->getCinePlayer();
    auto &ai = manager->getAIManager();
    auto &guidance = *(ai.getEFQualityTask());
    guidance.setView(CardiacView::A2C);

    guidance.init(nullptr); // No asset manager, so image loading will fail, but that's not needed for this test
    // No need to call start() on guidance, this test will not use the background thread

    DIR* dir = opendir("guidance/thor");
    if (!dir)
    {
        fprintf(stderr, "Failed to open directory, errno = %d\n", errno);
        return 1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (!strstr(entry->d_name, ".thor"))
            continue;

        RunGuidanceClip(*manager, entry->d_name);
        guidance.mPredictions.clear(); // clear temporal buffer
    }
    closedir(dir);

    return 0;
}
