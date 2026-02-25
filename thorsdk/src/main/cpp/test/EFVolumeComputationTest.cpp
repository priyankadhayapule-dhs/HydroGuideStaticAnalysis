#include <EFWorkflow.h>
#include <json/json.h>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include "../libUltrasound/include/AI/EFWorkflow.h"

int main(int argc, char **argv)
{
    std::ifstream f(argv[1]);
    Json::Value root;
    f >> root;

    EFWorkflow *ef = EFWorkflow::GetGlobalWorkflow();

    const auto a4c_es = root["a4cEfClip"]["overlayInformation"]["segmentationInfo"]["segmentationInfoEsUser"]["controlPointsList"];
    const auto a4c_ed = root["a4cEfClip"]["overlayInformation"]["segmentationInfo"]["segmentationInfoEdUser"]["controlPointsList"];

    const auto a2c_es = root["a2cEfClip"]["overlayInformation"]["segmentationInfo"]["segmentationInfoEsUser"]["controlPointsList"];
    const auto a2c_ed = root["a2cEfClip"]["overlayInformation"]["segmentationInfo"]["segmentationInfoEdUser"]["controlPointsList"];

    CardiacSegmentation a4c_es_seg;
    CardiacSegmentation a4c_ed_seg;

    CardiacSegmentation a2c_es_seg;
    CardiacSegmentation a2c_ed_seg;

    for (const auto &pt : a4c_es) { a4c_es_seg.coords.push_back(vec2(pt["x"].asFloat(), pt["y"].asFloat())); }
    for (const auto &pt : a4c_ed) { a4c_ed_seg.coords.push_back(vec2(pt["x"].asFloat(), pt["y"].asFloat())); }
    for (const auto &pt : a2c_es) { a2c_es_seg.coords.push_back(vec2(pt["x"].asFloat(), pt["y"].asFloat())); }
    for (const auto &pt : a2c_ed) { a2c_ed_seg.coords.push_back(vec2(pt["x"].asFloat(), pt["y"].asFloat())); }

    a4c_es_seg.coords.pop_back();
    a4c_ed_seg.coords.pop_back();
    a2c_es_seg.coords.pop_back();
    a2c_ed_seg.coords.pop_back();

    ef->setAISegmentation(CardiacView::A4C, 0, a4c_es_seg);
    ef->setAISegmentation(CardiacView::A4C, 1, a4c_ed_seg);
    ef->setAIFrames(CardiacView::A4C, {0,1});

    if (!a2c_es_seg.coords.empty())
    {
        ef->setAISegmentation(CardiacView::A2C, 0, a2c_es_seg);
        ef->setAISegmentation(CardiacView::A2C, 1, a2c_ed_seg);
        ef->setAIFrames(CardiacView::A2C, {0,1});
    }

    ef->computeStats();
    const auto stats = ef->getStatistics();
    auto a4c_stats = Json::Value(Json::objectValue);
    a4c_stats["ESV"] = stats.originalA4cStats.esVolume;
    a4c_stats["EDV"] = stats.originalA4cStats.edVolume;
    a4c_stats["EF"]  = stats.originalA4cStats.EF;
    a4c_stats["SV"]  = stats.originalA4cStats.SV;

    auto a2c_stats = Json::Value(Json::objectValue);
    a2c_stats["ESV"] = stats.originalA2cStats.esVolume;
    a2c_stats["EDV"] = stats.originalA2cStats.edVolume;
    a2c_stats["EF"]  = stats.originalA2cStats.EF;
    a2c_stats["SV"]  = stats.originalA2cStats.SV;

    auto biplane_stats = Json::Value(Json::objectValue);
    biplane_stats["ESV"] = stats.originalBiplaneStats.esVolume;
    biplane_stats["EDV"] = stats.originalBiplaneStats.edVolume;
    biplane_stats["EF"]  = stats.originalBiplaneStats.EF;
    biplane_stats["SV"]  = stats.originalBiplaneStats.SV;

    auto json_stats = Json::Value(Json::objectValue);
    json_stats["a4c"] = a4c_stats;
    if (!a2c_es_seg.coords.empty())
    {
        json_stats["a2c"] = a2c_stats;
        json_stats["biplane"] = biplane_stats;
    }

    root["nativeEFData"] = json_stats;

    std::string(argv[1], strlen(argv[1])-5) + "-nativeResults.json";

    std::string outFile = std::string(argv[1], strlen(argv[1])-5) + "-nativeResults.json";
    std::ofstream fout(outFile);
    fout << root;

    return 0;
}