#include <sys/stat.h>

#include <UltrasoundManager.h>
#include <getopt.h>
#include <cstdio>
#include <vector>
#include <AIManager.h>
#include <AIModel.h>
#include <LVSegmentation.h>

#include <PhaseDetection.h>
#include <ImageUtils.h>
#include <EFWorkflow.h>

namespace echonous
{
    extern UltrasoundManager gUltrasoundManager;
}

void usage(const char *prog)
{
    printf("usage: %s [-u|--ups UPS_FILE] A4C_CLIP [A2C_CLIP]\n", prog);
}

bool parse_args(int argc, char **argv, const char **ups, const char **a4c, const char **a2c)
{
    int opt;
    static struct option long_options[] = {
        {"ups", required_argument, 0, 'u'},
        {0,0,0,0}
    };

    *ups = "thor.db";

    while (-1 != (opt = getopt_long(argc, argv, "u:", long_options, nullptr)))
    {
        switch(opt)
        {
        case 'u':
            *ups = optarg;
            break;
        default:
            return false;
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "must provide path to a4c clip file.\n");
        return false;
    }

    *a4c = argv[optind];
    if (optind+1 < argc)
        *a2c = argv[optind+1];
    else
        *a2c = nullptr;

    return true;
}

std::string slurp(const char *path)
{
    std::string result;

    FILE *f = fopen(path, "rb");
    if (!f) { return result; }
    fseek(f, 0, SEEK_END);
    auto len = ftell(f);
    fseek(f, 0, SEEK_SET);

    result.resize(len);
    if (1 != fread(&result[0], len, 1, f))
    {
        return "";
    }
    return result;
}

std::unique_ptr<AIModel> loadModel(const char *path, const std::vector<std::string>& outputLayers, const std::vector<std::string>& outputNodes)
{
    ThorStatus status;
    AIModel::Params params = {
        slurp(path),
        outputLayers,
        outputNodes
    };

    std::unique_ptr<AIModel> model(new AIModel);
    status = model->build(params);
    if (IS_THOR_ERROR(status))
    {
        return nullptr;
    }
    return model;
}

