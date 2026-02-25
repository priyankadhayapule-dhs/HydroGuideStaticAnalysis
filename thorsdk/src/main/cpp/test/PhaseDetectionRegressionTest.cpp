#include <sys/stat.h>

#include <UltrasoundManager.h>
#include <getopt.h>
#include <cstdio>
#include <vector>
#include <AIManager.h>
#include <AIModel.h>
#include <PhaseDetection.h>
#include <ImageUtils.h>

namespace echonous
{
    extern UltrasoundManager gUltrasoundManager;
}

void usage(const char *prog)
{
    printf("usage: %s [-u|--ups UPS_FILE] CLIP\n", prog);
}

bool parse_args(int argc, char **argv, const char **ups, const char **clip)
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
        fprintf(stderr, "must provide path to clip file.\n");
        return false;
    }

    *clip = argv[optind];
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

ThorStatus phase_detect(CineBuffer *cb, FILE *out)
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

        // select the pair of ed/es frames which produces the median "ef"
        std::vector<std::pair<unsigned, float>> estimated_ef;
        for (unsigned i=0; i < ed_frames_mapped.size(); ++i)
        {
            float ef = (phase[ed_frames_mapped[i]-normalized_phase.begin()] - phase[es_frames_mapped[i]-normalized_phase.begin()]) / phase[ed_frames_mapped[i]-normalized_phase.begin()];
            estimated_ef.emplace_back(i, ef);
        }

        fprintf(out, "\"estimated_ef\": [%f", estimated_ef[0].second);
        for (unsigned i = 1; i < estimated_ef.size(); ++i)
        {
            fprintf(out, ", %f", estimated_ef[i].second);
        }
        fprintf(out, "],\n");

        std::sort(estimated_ef.begin(), estimated_ef.end(), [](const std::pair<unsigned, float> &p1, const std::pair<unsigned, float> &p2){
            return p1.second < p2.second;
        });

        unsigned median_idx = estimated_ef[estimated_ef.size()/2].first;

        edframe = ed_frames_mapped[median_idx] - normalized_phase.begin();
        esframe = es_frames_mapped[median_idx] - normalized_phase.begin();
    }

    fprintf(out, "\"ed_frame\": %d,\n\"es_frame\": %d\n", edframe, esframe);

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

int main(int argc, char **argv)
{
    const char *ups;
    const char *clip;
    ThorStatus status;

    if (!parse_args(argc, argv, &ups, &clip))
    {
        usage(argv[0]);
        return 0;
    }

    status = echonous::gUltrasoundManager.openDirect("assets");
    if (IS_THOR_ERROR(status))
    {
        fprintf(stderr, "Error opening ups file %s.\n", ups);
        return 1;
    }

    auto &player = echonous::gUltrasoundManager.getCinePlayer();
    status = player.openRawFile(clip);
    if (IS_THOR_ERROR(status))
    {
        fprintf(stderr, "Error opening clip file %s.\n", ups);
        return 1;
    }

    auto &cinebuffer = echonous::gUltrasoundManager.getCineBuffer();
    printf("{\"clip\": \"%s\",\n", ftail(clip));
    phase_detect(&cinebuffer, stdout);
    printf("}\n");

    return 0;
}