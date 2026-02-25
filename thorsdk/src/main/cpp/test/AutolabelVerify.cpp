//
// Created by tcros on 5/22/2020.
//

#include <cstdio>
#include <cstring>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <CardiacAnnotator.h>
#include <UltrasoundManager.h>
#include <CineBuffer.h>
#include <YoloBuffer.h>
#include <json/json.h>

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

void AutolabelFile(std::unique_ptr<UltrasoundManager> &manager, const char *path)
{
    THOR_TRY(manager->getCinePlayer().openRawFile(path));

    auto scanconverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
    auto params = manager->getCineBuffer().getParams().converterParams;
    // Want to convert so that the edges are in frame
    float scaleX = (CARDIAC_ANNOTATOR_INPUT_WIDTH*0.5f)/(CARDIAC_ANNOTATOR_INPUT_HEIGHT*sin(-params.startRayRadian));
    float endImgDepth = params.startSampleMm +
                        (params.sampleSpacingMm * (params.numSamples - 1));
    float flipX = -1.0f * scaleX;
    scanconverter->setConversionParameters(params.numSamples,
                                           params.numRays,
                                           CARDIAC_ANNOTATOR_INPUT_WIDTH,
                                           CARDIAC_ANNOTATOR_INPUT_HEIGHT,
                                           0,
                                           endImgDepth,
                                           params.startSampleMm,
                                           params.sampleSpacingMm,
                                           params.startRayRadian,
                                           params.raySpacing,
                                           flipX,
                                           1.0f);
    THOR_TRY(scanconverter->init());

    auto annotator = std::unique_ptr<CardiacAnnotator>(new CardiacAnnotator);
    THOR_TRY(annotator->init(*manager.get()));

    auto ybuffer = std::unique_ptr<YoloBuffer>(new YoloBuffer(15));

    // Output JSON file
    Json::Value root(Json::ValueType::objectValue);
    root["file"] = path;
    root["frames"] = Json::Value(Json::ValueType::arrayValue);

    // Intermediate storage
    std::vector<float> postscan(CARDIAC_ANNOTATOR_INPUT_WIDTH*CARDIAC_ANNOTATOR_INPUT_HEIGHT);
    std::vector<YoloPrediction> predictions;
    std::vector<float> view;

    // Main conversion loop
    auto &cinebuffer = manager->getCineBuffer();
    uint32_t maxFrame = cinebuffer.getMaxFrameNum()+1;
    std::vector<float> postscan_all;
    std::vector<float> viewlayer;
    std::vector<float> boxlayer1;
    std::vector<float> boxlayer2;
    std::vector<float> procbox1;
    std::vector<float> procbox2;
    std::vector<YoloPrediction> detections_by_label[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
    for (uint32_t frameNum=0; frameNum != maxFrame; ++frameNum)
    {
        auto &jsonFrame = root["frames"][frameNum];
        jsonFrame["frameNum"] = frameNum;

        uint8_t *prescan = cinebuffer.getFrame(frameNum, DAU_DATA_TYPE_B, CineBuffer::FrameContents::DataOnly);
        THOR_TRY(scanconverter->runCLTex(prescan, postscan.data()));

        predictions.clear();
        //THOR_TRY(annotator->annotate(postscan.data(), view, predictions));
        std::copy(postscan.begin(), postscan.end(), std::back_inserter(postscan_all));
        THOR_TRY(annotator->setInput(postscan.data()));
        THOR_TRY(annotator->runModel());
        std::copy(annotator->mX0.begin(), annotator->mX0.end(), std::back_inserter(viewlayer));
        std::copy(annotator->mX1.begin(), annotator->mX1.end(), std::back_inserter(boxlayer1));
        std::copy(annotator->mX2.begin(), annotator->mX2.end(), std::back_inserter(boxlayer2));
        THOR_TRY(annotator->processClassLayer(view));
        THOR_TRY(annotator->processYoloLayer(annotator->mX1, CARDIAC_ANNOTATOR_ANCHOR_INFOS[0]));
        THOR_TRY(annotator->processYoloLayer(annotator->mX2, CARDIAC_ANNOTATOR_ANCHOR_INFOS[1]));
        std::copy(annotator->mX1.begin(), annotator->mX1.end(), std::back_inserter(procbox1));
        std::copy(annotator->mX2.begin(), annotator->mX2.end(), std::back_inserter(procbox2));
        THOR_TRY(annotator->postprocessYoloOutputExt(annotator->mX1, annotator->mX2, detections_by_label, predictions));

        for (float conf : view) { jsonFrame["viewPred"].append(conf);}
        for (const YoloPrediction &pred : predictions)
        {
            Json::Value jsonPred;
            jsonPred["class_pred"] = pred.class_pred;
            jsonPred["class_conf"] = pred.class_conf;
            jsonPred["conf_product"] = pred.class_conf * pred.obj_conf;
            jsonPred["obj_conf"]   = pred.obj_conf;
            jsonPred["x"] = pred.x;
            jsonPred["y"] = pred.y;
            jsonPred["w"] = pred.w;
            jsonPred["h"] = pred.h;
            jsonFrame["predictions"].append(jsonPred);
        }

        for (const std::vector<YoloPrediction> &label_prediction : detections_by_label)
        {
            Json::Value json_label_prediction(Json::ValueType::arrayValue);
            for (const YoloPrediction &pred : label_prediction)
            {
                Json::Value jsonPred;
                jsonPred["class_pred"] = pred.class_pred;
                jsonPred["class_conf"] = pred.class_conf;
                jsonPred["conf_product"] = pred.class_conf * pred.obj_conf;
                jsonPred["obj_conf"]   = pred.obj_conf;
                jsonPred["x"] = pred.x;
                jsonPred["y"] = pred.y;
                jsonPred["w"] = pred.w;
                jsonPred["h"] = pred.h;
                json_label_prediction.append(jsonPred);
            }
            jsonFrame["pre_NMS_preds"].append(json_label_prediction);
        }

        ybuffer->append(view, predictions);
        const auto &smooth = ybuffer->get();
        for (const YoloPrediction &pred : smooth)
        {
            Json::Value jsonPred;
            jsonPred["class_pred"] = pred.class_pred;
            jsonPred["class_conf"] = pred.class_conf;
            jsonPred["obj_conf"]   = pred.obj_conf;
            jsonPred["x"] = pred.x;
            jsonPred["y"] = pred.y;
            jsonPred["w"] = pred.w;
            jsonPred["h"] = pred.h;
            jsonFrame["smoothed"].append(jsonPred);
        }

        Json::Value jsonViewFlat = Json::Value(Json::ValueType::arrayValue);
        for (float f : ybuffer->mViewFlat) { jsonViewFlat.append(f); }
        jsonFrame["smoothView"] = jsonViewFlat;
    }
    fprintf(stderr, "Processed %u frames\n", maxFrame);

    const char *id_begin = strchr(path, '/')+1;
    const char *id_end = strchr(path, '.');
    char outpath[128];
    snprintf(outpath, sizeof(outpath), "Autolabel_Validation_JSON/%.*s.json",
             static_cast<int>(id_end - id_begin), id_begin);

    std::ofstream os(outpath);
    os << root << "\n";

    /*

    snprintf(outpath, sizeof(outpath), "Autolabel_Validation_RAW/%.*s_scanconv.raw",
             static_cast<int>(id_end - id_begin), id_begin);
    FILE *file = fopen(outpath, "wb");
    fwrite(postscan_all.data(), sizeof(float), postscan_all.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "Autolabel_Validation_RAW/%.*s_view.raw",
             static_cast<int>(id_end - id_begin), id_begin);
    file = fopen(outpath, "wb");
    fwrite(viewlayer.data(), sizeof(float), viewlayer.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "Autolabel_Validation_RAW/%.*s_box1.raw",
             static_cast<int>(id_end - id_begin), id_begin);
    file = fopen(outpath, "wb");
    fwrite(boxlayer1.data(), sizeof(float), boxlayer1.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "Autolabel_Validation_RAW/%.*s_box2.raw",
             static_cast<int>(id_end - id_begin), id_begin);
    file = fopen(outpath, "wb");
    fwrite(boxlayer2.data(), sizeof(float), boxlayer2.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "Autolabel_Validation_RAW/%.*s_proc1.raw",
             static_cast<int>(id_end - id_begin), id_begin);
    file = fopen(outpath, "wb");
    fwrite(procbox1.data(), sizeof(float), procbox1.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "Autolabel_Validation_RAW/%.*s_proc2.raw",
             static_cast<int>(id_end - id_begin), id_begin);
    file = fopen(outpath, "wb");
    fwrite(procbox2.data(), sizeof(float), procbox2.size(), file);
    fclose(file);

     */
}

int main()
{
    auto manager = std::unique_ptr<UltrasoundManager>(new UltrasoundManager);
    THOR_TRY(manager->openDirect("assets"));

    // Ignore mkdir error, which can happen if the dir already exists
    mkdir("Autolabel_Validation_JSON", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    mkdir("Autolabel_Validation_RAW", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    DIR* dir = opendir("Autolabel_Validation_THOR");
    if (!dir)
    {
        fprintf(stderr, "Failed to open directory, errno = %d\n", errno);
        return 1;
    }

    struct dirent *entry;
    char path[128];

    while ((entry = readdir(dir)))
    {
        if (!strstr(entry->d_name, ".thor"))
            continue;

        //if (0 != strcmp(entry->d_name, "11012019f39d49eb-4dcb-45d5-9bce-c8bbf76fb92b.thor"))
        //    continue;

        snprintf(path, sizeof(path), "Autolabel_Validation_THOR/%s", entry->d_name);
        printf("Opening file %s\n", path);
        AutolabelFile(manager, path);
        //break;
    }
    closedir(dir);

    return 0;
}