ThorStatus phase_detect(CineBuffer *cb, FILE *out, int *pEdFrame, int *pEsFrame)
{
    ThorStatus err;
    //auto model = echonous::gUltrasoundManager.getAIManager().getModel("phase_detection.dlc", {"fully_connected_0"}, {"177"});
    auto model = loadModel("/data/local/thortests/assets/phase_detection_regression_080819.dlc", {"fully_connected_0"}, {"183"});
    if (!model)
    {
        return TER_AI_MODEL_LOAD;
    }

    // Find model width and height
    ivec3 size = model->inputSize();

    std::vector<float> frameBuffer;
    std::vector<cv::Mat> frames;

    err = echonous::phase::ScanConvert(cb, cv::Size(size.x, size.y), frameBuffer, &frames);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    
    mkdir("/data/local/thortests/phase_output", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    for (unsigned i = 0; i < frames.size(); ++i)
    {
        char path[128];
        snprintf(path, sizeof(path), "/data/local/thortests/phase_output/frame%03i.pgm", i);
        WriteImg(path, frames[i], true);
    }
    

    // write json by hand

    std::vector<float> phase;
    err = echonous::phase::RunModel(model.get(), frames, phase);
    if (IS_THOR_ERROR(err))
    {
        fprintf(stderr, "Error running model\n");
        return err;
    }

    fprintf(out, "\"area_estimate\":[%f", phase[0]);
    for (unsigned i = 1; i < phase.size(); ++i)
    {
        fprintf(out, ", %f", phase[i]);
    }
    fprintf(out, "],\n");

    int esframe, edframe;
    {
        std::vector<float> normalized_phase;
        echonous::phase::normalize(phase.begin(), phase.end(), std::back_inserter(normalized_phase));

        typedef std::vector<float>::iterator iterator;
        std::vector<iterator> ed_frames_raw, es_frames_raw;

        // Determine window size as half the period
        size_t T = echonous::phase::period(normalized_phase.begin(), normalized_phase.end())+1;
        if (T == 0)
            T = 20;
        
        fprintf(out, "\"T\": %lu,\n", T);

        es_frames_raw.push_back(normalized_phase.begin());

        // Find local minima/maxima
        echonous::phase::local_minmax(normalized_phase.begin(), normalized_phase.end(), T, std::back_inserter(ed_frames_raw), std::back_inserter(es_frames_raw));

        fprintf(out, "\"es_frames_raw\": [%ld", es_frames_raw[0]-normalized_phase.begin());
        for (unsigned i = 1; i < es_frames_raw.size(); ++i)
        {
            fprintf(out, ", %ld", es_frames_raw[i]-normalized_phase.begin());
        }
        fprintf(out, "],\n");

        fprintf(out, "\"ed_frames_raw\": [%ld", ed_frames_raw[0]-normalized_phase.begin());
        for (unsigned i = 1; i < ed_frames_raw.size(); ++i)
        {
            fprintf(out, ", %ld", ed_frames_raw[i]-normalized_phase.begin());
        }
        fprintf(out, "],\n");

        // for each es frame, find the corresponding ed frame (or estimate via period)
        std::vector<iterator> es_frames_mapped;
        std::vector<iterator> ed_frames_mapped;
        auto cur_ed_frame = ed_frames_raw.begin();
        auto next_ed_frame = cur_ed_frame+1;
        for (size_t i=1; i != es_frames_raw.size(); ++i)
        {
            iterator cycle_start = es_frames_raw[i-1];
            iterator cycle_end = es_frames_raw[i];

            // advance ed frame iterators
            // next points one past the current cycle, so cur points to the last ed frame in the cycle
            while (next_ed_frame != ed_frames_raw.end() && *next_ed_frame < cycle_end)
            {
                ++cur_ed_frame;
                ++next_ed_frame;
            }

            if (cycle_start < *cur_ed_frame && *cur_ed_frame <= cycle_end)
            {
                ed_frames_mapped.push_back(*cur_ed_frame);
                es_frames_mapped.push_back(cycle_end);
            }
            else
            {
                // estimate using previous ed frame (if any) if the value would be in the cycle
                if (!ed_frames_mapped.empty() && ed_frames_mapped.back()+T < cycle_end)
                {
                    ed_frames_mapped.push_back(ed_frames_mapped.back()+T);
                    es_frames_mapped.push_back(cycle_end);
                }
            }
        }

        if (ed_frames_mapped.empty())
        {
            return TER_AI_FRAME_ID;
        }

        fprintf(out, "\"es_frames_mapped\": [%ld", es_frames_mapped[0]-normalized_phase.begin());
        for (unsigned i = 1; i < es_frames_mapped.size(); ++i)
        {
            fprintf(out, ", %ld", es_frames_mapped[i]-normalized_phase.begin());
        }
        fprintf(out, "],\n");

        fprintf(out, "\"ed_frames_mapped\": [%ld", ed_frames_mapped[0]-normalized_phase.begin());
        for (unsigned i = 1; i < ed_frames_mapped.size(); ++i)
        {
            fprintf(out, ", %ld", ed_frames_mapped[i]-normalized_phase.begin());
        }
        fprintf(out, "],\n");

        // select the pair of ed/es frames which are closest to the mean "ef"
        std::vector<float> estimated_ef;
        for (unsigned i=0; i < ed_frames_mapped.size(); ++i)
        {
            float ef = (phase[ed_frames_mapped[i]-normalized_phase.begin()] - phase[es_frames_mapped[i]-normalized_phase.begin()]) / phase[ed_frames_mapped[i]-normalized_phase.begin()];
            estimated_ef.emplace_back(ef);
        }

        fprintf(out, "\"estimated_ef\": [%f", estimated_ef[0]);
        for (unsigned i = 1; i < estimated_ef.size(); ++i)
        {
            fprintf(out, ", %f", estimated_ef[i]);
        }
        fprintf(out, "],\n");

        float mean_ef = std::accumulate(estimated_ef.begin(), estimated_ef.end(), 0.0f) / estimated_ef.size();
        auto mean_iter = std::min_element(estimated_ef.begin(), estimated_ef.end(), [=](float a, float b)
            {
                return std::abs(a-mean_ef) < std::abs(b-mean_ef);
            });
        auto mean_idx = std::distance(estimated_ef.begin(), mean_iter);

        edframe = ed_frames_mapped[mean_idx] - normalized_phase.begin();
        esframe = es_frames_mapped[mean_idx] - normalized_phase.begin();
    }

    fprintf(out, "\"ed_frame\": %d,\n\"es_frame\": %d", edframe, esframe);

    *pEdFrame = edframe;
    *pEsFrame = esframe;

    return THOR_OK;
}

ThorStatus segment(CineBuffer *cb, EFWorkflow *w, int frameNum, bool isES, FILE *out, CardiacSegmentation &segOut, CardiacView view)
{
    ThorStatus err;
    std::shared_ptr<AIModel> model1, model2, model3;

    if (isES)
    {
        //LOGD("Using combined EDES ensemble on frame %d", mFrame);
        model1 = loadModel("/data/local/thortests/assets/lv_segmentation_window_es_1_111219.dlc", {"prelu_12"}, {"169"});
        model2 = loadModel("/data/local/thortests/assets/lv_segmentation_window_es_2_111219.dlc", {"prelu_12"}, {"169"});
        model3 = loadModel("/data/local/thortests/assets/lv_segmentation_window_es_3_111219.dlc", {"prelu_12"}, {"169"});
    }
    else
    {       
        //LOGD("Using ED emsemble to segment frame %d", mFrame);
        model1 = loadModel("/data/local/thortests/assets/lv_segmentation_window_ed_1.dlc", {"prelu_12"}, {"169"});
        model2 = loadModel("/data/local/thortests/assets/lv_segmentation_window_ed_2.dlc", {"prelu_12"}, {"169"});
        model3 = loadModel("/data/local/thortests/assets/lv_segmentation_window_ed_3.dlc", {"prelu_12"}, {"169"});
    }

    if (!model1 || !model2 || !model3)
    {
        return TER_AI_MODEL_LOAD;
    }

    // Find model width and height
    ivec3 size = model1->inputSize();

    // Scan converted frame
    cv::Mat frame;
    err = echonous::lv::ScanConvertWindow(cb, cv::Size(size.x, size.y), frameNum, frame);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    // run model (WARNING: hard coding the number of output classes/labels to 3)
    int numLabels = 3;
    cv::Mat probMap;
    AIModel *ensemble[] = {model1.get(), model2.get(), model3.get()};
    err = echonous::lv::RunEnsemble(ensemble, sizeof(ensemble)/sizeof(ensemble[0]), frame, numLabels, probMap);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    // Flatten to most likely label per pixel
    cv::Mat labelMap;
    // Allow from 1 to 5 labels?
    switch (numLabels)
    {
    case 2:
        err = echonous::lv::ChannelMax<2>(probMap, labelMap);
        break;
    case 3:
        err = echonous::lv::ChannelMax<3>(probMap, labelMap);
        break;
    case 4:
        err = echonous::lv::ChannelMax<4>(probMap, labelMap);
        break;
    case 5:
        err = echonous::lv::ChannelMax<5>(probMap, labelMap);
        break;
    default:
        LOGE("Unsupported number of labels: %d", size.z);
        err = TER_AI_SEGMENTATION;
    }
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    // WARNING: hard coding the index of the LV label to be 1. This may need to change if the
    // models change
    cv::Mat lv;
    err = echonous::lv::LargestComponent(labelMap, 1, lv);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    mkdir("/data/local/thortests/seg_output", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    {
        char path[128];
        snprintf(path, sizeof(path), "/data/local/thortests/seg_output/%s_mask%03i.pgm", view == CardiacView::A4C ? "a4c":"a2c", frameNum);
        WriteImg(path, lv, false);
    }

    std::vector<cv::Point> denseContour;
    err = echonous::lv::DenseContour(lv, denseContour);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    std::vector<int> splineIndices;
    err = echonous::lv::ShapeDetect(denseContour, splineIndices);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    std::vector<float> arcLength;
    err = echonous::lv::PartialArcLength(denseContour, arcLength);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    err = echonous::lv::PlaceSidePoints(denseContour, arcLength, splineIndices);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    // {
    //     char filepath[256];
    //     snprintf(filepath, sizeof(filepath), "/sdcard/segmentation/%s/%d.pgm", mView == CardiacView::A2C ? "a2c" : "a4c", mFrame);

    //     cv::Mat debugImg;
    //     frame.convertTo(debugImg, CV_8UC3);
    //     for (int idx : splineIndices)
    //     {
    //         cv::Point &p = denseContour[idx];
    //         cv::circle(debugImg, p, 5, cv::Scalar(255,0,0));
    //     }

    //     WriteImg(filepath, debugImg, true);
    // }

    ScanConverterParams params = cb->getParams().converterParams;
    float depth = params.startSampleMm + (params.sampleSpacingMm * (params.numSamples - 1));
    //printf("start depth = %0.2fmm\n", params.startSampleMm);
    CardiacSegmentation seg;
    err = echonous::lv::ConvertToPhysical(denseContour, splineIndices, -params.startRayRadian, depth, cv::Size(size.x, size.y), seg);
    if (IS_THOR_ERROR(err))
    {
        return err;
    }

    fprintf(out, "\"coords_x\": [%0.2f", seg.coords[0].x);
    for (unsigned i=1; i < seg.coords.size(); ++i)
    {
        fprintf(out, ", %0.2f", seg.coords[i].x);
    }
    fprintf(out, "],\n\"coords_y\": [%0.2f", seg.coords[0].y);
    for (unsigned i=1; i < seg.coords.size(); ++i)
    {
        fprintf(out, ", %0.2f", seg.coords[i].y);
    }
    fprintf(out, "],\n");

    segOut.coords = seg.coords;

    w->computeDisks(seg);
    fprintf(out, "\"disks_left_x\": [%0.2f", seg.disks[0].first.x);
    for (unsigned i=1; i < seg.disks.size(); ++i)
        fprintf(out, ", %0.2f", seg.disks[i].first.x);
    fprintf(out, "],\n\"disks_left_y\": [%0.2f", seg.disks[0].first.y);
    for (unsigned i=1; i < seg.disks.size(); ++i)
        fprintf(out, ", %0.2f", seg.disks[i].first.y);
    fprintf(out, "],\n\"disks_right_x\": [%0.2f", seg.disks[0].second.x);
    for (unsigned i=1; i < seg.disks.size(); ++i)
        fprintf(out, ", %0.2f", seg.disks[i].second.x);
    fprintf(out, "],\n\"disks_right_y\": [%0.2f", seg.disks[0].second.y);
    for (unsigned i=1; i < seg.disks.size(); ++i)
        fprintf(out, ", %0.2f", seg.disks[i].second.y);
    fprintf(out, "]");

    return THOR_OK;
}

// returns the last component of a path (POSIX style only)
const char *ftail(const char *path)
{
    const char *s = strrchr(path, '/');
    if (*s)
        return s+1;
    else
        return path;
}

int segmentClip(CardiacView view, const char *clip)
{
    ThorStatus status;

    auto &player = echonous::gUltrasoundManager.getCinePlayer();
    status = player.openRawFile(clip);
    if (IS_THOR_ERROR(status))
    {
        fprintf(stderr, "Error opening clip file %s.\n", clip);
        return 1;
    }

    printf("\"%s\":{\n", view == CardiacView::A4C ? "a4c":"a2c");

    auto &cinebuffer = echonous::gUltrasoundManager.getCineBuffer();
    printf("\"clip\": \"%s\",\n",ftail(clip));

    auto scp = cinebuffer.getParams().converterParams;
    float depth = (scp.numSamples-1) * scp.sampleSpacingMm + scp.startSampleMm;
    printf("\"imaging_depth\": %f,\n", depth);

    CardiacFrames frames;
    phase_detect(&cinebuffer, stdout, &frames.edFrame, &frames.esFrame);
    printf(",\n");

    CardiacSegmentation ed_seg;
    CardiacSegmentation es_seg;
    EFWorkflow *w = EFWorkflow::GetGlobalWorkflow();
    w->setAIFrames(view, frames);

    printf("\"ed\": {\n");
    segment(&cinebuffer, w, frames.edFrame, false, stdout, ed_seg, view);
    printf("},\n\"es\": {\n");
    segment(&cinebuffer, w, frames.esFrame, true, stdout, es_seg, view);
    printf("}},\n");

    w->setAISegmentation(view, frames.edFrame, ed_seg);
    w->setAISegmentation(view, frames.esFrame, es_seg);
    return 0;
}

int main(int argc, char **argv)
{
    const char *ups;
    const char *a4c;
    const char *a2c;
    ThorStatus status;

    if (!parse_args(argc, argv, &ups, &a4c, &a2c))
    {
        usage(argv[0]);
        return 0;
    }

    fprintf(stderr, "Running segmentation on a4c: %s, a2c: %s\n", a4c, a2c ? a2c:"null");

    status = echonous::gUltrasoundManager.openDirect("assets");
    if (IS_THOR_ERROR(status))
    {
        fprintf(stderr, "Error opening ups file %s.\n", ups);
        return 1;
    }

    printf("{\n");
    if (segmentClip(CardiacView::A4C, a4c))
        return 1;
    if (a2c)
        segmentClip(CardiacView::A2C, a2c);

    EFWorkflow *w = EFWorkflow::GetGlobalWorkflow();
    w->computeStats();

    auto stats = w->getStatistics();
    ((void)stats);

    
    printf(
        "\"hr_biplane\":  %f,\n"
        "\"ef_biplane\":  %f,\n"
        "\"edv_biplane\": %f,\n"
        "\"esv_biplane\": %f,\n"
        "\"sv_biplane\":  %f,\n"
        "\"co_biplane\":  %f,\n",
        stats.heartRate, stats.originalBiplaneStats.EF, stats.originalBiplaneStats.edVolume,
        stats.originalBiplaneStats.esVolume, stats.originalBiplaneStats.SV, stats.originalBiplaneStats.CO);

    printf(
        "\"hr_a4c\":  %f,\n"
        "\"ef_a4c\":  %f,\n"
        "\"edv_a4c\": %f,\n"
        "\"esv_a4c\": %f,\n"
        "\"sv_a4c\":  %f,\n"
        "\"co_a4c\":  %f,\n",
        stats.heartRate, stats.originalA4cStats.EF, stats.originalA4cStats.edVolume,
        stats.originalA4cStats.esVolume, stats.originalA4cStats.SV, stats.originalA4cStats.CO);

    printf(
        "\"hr_a2c\":  %f,\n"
        "\"ef_a2c\":  %f,\n"
        "\"edv_a2c\": %f,\n"
        "\"esv_a2c\": %f,\n"
        "\"sv_a2c\":  %f,\n"
        "\"co_a2c\":  %f\n}\n",
        stats.heartRate, stats.originalA2cStats.EF, stats.originalA2cStats.edVolume,
        stats.originalA2cStats.esVolume, stats.originalA2cStats.SV, stats.originalA2cStats.CO);

    return 0;
}