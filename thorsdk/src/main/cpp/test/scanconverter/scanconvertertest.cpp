#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>
#include "ScanConverterParams.h"
#include "ScanConverterReference.h"
#include "ThorFileProcessor.h"
#include "BModeRecordParams.h"
#include "BitfieldMacros.h"

#define TWO_PI 2*3.1415926f

#define SQLITE_TRY(expr, expected, result) do { \
    int rc; \
    if ((rc = (expr)) != (expected)) { \
        printf("sqlite3 error %d executing statement %s (in %s:%d)", rc, #expr, __func__, __LINE__); \
        return result; \
    }} while(0)

int readSCParamsDB(ScanConverterParams& sCParams, int imgCaseId)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    sqlite3_stmt* pStmt;
    int rc;
    int imgMode;
    int retVal = 0;
    int imagingCaseID = imgCaseId;

    rc = sqlite3_open("thor.db", &db);
    if (rc)
    {
        printf("Error in reading thor db file.");
        return(-1);
    }

    rc = sqlite3_prepare_v2(db,
                    "SELECT Mode FROM ImagingCases WHERE ID=?",
                    -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
      printf("prepare failed, return code is %d\n", rc);
      return(-1);
    }

    rc = sqlite3_bind_int(pStmt, 1, imagingCaseID);
    if (SQLITE_OK != rc)
    {
        printf("bind failed for getImagingMode, errorCode = %d", rc);
        return(-1);
    }

    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        printf("getImagingMode(): step failed, return code is %d\n", rc);
        return(-1);
    }
    imgMode = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);

    printf("imagingMode read from dB: %d\n", imgMode);

    // read scan conversion parameters
    // samples
    int samples = 0;

    rc = sqlite3_prepare_v2(db,
                            "SELECT NumSamplesB FROM Globals",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        printf("upsReader::getNumSamples(): prepare failed, return code is %d\n", rc);
        return(-1);
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        printf("upsReader::getNumSamples(): step failed, return code is %d\n", rc);
        return(-1);
    }
    samples = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);

    sCParams.numSamples = samples;

    // depth
    float depth = 400.0f;
    int depthId;

    rc = sqlite3_prepare_v2(db,
                            "SELECT Depth FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        printf("upsReader::getImagingDepth(): prepare failed, return code is %d\n", rc);
        return(-1);
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseID);
    if (SQLITE_OK != rc)
    {
        printf("bind failed for UpsReader::getImagingDepth, errorCode = %d", rc);
        return(-1);
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        printf("upsReader::getImagingDepth(): step failed, return code is %d\n", rc);
        return(-1);
    }
    depthId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        return(-1);
    }

    // depths
    rc = sqlite3_prepare_v2(db,
                            "SELECT Depth FROM Depths WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        printf("upsReader::getImagingDepth(): prepare failed, return code is %d\n", rc);
        return(-1);
    }

    rc = sqlite3_bind_int(pStmt, 1, depthId);
    if (SQLITE_OK != rc)
    {
        printf("bind failed for UpsReader::getImagingDepth, errorCode = %d", rc);
        return(-1);
    }

    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        printf("upsReader::getImagingDepth(): step failed, return code is %d\n", rc);
        return(-1);
    }
    depth = sqlite3_column_double(pStmt, 0);
    rc = sqlite3_finalize(pStmt);

    sCParams.sampleSpacingMm = depth / (sCParams.numSamples - 1);

    switch (imgMode)
    {
        case IMAGING_MODE_B:
            SQLITE_TRY(sqlite3_prepare_v2(db,
                                          "SELECT FovAzimuthStart, RxPitchB, NumRxBeamsB FROM Depths WHERE ID in "
                                          "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                          -1, &pStmt, 0), SQLITE_OK, false);
            break;
        case IMAGING_MODE_COLOR:
            SQLITE_TRY(sqlite3_prepare_v2(db,
                                          "SELECT FovAzimuthStartBC, RxPitchBC, NumRxBeamsBC FROM Depths WHERE ID in "
                                          "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                          -1, &pStmt, 0), SQLITE_OK, false);
            break;
        case IMAGING_MODE_M:
            SQLITE_TRY(sqlite3_prepare_v2(db,
                                          "SELECT FovAzimuthStartBM, RxPitchBM, NumRxBeamsBM FROM Depths WHERE ID in "
                                          "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                          -1, &pStmt, 0), SQLITE_OK, false);
            break;
        default:
            printf("%s imaging mode %d is invalid", __func__, imgMode);
            return (false);
            break;
    }

    SQLITE_TRY(sqlite3_bind_int(pStmt, 1, imagingCaseID), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(pStmt), SQLITE_ROW, false);

    sCParams.startRayRadian = sqlite3_column_double(pStmt, 0);
    sCParams.raySpacing = sqlite3_column_double(pStmt, 1);
    sCParams.numRays = sqlite3_column_int(pStmt, 2);

    printf("B-Mode Scan converter params:\n");
    printf("\t params.numSamplesMesh  = %d\n", sCParams.numSampleMesh);
    printf("\t params.numRayMesh      = %d\n", sCParams.numRayMesh);
    printf("\t params.numSamples      = %d\n", sCParams.numSamples);
    printf("\t params.numRays         = %d\n", sCParams.numRays);
    printf("\t params.startSampleMm   = %f\n", sCParams.startSampleMm);
    printf("\t params.sampleSpacingMm = %f\n", sCParams.sampleSpacingMm);
    printf("\t params.raySpacing      = %f\n", sCParams.raySpacing);
    printf("\t params.startRayRadian  = %f\n", sCParams.startRayRadian);

    // close dB
    sqlite3_close(db);
    return(0);
}


int main(int argc, char* argv[])
{
    time_t start,end;
    int screenW, screenH;

    // thor
    screenW = 1520;
    screenH = 1080;

    //ScanConverterParams sCParams = ScanConverterParams();
    ScanConverterParams sCParams;
    BModeRecordParams bModeParams;

    sCParams.numRayMesh = 50;
    sCParams.numSampleMesh = 100;
    sCParams.numSamples = 512;
    sCParams.numRays = 92;
    sCParams.startSampleMm = 0.0f;
    sCParams.startRayRadian = -(((float) 92) / 2.0f - 0.5f) * (TWO_PI / 360.0f);
    sCParams.sampleSpacingMm = 180.0f / 511;;
    sCParams.raySpacing = TWO_PI / 360.0f;

    //////////////////////////////////////////////////////////////////////
    // process inputfile
    // Thor Raw file
    ThorRawDataFile::Metadata       metadata;
    uint8_t*                        addressPtr = nullptr;
    ThorFileProcessor               fileProcessor;
    uint8_t                         fileVersion;
    ThorRawReader                   reader;

    //std::string srcPath = "07172019f34b48df-72f8-4df6-8b21-2f5e79765eeb.thor";  // v5
    std::string srcPath = "06182019cf1c299e-27ba-412a-a9d4-4d01984410a3.thor";  // v4
    int retVal, frameCount, frameInterval, imagingCaseID;
    int cineFrameHeaderSize, frameSizeWithHeader;
    uint8_t*    baseAddressPtr;
    uint8_t*    bDataPtr;

    //outFile
    FILE*       outFile;
    outFile = fopen ("sc_raw_out.bin", "wb");

    // open Reader
    retVal = reader.open(srcPath.c_str(), metadata);

    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Unable to open raw file");
    }

    addressPtr = reader.getDataPtr();
    if (nullptr == addressPtr)
    {
        ALOGE("getDataPtr() failed");
    }

    fileVersion = reader.getVersion();

    frameCount = metadata.frameCount;
    frameInterval = metadata.frameInterval;
    imagingCaseID = metadata.imageCaseId;

    printf("Thor file version: %d\n", fileVersion);
    printf("Number of frames: %d\n", frameCount);

    fileProcessor.processModeHeader(addressPtr, metadata.dataTypes, frameCount, fileVersion);
    cineFrameHeaderSize = fileProcessor.getFrameHeaderSize();
    frameSizeWithHeader = cineFrameHeaderSize + MAX_B_FRAME_SIZE;

    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        if (fileVersion > 4)
        {
            // version 5+: B header includes SC Params
            fileProcessor.getModeHeader(ThorRawDataFile::DataType::BMode, &bModeParams);
            sCParams = bModeParams.scanConverterParams;
        }
        else
        {
            // version 4 or earlier:
            // need to read directly from dB
            int rc = readSCParamsDB(sCParams, imagingCaseID);

            if (rc == -1)
            {
                printf("Error in reading SC Params from dB.\n");
                return(-1);
            }

            printf("successfully read ScParams from dB\n");
        }

        baseAddressPtr = (uint8_t*)fileProcessor.getDataPtr(ThorRawDataFile::DataType::BMode)
                          + cineFrameHeaderSize;
    }

    printf("numSamples: %d\n", sCParams.numSamples);
    printf("numRays: %d\n", sCParams.numRays);

    // allocate memory input, outputCL, outputRef;
    u_char* input_img =  new u_char[sCParams.numSamples * sCParams.numRays];
    u_char* output_Ref = new u_char[screenW * screenH];

    //u_char* output_CL = new u_char[screenW * screenH];
    //u_char* mask_in = new u_char[sCParams.numSamples * sCParams.numRays];
    //u_char* mask_out = new u_char[screenW * screenH];

    printf("Running a scan converter test - B-mode!\n");
    printf("output width: %d, height: %d\n", screenW, screenH);
    printf("\n");


    // scan converte refernece
    ScanConverterReference scanConverterReference = ScanConverterReference();
    scanConverterReference.setConversionParams(sCParams.numSamples,
                                               sCParams.numRays,
                                               screenW,    // screenWidth
                                               screenH,    // screenHeight
                                               0.0f,    // start img depth
                                               (sCParams.sampleSpacingMm * (sCParams.numSamples - 1) + sCParams.startSampleMm),  //end img depth
                                               sCParams.startSampleMm,
                                               sCParams.sampleSpacingMm,
                                               sCParams.startRayRadian,
                                               sCParams.raySpacing);

    // zoom pan params
    scanConverterReference.setZoomPanParams(1.0f, 0.0f, 0.0f);
    scanConverterReference.setFlipXFlipY(1.0f, 1.0f);

    // running refernece scan converter

    time (&start);
    for (int i = 0; i < frameCount; i++)
    {
      // b-mode data pointer
      bDataPtr = baseAddressPtr + frameSizeWithHeader * i;

      // run scanconversion
      scanConverterReference.runConversionBiCubic4x4(bDataPtr, output_Ref);

      // output to a file
      fwrite (output_Ref , sizeof(char), screenW*screenH, outFile);
    }

    time (&end);
    double dif = difftime (end, start);
    printf ("Elasped time is %.2lf seconds.\n", dif );

    // clean up
    reader.close();
    fclose (outFile);

    delete[] input_img;
    delete[] output_Ref;
    //delete[] mask_in;
    //delete[] mask_out;

    return(0);
}
