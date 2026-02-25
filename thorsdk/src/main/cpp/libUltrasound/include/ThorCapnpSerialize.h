//
// Copyright 2020 EchoNous Inc.
//

#pragma once
#include <CineBuffer.h>
#include <ThorCapnpTypes.capnp.h>

ThorStatus WriteThorCapnpFile(const char *path, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame);
ThorStatus ReadThorCapnpFile(const char *path, CineBuffer &cinebuffer, const char *dbPath);
ThorStatus UpdateCineParamsThorCapnpFile(const char *path, CineBuffer &cinebuffer);

//----------------------------------------------------------
// Writing/Building
// 

// Note that none of these classes return an error code.
// That is because the only failure case is an out of memory error, which
// is not really recoverable...
void BuildThorClip(echonous::capnp::ThorClip::Builder thorClipBuilder, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame);
void BuildCineParams(echonous::capnp::CineParams::Builder cineParamsBuilder, const CineBuffer::CineParams &cineparams);
void BuildScanConverterParams(echonous::capnp::ScanConverterParams::Builder scanConverterParamsBuilder, const ScanConverterParams &scanConverterParams);
void BuildCineData(echonous::capnp::CineData::Builder cineDataBuilder, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame, uint32_t dataType, uint32_t dataSize);
void BuildScrollingData(echonous::capnp::ScrollData::Builder scrollDataBuilder, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame, uint32_t dataType);
void BuildZoomPanParams(echonous::capnp::ZoomPanParams::Builder zoomPanParamsBuilder, const float* zoomPanParams);
void BuildTransitionFrames(echonous::capnp::ScrollData::Builder scrollDataBuilder, CineBuffer &cinebuffer);

//----------------------------------------------------------
// Reading
//

ThorStatus ReadThorClip(echonous::capnp::ThorClip::Reader thorClipReader, CineBuffer &cinebuffer, const char *dbPath);
void ReadCineParams(echonous::capnp::CineParams::Reader cineParamsReader, CineBuffer::CineParams &cineparams);
void ReadScanConverterParams(echonous::capnp::ScanConverterParams::Reader scanConverterParamsReader, ScanConverterParams &scanConverterParams);
void ReadCineData(echonous::capnp::CineData::Reader cineDataReader, CineBuffer &cinebuffer, uint32_t dataType);
void ReadScrollingData(echonous::capnp::ScrollData::Reader scrollDataReader, CineBuffer &cinebuffer, uint32_t dataType);
void ReadZoomPanParams(echonous::capnp::ZoomPanParams::Reader zoomPanParamsReader, float* zoomPanParams);
void ReadTransitionFrames(echonous::capnp::ScrollData::Reader scrollDataReader, CineBuffer &cinebuffer);