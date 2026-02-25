
//
// Copyright (C) 2017 EchoNous, Inc.
//
//
#define LOG_TAG  "DauUpsReader"

#include <stdint.h>
#include <ThorUtils.h>
#include "ThorConstants.h"
#include "UpsReader.h"
#include "TbfRegs.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <Crc32.h>

// Useful macros for error handling
#define SQLITE_TRY(expr, expected, result) do { \
    int rc; \
    if ((rc = (expr)) != (expected)) { \
        LOGE("Sqlite3 error %d executing statement %s (in %s:%d)", rc, #expr, __func__, __LINE__); \
        ++mErrorCount; \
        return result; \
    }} while(0)

// This is to mask off Dev probe bit and other misc bits
#define PROBE_TYPE_MASK 0xFFFFFFF

const char* const UpsReader::DEFAULT_UPS_FILE = "thor.db";

// This array is used to map probe type to name of UPS database
const UpsReader::ProbeDbInfo UpsReader::ProbeInfoArray[] =
{
    {PROBE_TYPE_TORSO3, "thor.db"},
    {PROBE_TYPE_TORSO1, "thor.db"},
    {PROBE_TYPE_LINEAR, "l38.db"},
    {0x0, ""}   // This entry marks the end of the array
};

//-----------------------------------------------------------------------------
UpsReader::UpsReader() :
    mErrorCount(0)
{
    LOGD("*** UpsReader - constructor");
}

//-----------------------------------------------------------------------------
UpsReader::~UpsReader()
{
    LOGD("*** UpsReader - destructor");
    close();
}

//-----------------------------------------------------------------------------
bool UpsReader::open(const char* dbFilename)
{
    LOGI("Overriding default UPS location: %s", dbFilename);
    return(openInternal(dbFilename));
}

//-----------------------------------------------------------------------------
ThorStatus UpsReader::open(const char* dbPath, uint32_t probeType)
{
    ThorStatus      retVal = THOR_ERROR;
    std::string     upsPath;
    const char*     upsDbPtr = getDbName(probeType);

    if (nullptr == upsDbPtr)
    {
        ALOGE("Unable to map the probe type (0x%X) to a UPS", probeType);
        retVal = TER_IE_UNSUPPORTED_PROBE;
        goto err_ret;
    }

    upsPath = dbPath;
    upsPath += "/";
    upsPath += upsDbPtr;

    if (!openInternal(upsPath.c_str()))
    {
        retVal = TER_IE_UPS_OPEN;
    }
    else
    {
        retVal = THOR_OK;
    }

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
const char* UpsReader::getDbName(uint32_t probeType)
{
    uint32_t    actualProbeType = probeType & PROBE_TYPE_MASK;

    for (int index = 0; 0x0 != ProbeInfoArray[index].probeType; index++)
    {
        if (actualProbeType == ProbeInfoArray[index].probeType)
        {
            ALOGD("Found UPS name for probeType 0x%X", probeType);
            return(ProbeInfoArray[index].dbName);
        }
    }
    ALOGE("Unable to locate UPS name for probeType 0x%X", probeType);
    return (nullptr);
}

//-----------------------------------------------------------------------------
bool UpsReader::checkIntegrity()
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "PRAGMA integrity_check",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    const unsigned char *result = sqlite3_column_text( stmt, 0 );
    if ( (result != nullptr) && strcmp( ( const char * )result, (const char *)"ok" ) == 0 )
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

//-----------------------------------------------------------------------------
bool UpsReader::openInternal(const char* dbFilename)
{
    std::unique_lock<std::recursive_mutex> lock(mLock);
    if (mDb_ptr)
    {
        if (0 == mDbFilename.compare(dbFilename))
        {
            // Being asking to open the same database
            ALOGD("openInternal: Being asked to open same database. Ignoring...");
            return true;
        }
        else
        {
            // A database is already open but being asked to open a different
            // one.  Close the old one first.
            ALOGD("openInternal: Being asked to open different database. Closing old one first.");
            close();
        }
    }

    SQLITE_TRY(sqlite3_open_v2(dbFilename, &mDb_ptr, SQLITE_OPEN_READONLY, NULL), SQLITE_OK, false);

    mDbFilename = dbFilename;
    LOGI("UpsReader::openInternal(%s) - SUCCESS!", dbFilename);
    // print schema

    return true;
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getBaseConfigLength(uint32_t imagingCaseId)
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                            "SELECT TxConfig.BaseConfigLength FROM TxConfig, ImagingCases WHERE"
                            "  ImagingCases.ID=? AND TxConfig.ID = ImagingCases.TxConfig",
                            -1, &stmt, 0), SQLITE_OK, 0);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    uint32_t length = sqlite3_column_int(stmt, 0);

    if (length >= (1 << HVSPI_W_C_LEN))
    {
        LOGE("hvsp_w_c (%d) is greater than 13 bits permit", length);
        mErrorCount++;
        return 0;
    }

    return (length);
}

//-----------------------------------------------------------------------------
bool UpsReader::getHvpsValues( uint32_t imagingCaseId, float& hvp, float& hvn)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                            "SELECT HVPB, HVNB FROM ImagingCases WHERE ID=?",
                            -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    hvp = sqlite3_column_double(stmt, 0);
    hvn  = sqlite3_column_double(stmt, 1);
    LOGI( "HVPB = %f, HVNB = %f", hvp, hvn);

    if ((hvp > MAX_HVPS_VOLTS) ||
        (hvp < MIN_HVPS_VOLTS) ||
        (hvn > MAX_HVPS_VOLTS) ||
        (hvn < MIN_HVPS_VOLTS))
    {
        mErrorCount++;
        LOGE("HVPS values read from UPS are out of bounds [4,40], HVPB = %f, HVNB = %f",
             hvp, hvn);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
bool UpsReader::getPwHvpsValues( uint32_t imagingCaseId, float& hvp, float& hvn)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT HVPPW, HVNPW FROM ImagingCases WHERE ID=?",
                                  -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    hvp = sqlite3_column_double(stmt, 0);
    hvn  = sqlite3_column_double(stmt, 1);
    LOGI( "HVPPW = %f, HVNPW = %f", hvp, hvn);

    if ((hvp > MAX_HVPS_VOLTS) ||
        (hvp < MIN_HVPS_VOLTS) ||
        (hvn > MAX_HVPS_VOLTS) ||
        (hvn < MIN_HVPS_VOLTS))
    {
        mErrorCount++;
        LOGE("HVPS values read from UPS are out of bounds [3,35], HVPPW = %f, HVNPW = %f",
             hvp, hvn);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
bool UpsReader::getCwHvpsValues( uint32_t imagingCaseId, float& hvp, float& hvn)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT HVPCW, HVNCW FROM ImagingCases WHERE ID=?",
                                  -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    hvp = sqlite3_column_double(stmt, 0);
    hvn  = sqlite3_column_double(stmt, 1);
    LOGI( "HVPCW = %f, HVNCW = %f", hvp, hvn);

    if ((hvp > MAX_HVPS_VOLTS) ||
        (hvp < MIN_HVPS_VOLTS) ||
        (hvn > MAX_HVPS_VOLTS) ||
        (hvn < MIN_HVPS_VOLTS))
    {
        mErrorCount++;
        LOGE("HVPS values read from UPS are out of bounds [4,40], HVPCW = %f, HVNCW = %f",
             hvp, hvn);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void UpsReader::close()
{
    std::unique_lock<std::recursive_mutex> lock(mLock);

    mDb_ptr.close();
    mDbFilename.clear();
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getSequenceBlob(uint32_t imagingCaseId, uint32_t* pBuf, uint32_t* numRegsPtr)
{
    int ret = 0;
    sqlite3_stmt* pStmt;
    int rc;
    int blobSize = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT SequenceBlobV2, NumRegisters FROM Sequence WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getSequenceBlob(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getSequenceBlob, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getSequenceBlob(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    blobSize = sqlite3_column_bytes(pStmt, 0);
    if (0 == blobSize)
    {
        mErrorCount++;
        goto err_ret;
    }
    memcpy(pBuf, sqlite3_column_blob(pStmt, 0), blobSize);
    *numRegsPtr = sqlite3_column_int(pStmt, 1);

    LOGD("number of registers in the sequence blob = %d", *numRegsPtr);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    ret = blobSize;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getPowerMgmtBlob(uint32_t imagingCaseId, uint32_t* pBuf)
{
    int ret = 0;
    sqlite3_stmt* pStmt;
    int rc;
    int blobSize = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT PMBlob FROM Sequence WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getPowerMgmtBlob(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getPowerMgmtBlob, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getPowerMgmtBlob(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    blobSize = sqlite3_column_bytes(pStmt, 0);
    if (0 == blobSize)
    {
        mErrorCount++;
        goto err_ret;
    }
    memcpy(pBuf, sqlite3_column_blob(pStmt, 0), blobSize);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    ret = blobSize;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getPowerDownBlob(uint32_t imagingCaseId, uint32_t* pBuf)
{
    int ret = 0;
    sqlite3_stmt* pStmt;
    int rc;
    int blobSize = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT PDBlob FROM Sequence WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getPowerDownBlob(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getPowerDownBlob, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getPowerDownBlob(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    blobSize = sqlite3_column_bytes(pStmt, 0);
    if (0 == blobSize)
    {
        mErrorCount++;
        goto err_ret;
    }
    memcpy(pBuf, sqlite3_column_blob(pStmt, 0), blobSize);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    ret = blobSize;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getImagingType(uint32_t imagingCaseId)
{
    Sqlite3Stmt stmt;
    uint32_t imagingType = 0;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT ImagingType from ImagingCases WHERE ID = ? ",
                                  -1,
                                  &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    imagingType = sqlite3_column_int(stmt, 0);
    return (imagingType);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getImagingOrgan(uint32_t imagingCaseId)
{
    Sqlite3Stmt stmt;
    uint32_t imagingOrgan = 0;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT Organ from ImagingCases WHERE ID = ? ",
                                  -1,
                                  &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    imagingOrgan = sqlite3_column_int(stmt, 0);
    return (imagingOrgan);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getImagingGlobalID(uint32_t imagingCaseId)
{
    Sqlite3Stmt stmt;
    uint32_t globalId = 0;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT GlobalID from Organs where ID in (SELECT Organ from ImagingCases where ID = ?) ",
                                  -1,
                                  &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    globalId = sqlite3_column_int(stmt, 0);
    return (globalId);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getOrganGlobalId(uint32_t organId)
{
    Sqlite3Stmt stmt;
    uint32_t organGlobalId = 0;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT GlobalID from Organs WHERE ID = ? ",
                                  -1,
                                  &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    organGlobalId = sqlite3_column_int(stmt, 0);
    return (organGlobalId);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getNumImagingCases()
{
    sqlite3_stmt* pStmt;
    int numImagingCases = 0;
    int numSequenceBlobs = 0;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr, "SELECT Count(*) FROM ImagingCases", -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getNumImagingCases(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getNumImagingCases(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    numImagingCases = sqlite3_column_int (pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_prepare_v2(mDb_ptr, "SELECT Count(*) FROM Sequence", -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getNumImagingCases(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getNumImagingCases(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    numSequenceBlobs = sqlite3_column_int (pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    if (numImagingCases != numSequenceBlobs)
    {
        mErrorCount++;
        LOGE("Number of sequence blobs is %d and it does not match the number of imaging cases\n", numSequenceBlobs);
    }

err_ret:
ok_ret:
    return (numImagingCases);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getNumTxBeamsB(uint32_t imagingCaseId, uint32_t imagingMode)
{
    Sqlite3Stmt stmt;
    uint32_t numTxBeams = 0;
    uint32_t organId = this->getImagingOrgan(imagingCaseId);
    uint32_t organGlobalId = this->getOrganGlobalId(organId);

    switch (imagingMode)
    {
    case IMAGING_MODE_B:
        if (organGlobalId == BLADDER_GLOBAL_ID)
        {
            SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                          "SELECT NumTxBeamsBbladder from Depths WHERE ID in "
                                          "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                          -1,
                                          &stmt, 0), SQLITE_OK, 0);
        }
        else
        {
            SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                          "SELECT NumTxBeamsB from Depths WHERE ID in "
                                          "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                          -1,
                                          &stmt, 0), SQLITE_OK, 0);
        }
        break;

    case IMAGING_MODE_COLOR:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT NumTxBeamsBC from Depths WHERE ID in "
                                      "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;

    case IMAGING_MODE_M:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT NumTxBeamsBM from Depths WHERE ID in "
                                      "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;

    default:
        LOGE("ERROR: %s imaging mode %d is invalid", __func__, imagingMode);
        mErrorCount++;
        return(0);
        break;
    }
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    numTxBeams = sqlite3_column_int(stmt, 0);
    return (numTxBeams);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getNumTxBeamsC(uint32_t imagingCaseId)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    int rc;
    uint32_t numTxBeamsC = 0;
    uint32_t numTotalTxBeamsC = 0;
    uint32_t organId;
    uint32_t wallFilterType;
    uint32_t wallFilterLow;
    uint32_t wallFilterMed;
    uint32_t wallFilterHigh;
    uint32_t wallFilterIndex;
    uint32_t ensembleSizeId;
    uint32_t ensembleSize;
    uint32_t dummyVector;
    uint32_t firstDummyVector;
    uint32_t numInterleaveGroup;
    uint32_t kIF;
    uint32_t TEMPNumTotalTxBeamsC;

    //------------------ fetch Organ from ImagingCases -------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Organ, WallFilterLowID, WallFilterMediumID, WallFilterHighID FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getNumTxBeamsC\n");
        mErrorCount++;
        goto err_ret;
    }
    organId         = sqlite3_column_int(pStmt, 0);
    wallFilterLow   = sqlite3_column_int(pStmt, 1);
    wallFilterMed   = sqlite3_column_int(pStmt, 2);
    wallFilterHigh  = sqlite3_column_int(pStmt, 3);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    LOGI("organId = %d, wallFilterLow = %d, wallFilterMed = %d, wallFilterHigh = %d",
         organId, wallFilterLow, wallFilterMed, wallFilterHigh);

    //--------- Get DefaultWallFilter type from Organs using organId -----------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT DefaultWallFilterIndex FROM organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getCFWallFilter(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getCFWallFilter(), errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getCFWallFilter(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    wallFilterType = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    // TODO: cleanup enums or #defines for wall fitler types
    switch (wallFilterType)
    {
    case 0:
        wallFilterIndex = wallFilterLow;
        break;

    case 1:
        wallFilterIndex = wallFilterMed;
        break;

    case 2:
        wallFilterIndex = wallFilterHigh;
        break;

    default:
        LOGE("UpsReader::getCFWallFilter(): wallFilterType (%d) is invalid",
             wallFilterType);
        mErrorCount++;
        goto err_ret;
        break;
    }

    //------------------ fetch EnsembleSizeIndex from WallFilters --------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT EnsembleSizeIndex from WallFilters WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, wallFilterIndex);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getNumTxBeamsC\n");
        mErrorCount++;
        goto err_ret;
    }
    ensembleSizeId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    LOGI("ensemble size index for imaging case %d is %d", imagingCaseId, ensembleSizeId);

    //------------------ fetch numTxBeamsC from Organs --------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT NumTxBeamsC, TEMPTxInterleaveFactor, TEMPNumTotalTxBeamsC from Organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getNumTxBeamsC\n");
        mErrorCount++;
        goto err_ret;
    }
    numTxBeamsC = sqlite3_column_int(pStmt, 0);
    kIF         = sqlite3_column_int(pStmt, 1);
    TEMPNumTotalTxBeamsC = sqlite3_column_int(pStmt, 2);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    LOGI("NumTxBeamsC for imaging case %d is %d", imagingCaseId, numTxBeamsC);
    LOGI("TEMPTxInterleaveFactor for organId %d is %d TEMPNumTotalTxBeamsC = %d", organId, kIF, TEMPNumTotalTxBeamsC);

    //------------------ fetch EnsembleSize from EnsembleSize-------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT EnsembleSize from EnsembleSize WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, ensembleSizeId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getNumTxBeamsC\n");
        mErrorCount++;
        goto err_ret;
    }
    ensembleSize = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    LOGI("ensemble size for imaging case %d is %d", imagingCaseId, ensembleSize);

    //------------------ fetch DummyVector from Globals ------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT DummyVector, firstDummyVector from Globals",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getNumTxBeamsC, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getNumTxBeamsC\n");
        mErrorCount++;
        goto err_ret;
    }
    dummyVector      = sqlite3_column_int(pStmt, 0);
    firstDummyVector = sqlite3_column_int(pStmt, 1);
    LOGI("DummyVector = %d, firstDummyVector = %d", dummyVector, firstDummyVector);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    ret = true;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // This is a temporary code block until until mechanism for handling dynamic color ROI
    // is put in place.
    if (0)
    {
        numInterleaveGroup = (uint32_t)(numTxBeamsC/kIF);
        numTotalTxBeamsC = (numInterleaveGroup-1)*(ensembleSize*kIF + dummyVector) + (ensembleSize*kIF + firstDummyVector);
        numTotalTxBeamsC += kIF*ensembleSize; // additional nodes for flushing the pipeline
    }
    else
    {
        numTotalTxBeamsC = TEMPNumTotalTxBeamsC;
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    goto ok_ret;

err_ret:
ok_ret:
    LOGI("getNumTxBeamsC(imagingCaseId = %d) = %d, totalCBeams = %d\n", imagingCaseId, numTxBeamsC, numTotalTxBeamsC);
    return(numTotalTxBeamsC);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getEnsembleSize(uint32_t imagingCaseId, uint32_t wallFilterType)
{
    Sqlite3Stmt stmt;
    uint32_t wallFilterId;

    switch (wallFilterType)
    {
    case 0:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT EnsembleSize FROM EnsembleSize WHERE ID in "
                                      "(SELECT EnsembleSizeIndex FROM WallFilters WHERE ID in "
                                      "(SELECT WallFilterLowID FROM ImagingCases WHERE ID = ?))",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;
    case 1:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT EnsembleSize FROM EnsembleSize WHERE ID in "
                                      "(SELECT EnsembleSizeIndex FROM WallFilters WHERE ID in "
                                      "(SELECT WallFilterMediumID FROM ImagingCases WHERE ID = ?))",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;
    case 2:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT EnsembleSize FROM EnsembleSize WHERE ID in "
                                      "(SELECT EnsembleSizeIndex FROM WallFilters WHERE ID in "
                                      "(SELECT WallFilterHighID FROM ImagingCases WHERE ID = ?))",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;
    default:
        LOGE("%s wall filter type = %d is invalid", wallFilterType);
        return (0);
        break;
    }
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    uint32_t ensembleSize = sqlite3_column_int(stmt, 0);
    return (ensembleSize);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getEnsembleSize(uint32_t imagingCaseId)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    int rc;
    uint32_t organId;
    uint32_t wallFilterType;
    uint32_t wallFilterLow;
    uint32_t wallFilterMed;
    uint32_t wallFilterHigh;
    uint32_t wallFilterIndex;
    uint32_t ensembleSizeId;
    uint32_t ensembleSize = 0;

    //------------------ fetch Organ from ImagingCases -------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Organ, WallFilterLowID, WallFilterMediumID, WallFilterHighID FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getEnsembleSize, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getEnsembleSize, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getEnsembleSize\n");
        mErrorCount++;
        goto err_ret;
    }
    organId         = sqlite3_column_int(pStmt, 0);
    wallFilterLow   = sqlite3_column_int(pStmt, 1);
    wallFilterMed   = sqlite3_column_int(pStmt, 2);
    wallFilterHigh  = sqlite3_column_int(pStmt, 3);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
            goto err_ret;
    }
    LOGI("organId = %d, wallFilterLow = %d, wallFilterMed = %d, wallFilterHigh = %d",
         organId, wallFilterLow, wallFilterMed, wallFilterHigh);


    //--------- Get DefaultWallFilter type from Organs using organId -----------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT DefaultWallFilterIndex FROM organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getEnsembleSize(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getEnsebleSize(), errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getEnsembleSize(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    wallFilterType = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    // TODO: cleanup enums or #defines for wall fitler types
    switch (wallFilterType)
    {
    case 0:
        wallFilterIndex = wallFilterLow;
        break;

    case 1:
        wallFilterIndex = wallFilterMed;
        break;

    case 2:
        wallFilterIndex = wallFilterHigh;
        break;

    default:
        LOGE("UpsReader::getEnsebleSize(): wallFilterType (%d) is invalid",
             wallFilterType);
        mErrorCount++;
        goto err_ret;
        break;
    }

    //------------------ fetch EnsembleSizeIndex from WallFilters --------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT EnsembleSizeIndex from WallFilters WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getEnsembleSize, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, wallFilterIndex);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getEnsembleSize, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getEnsembleSize\n");
        mErrorCount++;
        goto err_ret;
    }
    ensembleSizeId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    LOGI("ensemble size index for imaging case %d is %d", imagingCaseId, ensembleSizeId);


    //------------------ fetch EnsembleSize from EnsembleSize-------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT EnsembleSize from EnsembleSize WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getEnsembleSize, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, ensembleSizeId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getEnsembleSize, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getEnsembleSize\n");
        mErrorCount++;
        goto err_ret;
    }
    ensembleSize = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    LOGI("ensemble size for imaging case %d is %d", imagingCaseId, ensembleSize);
    goto ok_ret;

err_ret:
ok_ret:
    return(ensembleSize);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getNumParallel(uint32_t imagingCaseId)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    int rc;
    uint32_t numParallel = 4;
    uint32_t organId;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Organ from ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getNumParallel, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getNumParallel, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getNumParallel\n");
        mErrorCount++;
        goto err_ret;
    }
    organId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT NumParallel from Organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getNumParallel, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getNumParallel, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getNumParallel\n");
        mErrorCount++;
        goto err_ret;
    }
    numParallel = sqlite3_column_int(pStmt, 0);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    goto ok_ret;

err_ret:
ok_ret:
    LOGI("getNumParallel(imagingCaseId = %d) = %d\n", imagingCaseId, numParallel);
    return(numParallel);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getElementLocXLut(uint32_t* lutPtr)
{
    return( getGlobalLut("SELECT ElementPositionX FROM TransducerInfo",
                   TBF_LUT_ELOC_X_SIZE, lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getElementLocYLut(uint32_t* lutPtr)
{
    return( getGlobalLut("SELECT ElementPositionY FROM TransducerInfo",
                   TBF_LUT_ELOC_Y_SIZE, lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getApodPosLut(uint32_t* lutPtr)
{
    return( getGlobalLut("SELECT ApodizationPosition FROM ApodizationPositionLUT WHERE ID=0",
                   TBF_LUT_APODEL_SIZE, lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getGainDbToLinearLut(uint32_t* lutPtr)
{
    return( getGlobalLut("SELECT GainLinear FROM GainDbToLinearLUT WHERE ID=0",
                   TBF_LUT_MSDBL_SIZE, lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFractionalDelayLut(uint32_t* lutPtr)
{

    // TODO: *2 below is a band-aid until UPS is fixed to match TBF IDD
    return( getGlobalLut("SELECT Coeffs FROM FractionalDelayLUT WHERE ID=0",
                   TBF_LUT_MSRF_SIZE*2, lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTopParamsBlob(uint32_t* lutPtr)
{

    return( getGlobalLut("SELECT TopParamBlob FROM Globals",
                         TBF_NUM_TOP_REGS, lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTopStopParamsBlob(uint32_t* lutPtr)
{

    return( getGlobalLut("SELECT TopStopParamBlob FROM Globals",
                         TBF_NUM_TOP_REGS, lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getApodInterpRateLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT ApodizationInterpolationRateID from ImagingCases WHERE ID=?",
                              "SELECT Code from ApodizationInterpolationRateLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDIS_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getApodizationLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT ApodizationWeightID from ImagingCases WHERE ID=?",
                              "SELECT ApodizationWeights from ApodizationLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSAPW_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getApodizationScaleLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT ApodizationScaleID from ImagingCases WHERE ID=?",
                              "SELECT ScaleBeam0 from ApodizationScaleLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSB0APS_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getApodizationScaleCRoiMaxLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT ApodizationScaleID from ImagingCases WHERE ID=?",
                              "SELECT ScaleCRoiMax from ApodizationScaleLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSB0APS_SIZE,
                              lutPtr) );
}


//-----------------------------------------------------------------------------
uint32_t UpsReader::getBandPassFilter1I( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT BPF1 from ImagingCases WHERE ID=?",
                              "SELECT CoeffsI from BandPassFilters WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSBPFI_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getBandPassFilter1Q( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT BPF1 from ImagingCases WHERE ID=?",
                              "SELECT CoeffsQ from BandPassFilters WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSBPFQ_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCoherentCompoundGainLut   (uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CoherentCompoundGainID from ImagingCases WHERE ID=?",
                              "SELECT Coeffs from CoherentCompoundGainLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSCBC_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCompressionLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT CompressionID from ImagingCases WHERE ID=?",
                              "SELECT CompressionValues from Compression WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSCOMP_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFilterBlendingLut         (uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT FilterBlendingID from ImagingCases WHERE ID=?",
                              "SELECT Coeffs from FilterBlendingLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSFC_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB0XLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam0X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B0FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB0YLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam0Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B0FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB1XLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam1X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B1FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB1YLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam1Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B1FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB2XLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam2X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B2FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB2YLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam2Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B2FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB3XLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam3X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B3FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalCompensationB3YLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam3Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B3FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getFocalInterpolationRateLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT FocalInterpolationRateID from ImagingCases WHERE ID=?",
                              "SELECT Code from FocalInterpolationRateLut WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDIS_SIZE,
                              lutPtr) );
}


//-----------------------------------------------------------------------------
uint32_t UpsReader::getGainDgc01Lut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT GainDgcID from ImagingCases WHERE ID=?",
                              "SELECT GainValues01 from GainDgc WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDBG01_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getGainDgc23Lut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT GainDgcID from ImagingCases WHERE ID=?",
                              "SELECT GainValues23 from GainDgc WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDBG23_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTxDelayLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT TxDelayID from ImagingCases WHERE ID=?",
                              "SELECT Delay from TxDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTxPropagationDelayB0Lut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT TxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam0 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B0FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTxPropagationDelayB1Lut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT TxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam1 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B1FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTxPropagationDelayB2Lut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT TxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam2 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B2FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTxPropagationDelayB3Lut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT TxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam3 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B2FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTxWaveformBLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT TxWaveformB from ImagingCases WHERE ID=?",
                              "SELECT Waveform from TxWaveforms WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSTXW_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
// TODO: the name of the table may undergo change.
uint32_t UpsReader::getAfeInitLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT GainAtgcID from ImagingCases WHERE ID=?",
                              "SELECT GainValues from GainAtgc WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSAFEINIT_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getTxConfigLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT TxConfig from ImagingCases WHERE ID=?",
                              "SELECT TxConfig from TxConfig WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSSTTXC_SIZE,
                              lutPtr) );

}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFApodInterpRateLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT CFApodizationInterpolationRateID from ImagingCases WHERE ID=?",
                              "SELECT CODE from ApodizationInterpolationRateLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDIS_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFApodWeightLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFApodizationWeightID from ImagingCases WHERE ID=?",
                              "SELECT ApodizationWeights from ApodizationLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSAPW_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFApodScaleLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFApodizationScaleID from ImagingCases WHERE ID=?",
                              "SELECT ScaleBeam0 from ApodizationScaleLUT WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSB0APS_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB0XLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam0X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B0FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB0YLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam0Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B0FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB1XLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam1X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B1FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB1YLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam1Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B1FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB2XLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam2X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B2FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB2YLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam2Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B2FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB3XLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam3X from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B3FPX_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalCompensationB3YLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalCompensationID from ImagingCases WHERE ID=?",
                              "SELECT Beam3Y from FocalCompensation WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B3FPY_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFFocalInterpRateLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFFocalInterpolationRateID from ImagingCases WHERE ID=?",
                              "SELECT Code from FocalInterpolationRateLut WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDIS_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFGainDgc01Lut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFGainDgcID from ImagingCases WHERE ID=?",
                              "SELECT GainValues01 from GainDgc WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDBG01_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFGainDgc23Lut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFGainDgcID from ImagingCases WHERE ID=?",
                              "SELECT GainValues23 from GainDgc WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSDBG23_SIZE,
                              lutPtr) );
}


//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFTxDelayLut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFTxDelayID from ImagingCases WHERE ID=?",
                              "SELECT Delay from TxDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFTxPropagationDelayB0Lut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFTxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam0 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B0FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFTxPropagationDelayB1Lut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFTxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam1 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B1FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFTxPropagationDelayB2Lut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFTxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam2 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B2FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFTxPropagationDelayB3Lut(uint32_t imagingCaseId, uint32_t* lutPtr)
{
    return( getImagingCaseLut("SELECT CFTxPropagationDelayID from ImagingCases WHERE ID=?",
                              "SELECT PropagationBeam3 from TxPropagationDelay WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_B3FTXD_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
// TODO: the name of the table may undergo change.
uint32_t UpsReader::getCFAfeInitLut( uint32_t imagingCaseId, uint32_t* lutPtr )
{
    return( getImagingCaseLut("SELECT CFGainAtgcID from ImagingCases WHERE ID=?",
                              "SELECT GainValues from GainAtgc WHERE ID=?",
                              imagingCaseId,
                              TBF_LUT_MSAFEINIT_SIZE,
                              lutPtr) );
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFWallFilter( uint32_t imagingCaseId, uint32_t wallFilterType, uint32_t* lutPtr )
{
    Sqlite3Stmt stmt;

    switch (wallFilterType)
    {
    case 0:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT Coeffs FROM WallFilters WHERE ID in "
                                  "(SELECT WallFilterLowID FROM ImagingCases WHERE ID = ?)",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;
    case 1:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT Coeffs FROM WallFilters WHERE ID in "
                                  "(SELECT WallFilterMediumID FROM ImagingCases WHERE ID = ?)",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;
    case 2:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT Coeffs FROM WallFilters WHERE ID in "
                                  "(SELECT WallFilterHighID FROM ImagingCases WHERE ID = ?)",
                                      -1,
                                      &stmt, 0), SQLITE_OK, 0);
        break;
    default:
        LOGE("%s wall filter type = %d is invalid", wallFilterType);
        return (0);
        break;
    }
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    int32_t blobSize = sqlite3_column_bytes(stmt, 0);
    if ( (blobSize <= 0) || (blobSize > 1024) )
    {
        LOGE("UpsReader::getCFWallFilter(), invalid blob size : %d", blobSize);
        mErrorCount++;
        return (0);
    }
    memcpy(lutPtr, sqlite3_column_blob(stmt, 0), blobSize);

    return ((uint32_t)blobSize/sizeof(float));
}


//-----------------------------------------------------------------------------
uint32_t UpsReader::getCFWallFilter( uint32_t imagingCaseId, uint32_t* lutPtr)
{
    uint32_t organId;
    uint32_t wallFilterType;
    uint32_t wallFilterLow;
    uint32_t wallFilterMed;
    uint32_t wallFilterHigh;
    uint32_t wallFilterIndex;
    uint32_t blobSize;
    uint32_t numCoeffs = 0;
    sqlite3_stmt* pStmt;
    int rc;

    //----------------- Get Organ from ImagingCases ----------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Organ, WallFilterLowID, WallFilterMediumID, WallFilterHighID FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getCFWallFilter(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getCFWallFilter(), errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getCFWallFilter(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    organId         = sqlite3_column_int(pStmt, 0);
    wallFilterLow   = sqlite3_column_int(pStmt, 1);
    wallFilterMed   = sqlite3_column_int(pStmt, 2);
    wallFilterHigh  = sqlite3_column_int(pStmt, 3);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    //--------- Get DefaultWallFilterIndex from Organs using organId -----------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT DefaultWallFilterIndex FROM organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getCFWallFilter(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getCFWallFilter(), errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getCFWallFilter(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    wallFilterType = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    // TODO: cleanup enums or #defines for wall fitler types
    switch (wallFilterType)
    {
    case 0:
        wallFilterIndex = wallFilterLow;
        break;

    case 1:
        wallFilterIndex = wallFilterMed;
        break;

    case 2:
        wallFilterIndex = wallFilterHigh;
        break;

    default:
        LOGE("UpsReader::getCFWallFilter(): wallFilterType (%d) is invalid",
             wallFilterType);
        mErrorCount++;
        goto err_ret;
        break;
    }

    //--------- Get Coeffs from WallFilters using wallFilterIndex --------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Coeffs FROM WallFilters WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getCFWallFilter(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, wallFilterIndex);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getCFWallFilter(), errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getCFWallFilter(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    blobSize =sqlite3_column_bytes(pStmt, 0);
    if ( blobSize > 1024 ) // TODO: cleanup wall filter size range
    {
        LOGE("UpsReader::getCFWallFilter(), invalid blob size : %d", blobSize);
        mErrorCount++;
        goto err_ret;
    }
    memcpy(lutPtr, sqlite3_column_blob(pStmt, 0), blobSize);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    numCoeffs = blobSize / sizeof(float);
    goto ok_ret;
err_ret:
ok_ret:
    return (numCoeffs);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getMLinesToAverage(uint32_t imagingCaseId, uint32_t sweepSpeedId)
{
    Sqlite3Stmt stmt;
    uint32_t linesToAverage = 0;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT MSweepSpeedFast, MSweepSpeedMedium, MSweepSpeedSlow FROM ImagingCases, Organs "
                                  "WHERE ImagingCases.ID=? AND Organs.ID=ImagingCases.Organ",
                                  -1, &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);

    switch (sweepSpeedId)  // BUGBUG: create enums or macros for sweepSpeed
    {
    case 0:
        linesToAverage = sqlite3_column_int(stmt, 2);
        break;
    case 1:
        linesToAverage = sqlite3_column_int(stmt, 1);
        break;
    case 2:
        linesToAverage = sqlite3_column_int(stmt, 0);
        break;
    default:
        LOGE("%s: sweepSpeedId %d is invalid", __func__, sweepSpeedId);
        ++mErrorCount;
        break;
    }
    LOGD("%s M lines to average = %d", __func__, linesToAverage);
    return (linesToAverage);
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getImagingCaseLut(const char* idQueryString,
                                      const char* lutQueryString,
                                      uint32_t imagingCaseId,
                                      uint32_t maxSize,
                                      uint32_t* lutPtr)
{
    uint32_t lutSize = 0;
    uint32_t tableId;
    sqlite3_stmt* pStmt;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr, idQueryString, -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("sqlite3_prepare_v2 failed for %s - return code = %d\n",
               idQueryString, rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", idQueryString, rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for %s - return code = %d\n",
             idQueryString, rc);
        mErrorCount++;
        goto err_ret;
    }
    tableId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_prepare_v2(mDb_ptr, lutQueryString, -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("sqlite3_prepare_v2 failed for %s - return code = %d\n",
               lutQueryString, rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, tableId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", lutQueryString, rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for %s - return code = %d\n",
             lutQueryString, rc);
        mErrorCount++;
        goto err_ret;
    }
    lutSize = sqlite3_column_bytes(pStmt, 0);
    if (lutSize > (maxSize << 2))  // comparing byte counts
    {
        LOGE("Size of LUT got for \"%s\" (%ud) is greater than max (%ud)\n",
             lutQueryString, (lutSize >> 2), maxSize);
        lutSize = 0;
        mErrorCount++;
        goto err_ret;
    }
    memcpy( lutPtr, sqlite3_column_blob(pStmt, 0), lutSize);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        lutSize = 0;
        mErrorCount++;
        goto err_ret;
    }

err_ret:
ok_ret:
    return(lutSize >> 2); // returning size in # of 32-bit words
}

size_t UpsReader::getImagingCaseLut_v2(const char *fkey, const char *ftable, const char *colName, uint32_t imagingCaseId, void *lut, size_t size)
{
    Sqlite3Stmt stmt;

    char sql[255];
    // TODO(tim) make sure this doesn't overflow/get cut off
    snprintf(sql, sizeof(sql), "SELECT ftbl.%s FROM ImagingCases, %s AS ftbl WHERE"
        " ImagingCases.%s = ftbl.ID AND ImagingCases.ID = ?", colName, ftable, fkey);

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr, sql, -1, &stmt, nullptr), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);

    size_t nbytes = sqlite3_column_bytes(stmt, 0);
    if (nbytes != size)
    {
        LOGE("Size of LUT got for \"%s\" (%lu) is not equal to expected (%lu)", sql, nbytes, size);
        ++mErrorCount;
        return 0;
    }
    memcpy(lut, sqlite3_column_blob(stmt, 0), nbytes);

    return nbytes;
}

//-----------------------------------------------------------------------------
uint32_t UpsReader::getGlobalLut(const char* queryString, uint32_t maxSize, uint32_t* lutPtr)
{
    uint32_t lutSize = 0;
    sqlite3_stmt* pStmt;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr, queryString, -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("sqlite3_prepare_v2 failed for %s - return code = %d\n",
               queryString, rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    lutSize = sqlite3_column_bytes(pStmt, 0);
    if (lutSize > (maxSize<<2)) // comparing byte counts
    {
        LOGE("Size of LUT got for \"%s\" (%u) is greater than max (%u)\n",
             queryString, (lutSize > 2), maxSize);
        lutSize = 0;
        mErrorCount++;
        goto err_ret;
    }
    memcpy( lutPtr, sqlite3_column_blob(pStmt, 0), lutSize);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        lutSize = 0;
        goto err_ret;
    }

    goto ok_ret;

err_ret:
ok_ret:
    return (lutSize >> 2); // returning size in # of 32-bit words
}

//-----------------------------------------------------------------------------
bool UpsReader::getGainBreakpoints(float& nearGainFrac, float& farGainFrac)
{
    sqlite3_stmt* pStmt;
    bool ret = false;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr, "SELECT BNearGainFraction, BFarGainFraction FROM Globals", -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("sqlite3_prepare_v2 failed for GainFraction - return code = %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for getGainBreakpoints - return code = %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    nearGainFrac = sqlite3_column_double(pStmt, 0);
    farGainFrac  = sqlite3_column_double(pStmt, 1);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    ret = true;
    goto ok_ret;

err_ret:
ok_ret:
    return(ret);
}

//-----------------------------------------------------------------------------
bool UpsReader::getAutoGainParams(uint32_t imagingCaseId,
                                  struct Autoecho::Params& params,
                                  uint8_t* noiseFramePtr)
{
    bool    ret = false;
    sqlite3_stmt* pStmt;
    int rc;
    uint32_t autoEchoId = 0;
    int blobSize = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT AutoEchoID FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getCScanConverterParams(): prepare failed for AutoEchoID, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getCScanConverterParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getCScanConverterParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    autoEchoId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT AxiDecFactor, LatDecFactor, TargetGainDb,"
                            "SnrThresholdDb, StdMin, StdMax, MaxClipDb,"
                            "MinClipDb, GaussianKernelSize, GaussianKernelSigma,"
                            "NoiseFrame, GainCalcInterval, NoiseThresholdDb FROM AutoEcho WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getAutoGainParams(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, autoEchoId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getAutoGainParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getAutoGainParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }

    params.axiDecFactor         = sqlite3_column_int    (pStmt, 0);
    params.latDecFactor         = sqlite3_column_int    (pStmt, 1);
    params.targetGainDb         = sqlite3_column_double (pStmt, 2);
    params.snrThrDb             = sqlite3_column_double (pStmt, 3);
    params.minStdDb             = sqlite3_column_double (pStmt, 4);
    params.maxStdDb             = sqlite3_column_double (pStmt, 5);
    params.maxClipDb            = sqlite3_column_double (pStmt, 6);
    params.minClipDb            = sqlite3_column_double (pStmt, 7);
    params.gaussianKernelSize   = sqlite3_column_int    (pStmt, 8);
    params.gaussianKernelSigma  = sqlite3_column_double (pStmt, 9);
    params.gainCalcInterval     = sqlite3_column_int    (pStmt, 11);
    params.noiseThrDb           = sqlite3_column_double (pStmt, 12);

    ALOGI("Auto Gain params:");
    ALOGI("\t axiDecFactor        = %d", params.axiDecFactor);
    ALOGI("\t latDecFactor        = %d", params.latDecFactor);
    ALOGI("\t targetGainDb        = %f", params.targetGainDb);
    ALOGI("\t snrThrDb            = %f", params.snrThrDb);
    ALOGI("\t minStdDb            = %f", params.minStdDb);
    ALOGI("\t maxStdDb            = %f", params.maxStdDb);
    ALOGI("\t minClipDb           = %f", params.minClipDb);
    ALOGI("\t maxClipDb           = %f", params.maxClipDb);
    ALOGI("\t gaussianKernelSize  = %d", params.gaussianKernelSize);
    ALOGI("\t gaussianKernelSigma = %f", params.gaussianKernelSigma);
    ALOGI("\t gainCalcInterval    = %d", params.gainCalcInterval);
    ALOGI("\t noiseThrDb          = %f", params.noiseThrDb);

    blobSize = sqlite3_column_bytes(pStmt, 10);
    if (MAX_B_FRAME_SIZE < blobSize)
    {
        LOGE("upsReader::getAutoGainParams(): NoiseFrame larger than expected");
        mErrorCount++;
        goto err_ret;
    }
    memcpy(noiseFramePtr, sqlite3_column_blob(pStmt, 10), blobSize);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    ret = true;

err_ret:
    return(ret);
}

//-----------------------------------------------------------------------------
void UpsReader::getFov( uint32_t depthId, ScanDescriptor& sc, uint32_t imagingMode, uint32_t organId)
{
    sqlite3_stmt* pStmt;
    int rc;
    uint32_t organGlobalId = this->getOrganGlobalId(organId);

    switch (imagingMode)
    {
    case IMAGING_MODE_B:
        if (organGlobalId == BLADDER_GLOBAL_ID)
        {
            rc = sqlite3_prepare_v2(mDb_ptr,
                                    "SELECT Depth, FovAzimuthStartBladder, FovAzimuthSpanBladder FROM Depths WHERE ID=?",
                                    -1, &pStmt, 0);
        }
        else
        {
            rc = sqlite3_prepare_v2(mDb_ptr,
                                    "SELECT Depth, FovAzimuthStart, FovAzimuthSpan FROM Depths WHERE ID=?",
                                    -1, &pStmt, 0);
        }
        break;

    case IMAGING_MODE_COLOR:
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT Depth, FovAzimuthStartBC, FovAzimuthSpanBC FROM Depths WHERE ID=?",
                                -1, &pStmt, 0);
        break;


    case IMAGING_MODE_M:
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT Depth, FovAzimuthStartBM, FovAzimuthSpanBM FROM Depths WHERE ID=?",
                                -1, &pStmt, 0);
       break;

    default:
        LOGE("Unsupported imaging mode (%d)", imagingMode);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK != rc)
    {
        LOGE("%s prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    rc = sqlite3_bind_int(pStmt, 1, depthId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", __func__, rc);
        mErrorCount++;
        return;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("%s: step failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    sc.axialStart   = 0.0; // TODO: handle curved linear
    sc.axialSpan    = sqlite3_column_double(pStmt, 0);
    sc.azimuthStart = sqlite3_column_double(pStmt, 1);
    sc.azimuthSpan  = sqlite3_column_double(pStmt, 2);

    LOGI("%s: fov[0] = %f, fov[1] = %f, fov[2] = %f, fov[3] = %f",
         __func__, sc.axialStart, sc.axialSpan, sc.azimuthStart, sc.azimuthSpan );
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        return;
    }
    return;
}

//-----------------------------------------------------------------------------
void UpsReader::getDefaultRoi( uint32_t organId, uint32_t depthId, ScanDescriptor& sc)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT DefaultDepthIndex, RoiAxialStart, RoiAxialSpan, RoiAzimuthStart, RoiAzimuthSpan FROM Organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("%s prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", __func__, rc);
        mErrorCount++;
        return;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("%s: step failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    float roiAtDefaultDepth[4];
    int defaultDepthId;
    defaultDepthId         = sqlite3_column_int   (pStmt, 0);
    roiAtDefaultDepth[0] = sqlite3_column_double(pStmt, 1);
    roiAtDefaultDepth[1] = sqlite3_column_double(pStmt, 2);
    roiAtDefaultDepth[2] = sqlite3_column_double(pStmt, 3);
    roiAtDefaultDepth[3] = sqlite3_column_double(pStmt, 4);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        return;
    }

    //---------------- Get default depth -------------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Depth FROM Depths WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("%s prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    rc = sqlite3_bind_int(pStmt, 1, defaultDepthId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", __func__, rc);
        mErrorCount++;
        return;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("%s: step failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }
    float defaultDepth = sqlite3_column_double(pStmt, 0);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        return;
    }

    //---------------- Get depth value -------------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Depth FROM Depths WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("%s prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    rc = sqlite3_bind_int(pStmt, 1, depthId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", __func__, rc);
        mErrorCount++;
        return;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("%s: step failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }
    float depth = sqlite3_column_double(pStmt, 0);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        return;
    }

    //------------ Translate roi values from default depth to actual depth

    sc.axialStart   = roiAtDefaultDepth[0] * depth / defaultDepth;
    sc.axialSpan    = roiAtDefaultDepth[1] * depth / defaultDepth;
    sc.azimuthStart = roiAtDefaultDepth[2];
    sc.azimuthSpan  = roiAtDefaultDepth[3];

    LOGI("%s: defaultDepth = %f, actualDepth %f, roi[0] = %f, roi[1] = %f, roi[2] = %f, roi[3] = %f",
         __func__,
         defaultDepth, depth,
         sc.axialStart, sc.axialSpan, sc.azimuthStart, sc.azimuthSpan );
    return;
}

//-----------------------------------------------------------------------------
bool UpsReader::getScanConverterParams( uint32_t imagingCaseId,
                                        struct ScanConverterParams& params,
                                        uint32_t imagingMode)
{
    uint32_t organId = 0;
    float depthMm;
    uint32_t numSamples = 0;
    uint32_t organGlobalId = 0;

    params.numSampleMesh = 50;
    params.numRayMesh = 100;
    params.startSampleMm = 0.0f;

    if (!this->getNumSamplesB(imagingCaseId, numSamples))
    {
        LOGE("Failed to get number of samples\n");
        mErrorCount++;
        return(false);
    }
    params.numSamples = numSamples;
    if (!this->getImagingDepthMm(imagingCaseId, depthMm))
    {
        LOGE("Failed to get imaging depth from UPS\n");
        mErrorCount++;
        return(false);
    }

    if (params.numSamples == 0)
    {
        LOGI("NumSamplesB read from UPS is 0\n");
        mErrorCount++;
        return(false);
    }
    params.sampleSpacingMm = depthMm / (params.numSamples - 1);

    organId = this->getImagingOrgan(imagingCaseId);
    organGlobalId = this->getOrganGlobalId(organId);

    Sqlite3Stmt stmt;
    switch (imagingMode)
    {
    case IMAGING_MODE_B:
        if (organGlobalId == BLADDER_GLOBAL_ID) {
            SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                          "SELECT FovAzimuthStartbladder, RxPitchBbladder, NumRxBeamsBbladder FROM Depths WHERE ID in "
                                          "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                          -1, &stmt, 0), SQLITE_OK, false);
        }
        else {
            SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                          "SELECT FovAzimuthStart, RxPitchB, NumRxBeamsB FROM Depths WHERE ID in "
                                          "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                          -1, &stmt, 0), SQLITE_OK, false);
        }
        break;
    case IMAGING_MODE_COLOR:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT FovAzimuthStartBC, RxPitchBC, NumRxBeamsBC FROM Depths WHERE ID in "
                                      "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                      -1, &stmt, 0), SQLITE_OK, false);
        break;
    case IMAGING_MODE_M:
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT FovAzimuthStartBM, RxPitchBM, NumRxBeamsBM FROM Depths WHERE ID in "
                                      "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                      -1, &stmt, 0), SQLITE_OK, false);
        break;
    default:
        LOGE("%s imaging mode %d is invalid", __func__, imagingMode);
        mErrorCount++;
        return (false);
        break;
    }
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    if (params.probeShape == PROBE_SHAPE_LINEAR)
    {
        params.startRayRadian = 0.0f;
        params.startRayMm = sqlite3_column_double(stmt, 0);
    }
    else
    {
        // Phased array
        params.startRayRadian = sqlite3_column_double(stmt, 0);
        params.startRayMm = 0.0f;
    }
    params.raySpacing = sqlite3_column_double(stmt, 1);
    params.numRays = sqlite3_column_int(stmt, 2);

    LOGI("B-Mode Scan converter params:");
    LOGI("\t params.probeShape      = %d", params.probeShape);
    LOGI("\t params.numSamplesMesh  = %d", params.numSampleMesh);
    LOGI("\t params.numRayMesh      = %d", params.numRayMesh);
    LOGI("\t params.numSamples      = %d", params.numSamples);
    LOGI("\t params.numRays         = %d", params.numRays);
    LOGI("\t params.startSampleMm   = %f", params.startSampleMm);
    LOGI("\t params.sampleSpacingMm = %f", params.sampleSpacingMm);
    LOGI("\t params.raySpacing      = %f", params.raySpacing);
    LOGI("\t params.startRayRadian  = %f", params.startRayRadian);
    LOGI("\t params.startRayMm      = %f", params.startRayMm);
    return (true);
}

//-----------------------------------------------------------------------------
bool UpsReader::getCScanConverterParams( uint32_t imagingCaseId,
                                        struct ScanConverterParams& params)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    int rc;
    uint32_t organId = 0;
    float roiHeight;
    float roiAzimuthSpan;
    uint32_t numSamples = 0;
    float bmodeDepthMm;

    params.numSampleMesh = 50;
    params.numRayMesh    = 100;
    params.startSampleMm = 0.0f;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Organ FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getCScanConverterParams(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getCScanConverterParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getCScanConverterParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    organId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT RoiAzimuthStart, RoiAzimuthSpan, RoiAxialStart, RoiAxialSpan, RxPitchC FROM Organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("UpsReader::getCScanConverterParams(): prepare to get items from Organs failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getCScanConverterParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getCScanConverterParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }

    params.startRayRadian = sqlite3_column_double(pStmt, 0);
    roiAzimuthSpan        = sqlite3_column_double(pStmt, 1);
    params.startSampleMm  = sqlite3_column_double(pStmt, 2);
    roiHeight             = sqlite3_column_double(pStmt, 3);
    params.raySpacing     = sqlite3_column_double(pStmt, 4);

    if (0.0 < params.raySpacing)
    {
        params.numRays        = (int)(roiAzimuthSpan/params.raySpacing);
    }
    else
    {
        LOGE("getCScanConverterParams(): RoiRaySpacing read is 0");
        mErrorCount++;
        goto err_ret;
    }
    if (params.numRays <= 0)
    {
        LOGE("getCScanConverterParams(): numRays %d is not > 0", params.numRays);
        mErrorCount++;
        goto err_ret;
    }

    if (!this->getImagingDepthMm(imagingCaseId, bmodeDepthMm))
    {
        LOGE("Failed to get imaging depth from UPS\n");
        mErrorCount++;
        goto err_ret;
    }
    params.numSamples = (uint32_t)(256*(roiHeight/bmodeDepthMm));

    if (params.numSamples & 0x1) // round it up to nearest even number
    {
        params.numSamples += 1;
    }
    params.sampleSpacingMm = roiHeight / (params.numSamples - 1);

    LOGI("Color Scan converter params:");
    LOGI("\t params.numSamplesMesh  = %d", params.numSampleMesh);
    LOGI("\t params.numRayMesh      = %d", params.numRayMesh);
    LOGI("\t params.numSamples      = %d", params.numSamples);
    LOGI("\t params.numRays         = %d", params.numRays);
    LOGI("\t params.startSampleMm   = %f", params.startSampleMm);
    LOGI("\t params.sampleSpacingMm = %f", params.sampleSpacingMm);
    LOGI("\t params.raySpacing      = %f", params.raySpacing);
    LOGI("\t params.startRayRadian  = %f", params.startRayRadian);

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    ret = true;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

bool UpsReader::getBPersistenceLut(uint32_t imagingCaseId, float (&lut)[256])
{
    return 0 != getImagingCaseLut_v2("BPersistenceID", "BPersistence", "PersistenceWeight", imagingCaseId, lut, sizeof(lut));
}

bool UpsReader::getBSpatialFilterParams(uint32_t imagingCaseId, BSpatialFilterParams& params)
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT Mode, BlackholeAxialThresh,BlackholeLateralThresh,BlackholeAlpha,NoiseAxialThresh,"
                                  "NoiseLateralThresh,NoiseAlpha,KernelAxialSize,KernelLateralSize,"
                                  "GaussianAxialSigma,GaussianLateralSigma FROM BSpatialFilter WHERE ID in (SELECT BSpatialFilterID "
                                  "From ImagingCases where ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    params.mode =   sqlite3_column_int(stmt, 0);
    params.blackholeaxialthresh =         sqlite3_column_double(stmt, 1);
    params.blackholelateralthresh =   sqlite3_column_double(stmt, 2);
    params.blackholealpha =   sqlite3_column_double(stmt, 3);
    params.noiseaxialthresh =   sqlite3_column_double(stmt, 4);
    params.noiselateralthresh =   sqlite3_column_double(stmt, 5);
    params.noisealpha =   sqlite3_column_double(stmt, 6);
    params.kernelaxialsize =   sqlite3_column_int(stmt, 7);
    params.kernellateralsize =   sqlite3_column_int(stmt, 8);
    params.gaussianaxialsigma =   sqlite3_column_double(stmt, 9);
    params.gaussianlateralsigma =   sqlite3_column_double(stmt, 10);
    return true;
}

bool UpsReader::getBEdgeFilterParams(uint32_t imagingCaseId, BEdgeFilterParams& params)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT SCBlendMode,SCBlendWeight,LPFKernel,EdgeWeight,OrderMode FROM BEdgeFilter WHERE ID in (SELECT BEdgeFilterID "
                                  "From ImagingCases where ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    params.scBlendMode =   sqlite3_column_int(stmt, 0);
    params.scBlendWeight =  sqlite3_column_double(stmt, 1);
    params.lpfKernel =   sqlite3_column_int(stmt, 2);
    params.edgeWeight =   sqlite3_column_double(stmt, 3);
    params.order =   sqlite3_column_int(stmt, 4);
    return true;
}

bool UpsReader::getBSmoothParams(uint32_t imagingCaseId, BSmoothParams& params)
{
    const char *query =
            "SELECT BSmoothFilterMode, LPFtaps, aCoeff, bCoeff "
            "FROM BSmoothFilter, ImagingCases "
            "WHERE BSmoothFilter.ID = ImagingCases.BSmoothFilterID "
            "AND ImagingCases.ID = ?";
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr, query, -1, &stmt, nullptr), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    params.mode =         sqlite3_column_int(stmt, 0);
    params.numLPFtaps =   sqlite3_column_int(stmt, 1);

    if (params.numLPFtaps == 0)
        params.numLPFtaps = 3;
    params.aCoeffs.resize(params.numLPFtaps);
    unsigned int aCoeffsSize = sqlite3_column_bytes(stmt, 2) / sizeof(float);
    if (aCoeffsSize != params.numLPFtaps)
    {
        LOGE("Incorrect size for BSmoothFilter.aCoeffs (expected %u, actual %u)", params.numLPFtaps, aCoeffsSize);
        return false;
    }
    const float* aCoeffsBegin = (const float*)sqlite3_column_blob(stmt, 2);
    const float* aCoeffsEnd   = aCoeffsBegin + aCoeffsSize;
    std::copy(aCoeffsBegin, aCoeffsEnd, params.aCoeffs.begin());

    params.bCoeffs.resize(params.numLPFtaps);
    unsigned int bCoeffsSize = sqlite3_column_bytes(stmt, 3) / sizeof(float);
    if (bCoeffsSize != params.numLPFtaps)
    {
        LOGE("Incorrect size for BSmoothFilter.bCoeffs (expected %u, actual %u)", params.numLPFtaps, bCoeffsSize);
        return false;
    }
    const float* bCoeffsBegin = (const float*)sqlite3_column_blob(stmt, 3);
    const float* bCoeffsEnd   = bCoeffsBegin + bCoeffsSize;
    std::copy(bCoeffsBegin, bCoeffsEnd, params.bCoeffs.begin());

    return true;
}

//-----------------------------------------------------------------------------
bool UpsReader::getSpeckleReductionParams( uint32_t imagingCaseId,
                                           float (&paramArray)[SpeckleReduction::PARAM_ARRAY_SIZE])
{
    // UNUSED(imagingCaseId);

    // static float fixedParams[30] =
    // {
    //     22,     6060,   11,    11,     30,
    //     20,     44,     10,    7777,   1010,
    //     2222,   33,     606,   2020,   1,
    //     0,      0.5,    6,     33,     33,
    //     551,    1111,   0,     0,      0,
    //     0,      0,      0,     0,      0
    // };

    // memcpy( paramArray, fixedParams, sizeof(fixedParams));

    return 0 != getImagingCaseLut_v2("BSpeckleReductionID", "SpeckleReduction", "Params", imagingCaseId, paramArray, sizeof(paramArray));
}

//-----------------------------------------------------------------------------
bool UpsReader::getColorflowParams(uint32_t imagingCaseId, Colorflow::Params &params)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    uint32_t autoCorrFilterId;
    uint32_t thresholdId;
    uint32_t persistenceId;
    uint32_t smoothingFilterId;
    uint32_t noiseFilterId;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT CFAutoCorrFilterID, CFThresholdID, CFPersistence, CFSmoothFilterID, CFNoiseFilterID FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorflowParams(): prepare failed (step 1), return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorflowParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorflowParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    autoCorrFilterId  = sqlite3_column_int(pStmt, 0);
    thresholdId       = sqlite3_column_int(pStmt, 1);
    persistenceId     = sqlite3_column_int(pStmt, 2);
    smoothingFilterId = sqlite3_column_int(pStmt, 3);
    noiseFilterId = sqlite3_column_int(pStmt, 4);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

   // get AutoCorrParams
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT ACFilterMode, AxialSigmaND, LateralSigmaND, AxialSigmaP, LateralSigmaP, AxialSizeND, LateralSizeND, AxialSizeP, LateralSizeP FROM CFAutoCorrFilter WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorflowParams(): prepare failed (step 2), return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, autoCorrFilterId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorflowParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorflowParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }

    params.autoCorr.acFilterMode    = sqlite3_column_int(pStmt, 0);
    params.autoCorr.axialSigmaND    = sqlite3_column_double(pStmt, 1);
    params.autoCorr.lateralSigmaND  = sqlite3_column_double(pStmt, 2);
    params.autoCorr.axialSigmaP     = sqlite3_column_double(pStmt, 3);
    params.autoCorr.lateralSigmaP   = sqlite3_column_double(pStmt, 4);
    params.autoCorr.axialSizeND     = sqlite3_column_int(pStmt, 5);
    params.autoCorr.lateralSizeND   = sqlite3_column_int(pStmt, 6);
    params.autoCorr.axialSizeP      = sqlite3_column_int(pStmt, 7);
    params.autoCorr.lateralSizeP    = sqlite3_column_int(pStmt, 8);

    // get PersistenceParams
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT WeightVelocity, WeightPower FROM CFPersistence WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorflowParams(): prepare failed (step 3), return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, persistenceId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorflowParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorflowParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    params.persistence.vel   = sqlite3_column_double(pStmt, 0);
    params.persistence.power = sqlite3_column_double(pStmt, 1);

    // get ThrehsoldParams
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT "
                            "VelThreshold, PowerThresholdDb, VarThreshold, ClutterThresholdDb, PowerMaxDb, PowerDynamicRangeDb, GainRangeDb, ClutterFactor "
                            "FROM CFThreshold WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorflowParams(): prepare failed (step 4), return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, thresholdId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorflowParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorflowParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }

    params.threshold.vel             = sqlite3_column_double(pStmt, 0);
    params.threshold.power           = sqlite3_column_double(pStmt, 1);
    params.threshold.var             = sqlite3_column_double(pStmt, 2);
    params.threshold.clutter         = sqlite3_column_double(pStmt, 3);
    params.threshold.powMax          = sqlite3_column_double(pStmt, 4);
    params.threshold.powDynamicRange = sqlite3_column_double(pStmt, 5);
    params.threshold.gainRange       = sqlite3_column_double(pStmt, 6);
    params.threshold.clutterFactor   = sqlite3_column_double(pStmt, 7);

    // Get Smoothing Filter params
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT "
                            "LateralSizeVel, AxialSizeVel, LateralSizeP, AxialSizeP, LateralSigmaVel, AxialSigmaVel, VelSmoothFilterMode, VelThreshold, LateralSizeVel2, AxialSizeVel2, LateralSigmaVel2, AxialSigmaVel2, VelSmoothFilterMode2, PowThreshold "
                            "FROM CFSmoothFilter WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorflowParams(): prepare failed (step 5), return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, smoothingFilterId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorflowParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorflowParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    params.smoothing.lateralSizeVel = sqlite3_column_int(pStmt, 0);
    params.smoothing.axialSizeVel   = sqlite3_column_int(pStmt, 1);
    params.smoothing.lateralSizeP   = sqlite3_column_int(pStmt, 2);
    params.smoothing.axialSizeP     = sqlite3_column_int(pStmt, 3);
    params.smoothing.lateralSigmaVel = sqlite3_column_double(pStmt, 4);
    params.smoothing.axialSigmaVel   = sqlite3_column_double(pStmt, 5);
    params.smoothing.mode     = sqlite3_column_int(pStmt, 6);
    params.smoothing.velThresh    = sqlite3_column_double(pStmt, 7);
    params.smoothing.lateralSizeVel2 = sqlite3_column_int(pStmt, 8);
    params.smoothing.axialSizeVel2   = sqlite3_column_int(pStmt, 9);
    params.smoothing.lateralSigmaVel2 = sqlite3_column_double(pStmt, 10);
    params.smoothing.axialSigmaVel2   = sqlite3_column_double(pStmt, 11);
    params.smoothing.mode2     = sqlite3_column_int(pStmt, 12);
    params.smoothing.powThresh    = sqlite3_column_double(pStmt, 13);
    // Get Noise Filter params
    rc = sqlite3_prepare_v2(mDb_ptr, "SELECT FilterSize, Threshold FROM CFNoiseFilter WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorflowParams(): prepare failed (step 6), return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, noiseFilterId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorflowParams, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorflowParams(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    params.noisefilter.filtersize = sqlite3_column_int(pStmt, 0);
    params.noisefilter.threshold   = sqlite3_column_double(pStmt, 1);
    // params.ensembleSize = getEnsembleSize(imagingCaseId);
    // if (params.ensembleSize == 0)
    // {
    //     LOGE("upsReader::getColorflowParams(): invalid ensembleSize");
    //     mErrorCount++;
    //     goto err_ret;
    // }

    //CPD params
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "CPDCompressDynamicRangeDb, "
                                        "CPDCompressPivotPtIn, "
                                        "CPDCompressPivotPtOut "
                                        "from CPDCompression",
                                        -1,
                                        &pStmt,
                                        0))
    {
        LOGE("prepare failed for %s", __func__);
        mErrorCount++;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("sqlite3_step failed for %s", __func__);
        mErrorCount++;
    }
    params.cpd.cpdCompressDynamicRangeDb = sqlite3_column_double(pStmt, 0);
    params.cpd.cpdCompressPivotPtIn      = sqlite3_column_double(pStmt, 1);
    params.cpd.cpdCompressPivotPtOut     = sqlite3_column_double   (pStmt, 2);

    ret = true;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}


//-----------------------------------------------------------------------------
bool UpsReader::getImagingDepthMm(uint32_t imagingCaseId, float& depth)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    uint32_t depthId = 0;
    int rc;

    depth = 400.0f;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Depth FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getImagingDepth(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getImagingDepth, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getImagingDepth(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    depthId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Depth FROM Depths WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getImagingDepth(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_bind_int(pStmt, 1, depthId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getImagingDepth, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getImagingDepth(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    depth = sqlite3_column_double(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    ret = true;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

//-----------------------------------------------------------------------------
bool UpsReader::getDepthIndex(uint32_t imagingCaseId, uint32_t& depthIndex)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    uint32_t depthId = 0;
    int rc;

    depthIndex = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Depth FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("%s: prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return (false);
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", __func__, rc);
        mErrorCount++;
        return (false);
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("%s: step failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return (false);
    }
    depthIndex = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        return (false);
    }
    return (true);
}

//-----------------------------------------------------------------------------
void UpsReader::getDecimationRate(uint32_t depthIndex, float& decimationRate)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    uint32_t depthId = 0;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT DecimationRate FROM Depths WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("%s: prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }
    rc = sqlite3_bind_int(pStmt, 1, depthIndex);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for %s, errorCode = %d", __func__, rc);
        mErrorCount++;
        return;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("%s: step failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }
    decimationRate = sqlite3_column_double(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        return;
    }
    return;
}

//-----------------------------------------------------------------------------
bool UpsReader::getNumSamples(uint32_t& samples)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    uint32_t depthId = 0;
    int rc;

    samples = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT NumSamplesB FROM Globals",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getNumSamples(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getNumSamples(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    samples = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    ret = true;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

bool UpsReader::getNumSamplesB(uint32_t imagingCaseId, uint32_t& samples)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT NumSamplesB FROM Depths WHERE ID In (SELECT Depth from ImagingCases WHERE ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    samples = sqlite3_column_int(stmt, 0);
    return (true);
}

//-----------------------------------------------------------------------------
int UpsReader::getImagingMode(uint32_t imagingCaseId)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    int mode;
    int rc;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Mode FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getImagingMode(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getImagingMode, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getImagingMode(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    mode = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    if ( (IMAGING_MODE_B      != mode) &&
         (IMAGING_MODE_COLOR  != mode) &&
         (IMAGING_MODE_M      != mode) &&
         (IMAGING_MODE_PW     != mode) &&
         (IMAGING_MODE_CW     != mode) )
    {
        LOGE("upsReader::getImagingMode(): imaging mode (%d) read from UPS is invalid",
             mode);
        mErrorCount++;
        goto err_ret;
    }
    goto ok_ret;

err_ret:
    mode = -1;  // TODO: cleanup error handling

ok_ret:
    return (mode);
}

//-------------------------------------------------------------------------------------
uint32_t UpsReader::getBDummyVector()
{
    int rc;
    sqlite3_stmt* pStmt;
    uint32_t dummyVector = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT BDummyVector from Globals",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getBDummyVector, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite3_step failed for UpsReader::getBDummyVector()\n");
        mErrorCount++;
        goto err_ret;
    }
    dummyVector  = sqlite3_column_int(pStmt, 0);
    LOGI("BDummyVector = %d", dummyVector);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        LOGI("finalize failed for %s", __func__);
        mErrorCount++;
        dummyVector = 0;
        goto err_ret;
    }
err_ret:
    return (dummyVector);
}

//-----------------------------------------------------------------------------
void UpsReader::loadTransducerInfo()
{
    sqlite3_stmt* pStmt;
    int rc;
    unsigned blobSize;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT "
                            "   NumElements, "         // 0
                            "   RadiusOfCurvature, "   // 1
                            "   Apex, "                // 2
                            "   Pitch, "               // 3
                            "   LensDelay, "           // 4
                            "   SkinAdjustSamples, "   // 5
                            "   ArrayElevation, "      // 6
                            "   ArrayAperture, "       // 7
                            "   ElevationFocalDepth, " // 8
                            "   MaxVoltage, "          // 9
                            "   CenterFrequency, "     // 10
                            "   BandwidthPercent, "    // 11
                            "   FovShape, "            // 12
                            "   PositionIntBits, "     // 13
                            "   PositionFracBits, "    // 14
                            "   ElementPositionX, "    // 15
                            "   ElementPositionY "     // 16
                            "FROM TransducerInfo",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::loadTransducerInfo, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite2_step failed for UpsReader::getTransducerInfo\n");
        mErrorCount++;
        goto err_ret;
    }
    mTransducerInfo.numElements         = sqlite3_column_int   (pStmt, 0);
    mTransducerInfo.radiusOfCurvature   = sqlite3_column_double(pStmt, 1);
    mTransducerInfo.apex                = sqlite3_column_double(pStmt, 2);
    mTransducerInfo.pitch               = sqlite3_column_double(pStmt, 3);
    mTransducerInfo.lensDelay           = sqlite3_column_double(pStmt, 4);
    mTransducerInfo.skinAdjustSamples   = sqlite3_column_int   (pStmt, 5);
    mTransducerInfo.arrayElevation      = sqlite3_column_double(pStmt, 6);
    mTransducerInfo.arrayAperture       = sqlite3_column_double(pStmt, 7);
    mTransducerInfo.elevationFocalDepth = sqlite3_column_double(pStmt, 9);
    mTransducerInfo.maxVoltage          = sqlite3_column_double(pStmt, 9);
    mTransducerInfo.centerFrequency     = sqlite3_column_double(pStmt, 10);
    mTransducerInfo.bandwidthPercent    = sqlite3_column_double(pStmt, 11);
    mTransducerInfo.fovShapeIndex       = sqlite3_column_int   (pStmt, 12);
    mTransducerInfo.positionIntBits     = sqlite3_column_int   (pStmt, 13);
    mTransducerInfo.positionFracBits    = sqlite3_column_int   (pStmt, 14);

    blobSize = sqlite3_column_bytes(pStmt, 15);
    if (blobSize > sizeof(mTransducerInfo.elementPositionX))
    {
        LOGE("%s: size of ElementPositionX exceeds expected value", __func__);
        mErrorCount++;
        goto err_ret;
    }
    memcpy( (uint8_t *)mTransducerInfo.elementPositionX, sqlite3_column_blob(pStmt, 15), blobSize);
    blobSize = sqlite3_column_bytes(pStmt, 16);
    if (blobSize > sizeof(mTransducerInfo.elementPositionY))
    {
        LOGE("%s: size of ElementPositionY exceeds expected value", __func__);
        mErrorCount++;
        goto err_ret;
    }
    memcpy( (uint8_t *)mTransducerInfo.elementPositionY, sqlite3_column_blob(pStmt, 16), blobSize);


    rc = sqlite3_finalize(pStmt);

    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    goto ok_ret;

err_ret:
ok_ret:
    return;
}

//-----------------------------------------------------------------------------
bool UpsReader::getTransducerInfo(UpsReader::TransducerInfo& transducerInfo)
{
    bool ret = false;
    sqlite3_stmt* pStmt;
    int rc;
    int blobSize;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT "
                            "   NumElements, "         // 0
                            "   RadiusOfCurvature, "   // 1
                            "   Apex, "                // 2
                            "   Pitch, "               // 3
                            "   LensDelay, "           // 4
                            "   SkinAdjustSamples "    // 5
                            "   ArrayElevation, "      // 6
                            "   ArrayAperture, "       // 7
                            "   ElevationFocalDepth, " // 8
                            "   MaxVoltage, "          // 9
                            "   CenterFrequency, "     // 10
                            "   BandwidthPercent, "    // 11
                            "   FovShape, "            // 12
                            "   PositionIntBits, "     // 13
                            "   PositionFracBits, "    // 14
                            "   ElementPositionX, "    // 15
                            "   ElementPositionY, "    // 16
                            "FROM TransducerInfo",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for UpsReader::getTransducerInfo, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("sqlite2_step failed for UpsReader::getTransducerInfo\n");
        mErrorCount++;
        goto err_ret;
    }
    transducerInfo.numElements         = sqlite3_column_int   (pStmt, 0);
    transducerInfo.radiusOfCurvature   = sqlite3_column_double(pStmt, 1);
    transducerInfo.apex                = sqlite3_column_double(pStmt, 2);
    transducerInfo.pitch               = sqlite3_column_double(pStmt, 3);
    transducerInfo.lensDelay           = sqlite3_column_double(pStmt, 4);
    transducerInfo.skinAdjustSamples   = sqlite3_column_int   (pStmt, 5);
    transducerInfo.arrayElevation      = sqlite3_column_double(pStmt, 6);
    transducerInfo.arrayAperture       = sqlite3_column_double(pStmt, 7);
    transducerInfo.elevationFocalDepth = sqlite3_column_double(pStmt, 8);
    transducerInfo.maxVoltage          = sqlite3_column_double(pStmt, 9);
    transducerInfo.centerFrequency     = sqlite3_column_double(pStmt, 10);
    transducerInfo.bandwidthPercent    = sqlite3_column_double(pStmt, 11);
    transducerInfo.fovShapeIndex       = sqlite3_column_int   (pStmt, 12);
    transducerInfo.positionIntBits     = sqlite3_column_int   (pStmt, 13);
    transducerInfo.positionFracBits    = sqlite3_column_int   (pStmt, 14);

    rc = sqlite3_finalize(pStmt);

    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    ret = true;
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

//-----------------------------------------------------------------------------
void UpsReader::logTransducerInfo()
{
    const TransducerInfo* tinfoPtr = getTransducerInfoPtr();

    LOGI("-----------------  Transducer Info BEGIN --------------------------");
    LOGI(" ");
    LOGI("\t numElements              = %d", tinfoPtr->numElements);
    LOGI("\t radiusOfCurvature        = %f", tinfoPtr->radiusOfCurvature);
    LOGI("\t apex                     = %f", tinfoPtr->apex);
    LOGI("\t pitch                    = %f", tinfoPtr->pitch);
    LOGI("\t lensDelay                = %f", tinfoPtr->lensDelay);
    LOGI("\t arrayElevation           = %f", tinfoPtr->arrayElevation);
    LOGI("\t maxVoltage               = %f", tinfoPtr->maxVoltage);
    LOGI("\t centerFrequency          = %f", tinfoPtr->centerFrequency);
    LOGI("\t bandwidthPercent         = %f", tinfoPtr->bandwidthPercent);
    LOGI("\t positionIntBits          = %d", tinfoPtr->positionIntBits);
    LOGI("\t positionFracBits         = %d", tinfoPtr->positionFracBits);
    LOGI("\t fovShapeIndex            = %d", tinfoPtr->fovShapeIndex);
    LOGI(" ");
    LOGI("-----------------  Transducer Info  END  --------------------------");
}

//-----------------------------------------------------------------------------
void UpsReader::loadGlobals()
{
    sqlite3_stmt* pStmt;

    int rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT "
                                "   SamplingFrequency, "       // 0
                                "   QuantizationFrequency, "   // 1
                                "   MinRoiAzimuthSpan, "       // 2
                                "   MinRoiAxialSpan, "         // 3
                                "   MaxRoiAzimuthSpan, "       // 4
                                "   MaxRoiAxialSpan, "         // 5
                                "   MinRoiAxialStart, "        // 6
                                "   DefaultOrganIndex, "       // 7
                                "   DefaultWorkflowIndex, "    // 8
                                "   TxQuantizationFrequency, " // 9
                                "   TxStart, "                 // 10
                                "   TxRef, "                   // 11
                                "   TxTimingAdjustment, "      // 12
                                "   RxTimingAdjustment, "      // 13
                                "   TxClamp, "                 // 14
                                "   NumSamplesB, "             // 15
                                "   NumSamplesC, "             // 16
                                "   MaxInterleaveFactor, "     // 17
                                "   CTxFocusFraction, "        // 18
                                "   BNearGainFraction, "       // 19
                                "   BFarGainFraction, "        // 20
                                "   AfePgblTime, "             // 21
                                "   AfePsbTime, "              // 22
                                "   AfeInitCommands, "         // 23
                                "   TxApmode, "                // 24
                                "   TopParamBlob, "            // 25
                                "   TopStopParamBlob, "        // 26
                                "   AfeResistSelectID, "       // 27
                                "   DummyVector, "             // 28
                                "   firstDummyVector, "        // 29
                                "   AfeGainSelectIDB, "        // 30
                                "   AfeGainSelectIDC, "        // 31
                                "   AfeVersion, "              // 32
                                "   TargetFrameRate, "         // 33
                                "   SPIoverheadUs, "           // 34
                                "   MinPMtimeUs, "             // 35
                                "   PmHvClkOffsetUs, "         // 36
                                "   BHvClkOffsetUs, "          // 37
                                "   CHvClkOffsetUs, "          // 38
                                "   ProtoVersion, "            // 39
                                "   BDummyVector, "            // 40
                                "   ColorRxAperture, "         // 41
                                "   QuantFR, "                 // 42
                                "   FS_S, "                    // 43
                                "   DM_P, "                    // 44
                                "   BPF_P, "                   // 45
                                "   CTBLimit, "                // 46
                                "   MPrf, "                    // 47
                                "   Ispta3Limit, "             // 48
                                "   MILimit, "                 // 49
                                "   TargetSurfTempC, "         // 50
                                "   AmbientTempC, "            // 51
                                "   AzimuthMarginFraction, "   // 52
                                "   AxialMarginFraction, "     // 53
                                "   MaxProbeTemperature, "     // 54
                                "   PWTargetFrameRate, "       // 55
                                "   TILimit, "                 // 56
                                "   AfeGainSelectIDD "         // 57
                                "FROM Globals",
                       -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("prepare failed for %s", __func__);
        mErrorCount++;
        return;
    }
    rc = sqlite3_step(pStmt);
    if (SQLITE_ROW != rc)
    {
        LOGE("step failed for %s", __func__);
        mErrorCount++;
        return;
    }

    mGlobals.samplingFrequency       = sqlite3_column_double(pStmt, 0);
    mGlobals.quantizationFrequency   = sqlite3_column_double(pStmt, 1);
    mGlobals.minRoiAzimuthSpan       = sqlite3_column_double(pStmt, 2);
    mGlobals.minRoiAxialSpan         = sqlite3_column_double(pStmt, 3);
    mGlobals.maxRoiAzimuthSpan       = sqlite3_column_double(pStmt, 4);
    mGlobals.maxRoiAxialSpan         = sqlite3_column_double(pStmt, 5);
    mGlobals.minRoiAxialStart        = sqlite3_column_double(pStmt, 6);
    mGlobals.defaultOrganIndex       = sqlite3_column_int   (pStmt, 7);
    mGlobals.defaultWorkflowIndex    = sqlite3_column_int   (pStmt, 8);
    mGlobals.txQuantizationFrequency = sqlite3_column_double(pStmt, 9);
    mGlobals.txStart                 = sqlite3_column_int   (pStmt, 10);
    mGlobals.txRef                   = sqlite3_column_int   (pStmt, 11);
    mGlobals.txTimingAdjustment      = sqlite3_column_double(pStmt, 12);
    mGlobals.rxTimingAdjustment      = sqlite3_column_double(pStmt, 13);
    mGlobals.txClamp                 = sqlite3_column_double(pStmt, 14);
    mGlobals.maxBSamplesPerLine      = sqlite3_column_int   (pStmt, 15);
    mGlobals.maxCSamplesPerLine      = sqlite3_column_int   (pStmt, 16);
    mGlobals.maxInterleaveFactor     = sqlite3_column_int   (pStmt, 17);
    mGlobals.cTxFocusFraction        = sqlite3_column_double(pStmt, 18);
    mGlobals.bNearGainFraction       = sqlite3_column_double(pStmt, 19);
    mGlobals.bFarGainFraction        = sqlite3_column_double(pStmt, 20);
    mGlobals.afePgblTime             = sqlite3_column_double(pStmt, 21);
    mGlobals.afePsbTime              = sqlite3_column_double(pStmt, 22);
    mGlobals.afeInitCommands         = sqlite3_column_int   (pStmt, 23);
    mGlobals.txApMode                = sqlite3_column_int   (pStmt, 24);

    mGlobals.afeResistSelectId       = sqlite3_column_int   (pStmt, 27);
    mGlobals.dummyVector             = sqlite3_column_int   (pStmt, 28);
    mGlobals.firstDummyVector        = sqlite3_column_int   (pStmt, 29);
    mGlobals.afeGainSelectIdB        = sqlite3_column_int   (pStmt, 30);
    mGlobals.afeGainSelectIdC        = sqlite3_column_int   (pStmt, 31);
    mGlobals.afeVersion              = sqlite3_column_double(pStmt, 32);
    mGlobals.targetFrameRate         = sqlite3_column_double(pStmt, 33);
    mGlobals.spiOverheadUs           = sqlite3_column_double(pStmt, 34);
    mGlobals.minPmTimeUs             = sqlite3_column_double(pStmt, 35);
    mGlobals.pmHvClkOffsetUs         = sqlite3_column_double(pStmt, 36);
    mGlobals.bHvClkOffsetUs          = sqlite3_column_double(pStmt, 37);
    mGlobals.cHvClkOffsetUs          = sqlite3_column_double(pStmt, 38);
    mGlobals.protoVersion            = sqlite3_column_int   (pStmt, 39);
    mGlobals.bDummyVector            = sqlite3_column_int   (pStmt, 40);
    mGlobals.colorRxAperture         = sqlite3_column_int   (pStmt, 41);
    mGlobals.quantFR                 = sqlite3_column_int   (pStmt, 42);
    mGlobals.fs_S                    = sqlite3_column_int   (pStmt, 43);
    mGlobals.dm_P                    = sqlite3_column_int   (pStmt, 44);
    mGlobals.bpf_P                   = sqlite3_column_int   (pStmt, 45);
    mGlobals.ctbLimit                = sqlite3_column_int   (pStmt, 46);
    mGlobals.mPrf                    = sqlite3_column_double(pStmt, 47);
    mGlobals.ispat3Limit             = sqlite3_column_double(pStmt, 48);
    mGlobals.miLimit                 = sqlite3_column_double(pStmt, 49);
    mGlobals.targetSurfaceTempC      = sqlite3_column_double(pStmt, 50);
    mGlobals.ambientTempC            = sqlite3_column_double(pStmt, 51);
    mGlobals.azimuthMarginFraction   = sqlite3_column_double(pStmt, 52);
    mGlobals.axialMarginFraction     = sqlite3_column_double(pStmt, 53);
    mGlobals.maxProbeTemperature     = sqlite3_column_double(pStmt, 54);
    mGlobals.pwTargetFrameRate       = sqlite3_column_double(pStmt, 55);
    mGlobals.tiLimit                 = sqlite3_column_double(pStmt, 56);
    mGlobals.afeGainSelectIdD        = sqlite3_column_int   (pStmt, 57);

    int blobSize;
    blobSize = sqlite3_column_bytes(pStmt, 25);
    if (sizeof(uint32_t)*TBF_NUM_TOP_REGS != blobSize)
    {
        LOGE("%s, incorrect size of TOP Params blob", __func__);
        mErrorCount++;
        return;
    }
    memcpy((uint8_t *)mGlobals.topParams, sqlite3_column_blob(pStmt, 25), blobSize);

    blobSize = sqlite3_column_bytes(pStmt, 26);
    if (sizeof(uint32_t)*TBF_NUM_TOP_REGS != blobSize)
    {
        LOGE("%s, incorrect size of TOP STOP Params blob", __func__);
        mErrorCount++;
        return;
    }
    memcpy((uint8_t *)mGlobals.topStopParams, sqlite3_column_blob(pStmt, 26), blobSize);
    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

//-----------------------------------------------------------------------------
void UpsReader::logGlobals()
{
    const Globals *glbPtr = getGlobalsPtr();

    LOGI("------------------ Globals read from UPS -------------------------");
    LOGI(" ");
    LOGI("\t samplingFrequency          = %f", glbPtr->samplingFrequency);
    LOGI("\t quantizationFrequency      = %f", glbPtr->samplingFrequency);
    LOGI("\t minRoiAxialSpan            = %f", glbPtr->minRoiAxialSpan);
    LOGI("\t minRoiAzimuthSpan          = %f", glbPtr->minRoiAzimuthSpan);
    LOGI("\t maxRoiAxialSpan            = %f", glbPtr->maxRoiAxialSpan);
    LOGI("\t maxRoiAzimuthSpan          = %f", glbPtr->maxRoiAzimuthSpan);
    LOGI("\t minRoiAxialStart           = %f", glbPtr->minRoiAxialStart);
    LOGI("\t defaultOrganIndex          = %d", glbPtr->defaultOrganIndex);
    LOGI("\t defaultWorkflowIndex       = %d", glbPtr->defaultWorkflowIndex);
    LOGI("\t txQuantizationFrequency    = %f", glbPtr->txQuantizationFrequency);
    LOGI("\t txStart                    = %d", glbPtr->txStart);
    LOGI("\t txRef                      = %d", glbPtr->txRef);
    LOGI("\t txTimingAdjustment         = %f", glbPtr->txTimingAdjustment);
    LOGI("\t rxTimingAdjustment         = %f", glbPtr->txTimingAdjustment);
    LOGI("\t txClamp                    = %f", glbPtr->txClamp);
    LOGI("\t maxBSamplesPerLine         = %d", glbPtr->maxBSamplesPerLine);
    LOGI("\t maxCSamplesPerLine         = %d", glbPtr->maxCSamplesPerLine);
    LOGI("\t maxInterleaveFactor        = %d", glbPtr->maxInterleaveFactor);
    LOGI("\t bNearGainFraction          = %f", glbPtr->bNearGainFraction);
    LOGI("\t bFarGainFraction           = %f", glbPtr->bFarGainFraction);
    LOGI("\t afePgblTime                = %f", glbPtr->afePgblTime);
    LOGI("\t afePsbTime                 = %f", glbPtr->afePsbTime);
    LOGI("\t afeInitCommands            = %d", glbPtr->afeInitCommands);
    LOGI("\t txApMode                   = %d", glbPtr->txApMode);
    LOGI("\t afeResistSelectId          = %d", glbPtr->afeResistSelectId);
    LOGI("\t dummyVector                = %d", glbPtr->dummyVector);
    LOGI("\t firstDummyVector           = %d", glbPtr->firstDummyVector);
    LOGI("\t afeGainSelectIdB           = %d", glbPtr->afeGainSelectIdB);
    LOGI("\t afeGainSelectIdC           = %d", glbPtr->afeGainSelectIdC);
    LOGI("\t afeVersion                 = %f", glbPtr->afeVersion);
    LOGI("\t targetFrameRate            = %f", glbPtr->targetFrameRate);
    LOGI("\t spiOverheadUs              = %f", glbPtr->spiOverheadUs);
    LOGI("\t minPmTimeUs                = %f", glbPtr->minPmTimeUs);
    LOGI("\t pmHvClkOffsetUs            = %f", glbPtr->pmHvClkOffsetUs);
    LOGI("\t bHvClkOffsetUs             = %f", glbPtr->bHvClkOffsetUs);
    LOGI("\t cHvClkOffsetUs             = %f", glbPtr->cHvClkOffsetUs);
    LOGI("\t protoVersion               = %d", glbPtr->protoVersion);
    LOGI("\t bDummyVector               = %d", glbPtr->bDummyVector);
    LOGI("\t colorRxAperture            = %d", glbPtr->colorRxAperture);
    LOGI("\t maxProbeTemperature        = %f", glbPtr->maxProbeTemperature);
    LOGI("\t pwTargetFrameRate          = %f", glbPtr->pwTargetFrameRate);
    LOGI(" ");
    LOGI("------------------ End of Globals read from UPS ------------------");
}

//-----------------------------------------------------------------------------
char* UpsReader::getVersion()
{
    sqlite3_stmt* pStmt;
    int rc;
    uint32_t v_major;
    uint32_t v_minor;
    uint32_t v_release;
    char*    v_date;

    sprintf(mVersionString, "Unable to read UPS version");
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT ReleaseNumMajor, ReleaseNumMinor, ReleaseDate, ReleaseType FROM VersionInfo",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("error reading version info, error code = %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }

    rc = sqlite3_step(pStmt);
    if (SQLITE_ROW != rc)
    {
        LOGE("error reading version info, error code = %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }

    v_major   = sqlite3_column_int(pStmt, 0);
    v_minor   = sqlite3_column_int(pStmt, 1);
    v_date    = (char *)sqlite3_column_text(pStmt, 2);
    v_release = sqlite3_column_int(pStmt, 3);

    sprintf(mVersionString, "%d.%d.%d:%s",
            v_major, v_minor, v_release, v_date);

    sqlite3_finalize(pStmt);
    goto ok_ret;

err_ret:
ok_ret:
    return(mVersionString);
}

//-----------------------------------------------------------------------------
bool UpsReader::getVersionInfo( VersionInfo& versionInfo)
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT ReleaseNumMajor, ReleaseNumMinor, ReleaseType FROM VersionInfo",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    versionInfo.releaseNumMajor = sqlite3_column_int(stmt, 0);
    versionInfo.releaseNumMinor = sqlite3_column_int(stmt, 1);
    versionInfo.releaseType     = sqlite3_column_int(stmt, 2);
    return (true);
}

//-----------------------------------------------------------------------------
int UpsReader::getVelocityMap( uint32_t imagingCaseId, uint32_t* buf )
{
    uint32_t organId;
    sqlite3_stmt* pStmt;
    int velocityMap = 0;
    int blobSize = 0;
    int rc;
    int ret = 0;

    //----------------- Get Organ from ImagingCases ----------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Organ FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getVelocityMap(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getVelocityMap(), errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getVelocityMap(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    organId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    //----------------- Get velocityMap from Organs ---------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT VelocityMap FROM Organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorMap(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorMap, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorMap(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    velocityMap = sqlite3_column_int(pStmt, 0);
    LOGI("Velocity Map index for imaging case %d is %d", imagingCaseId, velocityMap);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    //----------- get the velocity map from Colormap --------------------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
        "SELECT RGB FROM Colormap WHERE ID=?",
        -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorMap(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, velocityMap);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorMap, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorMap(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
     blobSize = sqlite3_column_bytes(pStmt, 0);
    if (1024 != blobSize)  // expect 256 32-bit values
    {
        mErrorCount++;
        goto err_ret;
    }
    memcpy(buf, sqlite3_column_blob(pStmt, 0), blobSize);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    ret = blobSize / sizeof(uint32_t);
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);

}

//-----------------------------------------------------------------------------
int UpsReader::getDefaultColorMapIndex(uint32_t imagingCaseId, uint32_t &colorMapId)
{
    uint32_t organId;
    sqlite3_stmt* pStmt;
    int velocityMap = 0;
    int blobSize = 0;
    int rc;
    int ret = 0;

    //----------------- Get Organ from ImagingCases ----------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Organ FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getVelocityMap(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getVelocityMap(), errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getVelocityMap(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    organId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    //----------------- Get velocityMap from Organs ---------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT VelocityMap FROM Organs WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorMap(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, organId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorMap, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorMap(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    velocityMap = sqlite3_column_int(pStmt, 0);
    LOGI("Velocity Map index for imaging case %d is %d", imagingCaseId, velocityMap);
    colorMapId = velocityMap;
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    goto ok_ret;

err_ret:
ok_ret:
    return (ret);
}

//-----------------------------------------------------------------------------
int UpsReader::getColorMap( uint32_t colorMapId, bool isInverted, uint32_t* buf )
{
    sqlite3_stmt*   pStmt;
    int             blobSize = 0;
    int             rc, i;
    int             ret = 0;
    uint32_t        colorMap[256];

    //----------- get the colorMap map from Colormap --------------------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT RGB FROM Colormap WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getColorMap(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, colorMapId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getColorMap, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getColorMap(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    blobSize = sqlite3_column_bytes(pStmt, 0);
    if (1024 != blobSize)  // expect 256 32-bit values
    {
        mErrorCount++;
        goto err_ret;
    }
    memcpy(colorMap, sqlite3_column_blob(pStmt, 0), blobSize);

    // TODO: update when 2D ColorMap is implemented.
    if (isInverted)
    {
        for (int i = 0; i < 256; i++)
        {
            buf[i] = colorMap[255-i];
        }
    }
    else
    {
        for (int i = 0; i < 256; i++)
        {
            buf[i] = colorMap[i];
        }
    }

    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    ret = blobSize / sizeof(uint32_t);
    goto ok_ret;

err_ret:
ok_ret:
    return (ret);

}

//-----------------------------------------------------------------------------
bool UpsReader::getBEThresholds(uint32_t imagingCaseId, int& velThreshold, int& bThreshold)
{
    sqlite3_stmt* pStmt;
    float vt;
    float bt;
    int backEndId = 0;
    int blobSize = 0;
    int rc;
    int ret = 0;

    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT BackendID FROM ImagingCases WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getBEThresholds(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getBEThresholds, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getBEThresholds(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    backEndId = sqlite3_column_int(pStmt, 0);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }

    //-------------------------------------------------------
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT BEVelThreshold, BThreshold FROM Backend WHERE ID=?",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("upsReader::getBEThresholds(): prepare failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_bind_int(pStmt, 1, backEndId);
    if (SQLITE_OK != rc)
    {
        LOGE("bind failed for UpsReader::getBEThresholds, errorCode = %d", rc);
        mErrorCount++;
        goto err_ret;
    }
    rc = sqlite3_step( pStmt );
    if (SQLITE_ROW != rc)
    {
        LOGE("upsReader::getBEThresholds(): step failed, return code is %d\n", rc);
        mErrorCount++;
        goto err_ret;
    }
    vt = sqlite3_column_double(pStmt, 0);
    bt = sqlite3_column_double(pStmt, 1);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
        goto err_ret;
    }
    if ( (vt >= 0.5) || (vt < 0.0) || (bt > 1.0) || (bt < 0.0) )
    {
        LOGE("UPS values for BEVelThreshold (%f), BThreshold (%f) are out of range",
             vt, bt);
        mErrorCount++;
        goto err_ret;
    }

    velThreshold = (uint8_t)( floor(vt*128.0) );
    bThreshold = (uint8_t)( floor(bt*255.0) );

    ret = true;
    goto ok_ret;

err_ret:
ok_ret:
    return(ret);

}


//------------------------------------------------------------------------------------------
void UpsReader::getPrfList(float prfList[], uint32_t& numValidPrfs)
{
    sqlite3_stmt* pStmt;
    int rc;

    numValidPrfs = 0;
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT PrfHz FROM PRFs",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("%s: prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    unsigned prfCount = 0;
    for ( ; (sqlite3_step(pStmt) == SQLITE_ROW) && (prfCount < MAX_PRF_SETTINGS); prfCount++ )
    {
        prfList[prfCount] = sqlite3_column_double(pStmt, 0);
    }
    LOGD("%s read %d PRF settings", __func__, prfCount);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
    }
    numValidPrfs = prfCount;
    return;
}

//------------------------------------------------------------------------------------------
float UpsReader::getSpeedOfSound(uint32_t imagingCaseId)
{
    float sos = 1.54;  // mm per us
    uint32_t organId;
    uint32_t soundSpeedIndex;
    sqlite3_stmt* pStmt;
    int rc;

    {
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT Organ from ImagingCases WHERE ID=?",
                                -1, &pStmt, 0);
        if (SQLITE_OK != rc)
        {
            LOGE("prepare failed for %s,  errorCode = %d", __func__, rc);
            mErrorCount++;
            return(sos);
        }
        rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
        if (SQLITE_OK != rc)
        {
            LOGE("bind failed for %s, errorCode = %d", __func__, rc);
            mErrorCount++;
            return (sos);
        }
        rc = sqlite3_step( pStmt );
        if (SQLITE_ROW != rc)
        {
            LOGE("sqlite3_step failed for %s", __func__);
            mErrorCount++;
            return (sos);
        }
        organId = sqlite3_column_int(pStmt, 0);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return(sos);
        }
    }
    {
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT SoundSpeedIndex from Organs WHERE ID=?",
                                -1, &pStmt, 0);
        if (SQLITE_OK != rc)
        {
            LOGE("prepare failed for %s,  errorCode = %d", __func__, rc);
            mErrorCount++;
            return(sos);
        }
        rc = sqlite3_bind_int(pStmt, 1, organId);
        if (SQLITE_OK != rc)
        {
            LOGE("bind failed for %s, errorCode = %d", __func__, rc);
            mErrorCount++;
            return (sos);
        }
        if (SQLITE_ROW != sqlite3_step(pStmt))
        {
            LOGE("sqlite3_step failed for %s", __func__);
            mErrorCount++;
            return (sos);
        }
        soundSpeedIndex = sqlite3_column_int(pStmt, 0);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return(sos);
        }
    }
    {
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT SpeedOfSound from SoundSpeed WHERE ID=?",
                                -1, &pStmt, 0);
        if (SQLITE_OK != rc)
        {
            LOGE("prepare failed for %s,  errorCode = %d", __func__, rc);
            mErrorCount++;
            return(sos);
        }
        rc = sqlite3_bind_int(pStmt, 1, soundSpeedIndex);
        if (SQLITE_OK != rc)
        {
            LOGE("bind failed for %s, errorCode = %d", __func__, rc);
            mErrorCount++;
            return (sos);
        }
        if (SQLITE_ROW != sqlite3_step(pStmt))
        {
            LOGE("sqlite3_step failed for %s", __func__);
            mErrorCount++;
            return (sos);
        }
        sos = sqlite3_column_double(pStmt, 0);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
        }
    }
    return (sos);
}


//------------------------------------------------------------------------------------------
float UpsReader::getColorRxPitch( uint32_t imagingCaseId )
{
    uint32_t organId = 0;
    float    rxPitch = 0.0;
    sqlite3_stmt* pStmt;
    int rc;

    {
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT Organ from ImagingCases WHERE ID=?",
                                -1, &pStmt, 0);
        if (SQLITE_OK != rc)
        {
            LOGE("prepare failed for %s,  errorCode = %d", __func__, rc);
            mErrorCount++;
            return(rxPitch);
        }
        rc = sqlite3_bind_int(pStmt, 1, imagingCaseId);
        if (SQLITE_OK != rc)
        {
            LOGE("bind failed for %s, errorCode = %d", __func__, rc);
            mErrorCount++;
            return (rxPitch);
        }
        rc = sqlite3_step( pStmt );
        if (SQLITE_ROW != rc)
        {
            LOGE("sqlite3_step failed for %s", __func__);
            mErrorCount++;
            return (rxPitch);
        }
        organId = sqlite3_column_int(pStmt, 0);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return(rxPitch);
        }
    }
    {
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT RxPitchC from Organs WHERE ID=?",
                                -1, &pStmt, 0);
        if (SQLITE_OK != rc)
        {
            LOGE("prepare failed for %s,  errorCode = %d", __func__, rc);
            mErrorCount++;
            return(rxPitch);
        }
        rc = sqlite3_bind_int(pStmt, 1, organId);
        if (SQLITE_OK != rc)
        {
            LOGE("bind failed for %s, errorCode = %d", __func__, rc);
            mErrorCount++;
            return (rxPitch);
        }
        rc = sqlite3_step( pStmt );
        if (SQLITE_ROW != rc)
        {
            LOGE("sqlite3_step failed for %s", __func__);
            mErrorCount++;
            return (rxPitch);
        }
        rxPitch = sqlite3_column_double(pStmt, 0);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return(rxPitch);
        }
    }
    return (rxPitch);
}

//------------------------------------------------------------------------------------------
uint32_t UpsReader::getColorRxAperture( uint32_t imagingCaseId )
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT ColorRxAperture FROM ORGANS WHERE ID in "
                                  "(SELECT Organ FROM ImagingCases WHERE ID = ?)",
                                  -1,
                                  &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    uint32_t rxAperture = sqlite3_column_int(stmt, 0);
    return (rxAperture);
}

//------------------------------------------------------------------------------------------
void UpsReader::getColorBpfParams(uint32_t imagingCaseId,
                                  float&    centerFrequency,
                                  float&    bandwidth,
                                  uint32_t& numTaps,
                                  uint32_t& bpfScale)
{
    sqlite3_stmt* pStmt;
    uint32_t bpfIndex;

    // Initialize with some valid values
    centerFrequency = 3.0;   // MHz
    bandwidth       = 80.0;  // % value
    numTaps         = 32;
    bpfScale        = 3;

    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "CenterFrequency, "
                                        "Bandwidth, "
                                        "Taps, "
                                        "Scale "
                                        "from BandPassFilters WHERE ID IN "
                                        "(SELECT ColorBPFIndex FROM BandpassFilters WHERE ID IN "
                                        "(SELECT BPF1 FROM ImagingCases WHERE ID = ?))",
                                        -1,
                                        &pStmt,
                                        0))
    {
        LOGE("prepare failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_OK != sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("sqlite3_step failed for %s", __func__);
        mErrorCount++;
        return;
    }
    centerFrequency = sqlite3_column_double(pStmt, 0);
    bandwidth       = sqlite3_column_double(pStmt, 1);
    numTaps         = sqlite3_column_int   (pStmt, 2);
    bpfScale        = sqlite3_column_int   (pStmt, 3);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    LOGD("%s centerFrequency = %f, bandwidth = %f, numTaps = %d, bpfScale = %d",
         __func__, centerFrequency, bandwidth, numTaps, bpfScale);
}


//------------------------------------------------------------------------------------------
void UpsReader::getColorBpfLut(uint32_t imagingCaseId,
                               uint32_t *coeffsI,
                               uint32_t *coeffsQ )

{
    sqlite3_stmt* pStmt;
    uint32_t bpfIndex;
#if 0
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "CoeffsI, CoeffsQ "
                                        "from BandPassFilters WHERE ID IN "
                                        "(SELECT ColorBPFIndex FROM BandpassFilters WHERE ID IN "
                                        "(SELECT BPF1 FROM ImagingCases WHERE ID = ?))",
                                        -1,
                                        &pStmt,
                                        0))
#else
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "CoeffsI, CoeffsQ "
                                        "from BandPassFilters WHERE ID IN "
                                        "(SELECT BPF1 FROM ImagingCases WHERE ID = ?)",
                                        -1,
                                        &pStmt,
                                        0))
#endif
    {
        LOGE("prepare failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_OK != sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("sqlite3_step failed for %s", __func__);
        mErrorCount++;
        return;
    }
    uint32_t  lutSize = sqlite3_column_bytes(pStmt, 0);
    memcpy( coeffsI, sqlite3_column_blob(pStmt, 0), lutSize);

    lutSize = sqlite3_column_bytes(pStmt, 1);
    memcpy( coeffsQ, sqlite3_column_blob(pStmt, 1), lutSize);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
}

//------------------------------------------------------------------------------------------
void UpsReader::getColorFocalBreakpoints( uint32_t imagingCaseId,
                                          uint32_t& numBreakpointsC,
                                          uint32_t& numBreakpointsB)
{
    sqlite3_stmt* pStmt;
    uint32_t firId;

    numBreakpointsC = 1;
    numBreakpointsB = 1;

    // Get index into FocalInterpolationRateID
    {
        if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                            "SELECT FocalInterpolationRateID from ImagingCases WHERE ID=?",
                                            -1,
                                            &pStmt,
                                            0))
        {
            LOGE("prepare failed for %s", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_OK != sqlite3_bind_int(pStmt, 1, imagingCaseId))
        {
            LOGE("bind failed for %s", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_ROW != sqlite3_step(pStmt))
        {
            LOGE("sqlite3_step failed for %s", __func__);
            mErrorCount++;
            return;
        }
        firId = sqlite3_column_int(pStmt, 0);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return;
        }
    }
    // Get the number of breakpoints from the UPS table FocalInterpolationRateLUT
    {
        if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                            "SELECT NumBkptsC, NumBkpts from FocalInterpolationRateLUT WHERE ID=?",
                                            -1,
                                            &pStmt,
                                            0))
        {
            LOGE("prepare failed for %s", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_OK != sqlite3_bind_int(pStmt, 1, firId))
        {
            LOGE("bind failed for %s", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_ROW != sqlite3_step(pStmt))
        {
            LOGE("sqlite3_step failed for %s", __func__);
            mErrorCount++;
            return;
        }
        numBreakpointsC = sqlite3_column_int(pStmt, 0) + 1;
        numBreakpointsB = sqlite3_column_int(pStmt, 1) + 1;
        LOGD("Color focal breakpoints = %d, B-mode focal breakpoints = %d", numBreakpointsC, numBreakpointsB);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return;
        }
    }
}

//------------------------------------------------------------------------------------------
void UpsReader::getColorGainBreakpoints(uint32_t breakpoints[])
{
    sqlite3_stmt* pStmt;

    breakpoints[0] = 0;
    breakpoints[1] = 0;

    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT Code from GainInterpolationRateLUT WHERE ID=1",
                                        -1,
                                        &pStmt,
                                        0))
    {
        LOGE("prepare failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("sqlite3_step failed for %s", __func__);
        mErrorCount++;
        return;
    }
    uint32_t sizeBytes = sqlite3_column_bytes(pStmt, 0);
    if (sizeBytes != 2*sizeof(uint32_t))
    {
        LOGE("%s: wrong number of breakpoints", __func__);
        mErrorCount++;
        return;
    }
    memcpy( (uint8_t *)breakpoints, sqlite3_column_blob(pStmt, 0), sizeBytes);

    LOGD("ColorGain breakpoints = %d %d", breakpoints[0], breakpoints[1]);
    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
}

//------------------------------------------------------------------------------------------
void UpsReader::getFovInfoForColorAcq( uint32_t  imagingCaseId,
                                       uint32_t& numTxBeamsBC,
                                       float&    rxPitchBC,
                                       float&    fovAzimuthSpan)
{
    uint32_t depthId;
    sqlite3_stmt* pStmt;

    // Initialize return values
    numTxBeamsBC   = 28;
    rxPitchBC      = 0.013692;
    fovAzimuthSpan = 1.5638;

    // Get Depth index
    {
        if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                            "SELECT Depth FROM ImagingCases WHERE ID=?",
                                            -1, &pStmt, 0))
        {
            LOGE("%s prepare failed", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
        {
            LOGE("bind failed for %s", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_ROW != sqlite3_step(pStmt))
        {
            LOGE("%s step failed", __func__);
            mErrorCount++;
        }
        depthId = sqlite3_column_int(pStmt, 0);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return;
        }
    }
    {
        if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                            "SELECT "
                                            "NumTxBeamsBC, "
                                            "RxPitchBC, "
                                            "FovAzimuthSpanBC "
                                            "from Depths where ID = ?",
                                            -1,
                                            &pStmt,
                                            0))
        {
            LOGE("prepare failed for %s", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_OK != sqlite3_bind_int(pStmt, 1, depthId))
        {
            LOGE("bind failed for %s", __func__);
            mErrorCount++;
            return;
        }
        if (SQLITE_ROW != sqlite3_step(pStmt))
        {
            LOGE("%s step failed", __func__);
            mErrorCount++;
        }
        numTxBeamsBC   = sqlite3_column_int   (pStmt, 0);
        rxPitchBC      = sqlite3_column_double(pStmt, 1);
        fovAzimuthSpan = sqlite3_column_double(pStmt, 2);
        if (SQLITE_OK != sqlite3_finalize(pStmt))
        {
            mErrorCount++;
            return;
        }
        LOGD("%s numTxBeamsBC = %d, RxPitchBC = %f, FovAzimuthSpan = %f",
             __func__, numTxBeamsBC, rxPitchBC, fovAzimuthSpan);
    }
}

//-----------------------------------------------------------------------------
void UpsReader::getFocalInterpolationRateParams( uint32_t imagingCaseId, int32_t code[], int32_t codeCRoiMax[], uint32_t& codeCRoiMaxLength )
{
    sqlite3_stmt* pStmt;

    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT Code, CodeCRoiMax from FocalInterpolationRateLUT where ID in "
                                        "(SELECT FocalInterpolationRateID FROM ImagingCases WHERE ID=?)",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }
    uint32_t lutSize;

    lutSize = sqlite3_column_bytes(pStmt, 0);
    memcpy( code, sqlite3_column_blob(pStmt, 0), lutSize);


    lutSize = sqlite3_column_bytes(pStmt, 1);
    memcpy( codeCRoiMax, sqlite3_column_blob(pStmt, 1), lutSize);
    codeCRoiMaxLength = lutSize >> 2;

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
}

//----------------------------------------------------------------------------------------------------------
void UpsReader::getNumFocalBkpts( uint32_t imagingCaseId, uint32_t &numBkptsB, uint32_t &numBkptsCRoiMax )
{
    sqlite3_stmt* pStmt;


    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT NumBkpts, NumBkptsCRoiMax from FocalInterpolationRateLUT where ID in "
                                        "(SELECT FocalInterpolationRateID FROM ImagingCases WHERE ID=?)",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    numBkptsB       = sqlite3_column_int  (pStmt, 0);
    numBkptsCRoiMax = sqlite3_column_int  (pStmt, 1);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
}

//-----------------------------------------------------------------------------
void UpsReader::getFocalCompensationParams( uint32_t imagingCaseId,
                                            int32_t beam0X[],
                                            int32_t beam0Y[],
                                            int32_t beam1Y[],
                                            int32_t beam2Y[],
                                            int32_t beam3Y[],
                                            int32_t beam0XCRoiMax[],
                                            int32_t beam0YCRoiMax[],
                                            int32_t beam1YCRoiMax[],
                                            int32_t beam2YCRoiMax[],
                                            int32_t beam3YCRoiMax[] )

{
    sqlite3_stmt* pStmt;

    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "Beam0X,Beam0Y,Beam1Y,Beam2Y,Beam3Y, "
                                        "Beam0XCRoiMax,Beam0YCRoiMax,Beam1YCRoiMax,Beam2YCRoiMax,Beam3YCRoiMax "
                                        "FROM FocalCompensation where ID in "
                                        "(SELECT FocalCompensationID FROM ImagingCases WHERE ID=?)",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }
    uint32_t lutSize;

    lutSize = sqlite3_column_bytes(pStmt, 0);
    memcpy( beam0X, sqlite3_column_blob(pStmt, 0), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 1);
    memcpy( beam0Y, sqlite3_column_blob(pStmt, 1), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 2);
    memcpy( beam1Y, sqlite3_column_blob(pStmt, 2), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 3);
    memcpy( beam2Y, sqlite3_column_blob(pStmt, 3), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 4);
    memcpy( beam3Y, sqlite3_column_blob(pStmt, 4), lutSize);

    lutSize = sqlite3_column_bytes(pStmt, 5);
    memcpy( beam0XCRoiMax, sqlite3_column_blob(pStmt, 5), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 6);
    memcpy( beam0YCRoiMax, sqlite3_column_blob(pStmt, 6), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 7);
    memcpy( beam1YCRoiMax, sqlite3_column_blob(pStmt, 7), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 8);
    memcpy( beam2YCRoiMax, sqlite3_column_blob(pStmt, 8), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 9);
    memcpy( beam3YCRoiMax, sqlite3_column_blob(pStmt, 9), lutSize);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}


//-----------------------------------------------------------------------------
void UpsReader::getColorFocalCompensationParams( uint32_t imagingCaseId,
                                                 int32_t beam0XCRoiMax[],
                                                 uint32_t &intBits,
                                                 uint32_t &fracBits )

{
    sqlite3_stmt* pStmt;


    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "Beam0XCRoiMax, "
                                        "CoeffIntBits, "
                                        "CoeffFracBits "
                                        "FROM FocalCompensation WHERE ID in "
                                        "(SELECT FocalCompensationID FROM ImagingCases WHERE ID=?)",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }
    uint32_t lutSize;

    lutSize = sqlite3_column_bytes(pStmt, 0);
    memcpy( beam0XCRoiMax, sqlite3_column_blob(pStmt, 0), lutSize);
    intBits  = sqlite3_column_int  (pStmt, 1);
    fracBits = sqlite3_column_int  (pStmt, 2);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

//-----------------------------------------------------------------------------
void UpsReader::getApodizationScaleParams( uint32_t imagingCaseId,
                                           int32_t scaleBeam0[],
                                           int32_t scaleCRoiMax[])
{
    getApodizationScaleLut( imagingCaseId, (uint32_t *)scaleBeam0 );
    getApodizationScaleCRoiMaxLut( imagingCaseId, (uint32_t *)scaleCRoiMax );
}

//-----------------------------------------------------------------------------
void UpsReader::getGainDgcParams( const uint32_t imagingCaseId,
                                  uint32_t &intBits,
                                  uint32_t &fracBits,
                                  uint32_t gainValues01[],
                                  uint32_t gainValues23[],
                                  uint32_t gainValues01CRoiMax[],
                                  uint32_t gainValues23CRoiMax[] )
{
    sqlite3_stmt* pStmt;

    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "CoeffIntBits, "
                                        "CoeffFracBits, "
                                        "GainValues01, "
                                        "GainValues23, "
                                        "GainValues01CRoiMax, "
                                        "GainValues23CRoiMax "
                                        "from GainDgc WHERE ID in "
                                        "(SELECT GainDgcID from ImagingCases WHERE ID = ?)",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }
    intBits  = sqlite3_column_int  (pStmt, 0);
    fracBits = sqlite3_column_int  (pStmt, 1);

    uint32_t lutSize;
    lutSize = sqlite3_column_bytes(pStmt, 2);
    memcpy( gainValues01, sqlite3_column_blob(pStmt, 2), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 3);
    memcpy( gainValues23, sqlite3_column_blob(pStmt, 3), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 4);
    memcpy( gainValues01CRoiMax, sqlite3_column_blob(pStmt, 4), lutSize);
    lutSize = sqlite3_column_bytes(pStmt, 5);
    memcpy( gainValues23CRoiMax, sqlite3_column_blob(pStmt, 5), lutSize);


    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}



void UpsReader::getNumTxRxBeams( uint32_t imagingCaseId,
                                 uint32_t &numTxBeamsB,
                                 uint32_t &numRxBeamsB,
                                 uint32_t &numTxBeamsBC,
                                 uint32_t &numRxBeamsBC )
{
    sqlite3_stmt* pStmt;
    uint32_t organId;
    uint32_t organGlobalId;

    organId = this->getImagingOrgan(imagingCaseId);
    organGlobalId = this->getOrganGlobalId(organId);

    // get rest of requested stuff
    if (organGlobalId == BLADDER_GLOBAL_ID)
    {
        if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                            "SELECT "
                                            "NumTxBeamsBbladder, NumRxBeamsBbladder, NumTxBeamsBC, NumRxBeamsBC from Depths "
                                            "WHERE ID IN "
                                            "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                            -1, &pStmt, 0))
        {
            LOGE("%s prepare failed", __func__);
            mErrorCount++;
            return;
        }

    }
    else
    {
        if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                            "SELECT "
                                            "NumTxBeamsB, NumRxBeamsB, NumTxBeamsBC, NumRxBeamsBC from Depths "
                                            "WHERE ID IN "
                                            "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                            -1, &pStmt, 0))
        {
            LOGE("%s prepare failed", __func__);
            mErrorCount++;
            return;
        }
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    numTxBeamsB  = sqlite3_column_int(pStmt, 0);
    numRxBeamsB  = sqlite3_column_int(pStmt, 1);
    numTxBeamsBC = sqlite3_column_int(pStmt, 2);
    numRxBeamsBC = sqlite3_column_int(pStmt, 3);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

//-----------------------------------------------------------------------------------------------------
void UpsReader::getBWaveformInfo( uint32_t imagingCaseId,
                                  uint32_t &txApertureElement,
                                  float    &centerFrequency,
                                  uint32_t &numHalfCycles )
{
    sqlite3_stmt* pStmt;
    uint32_t focalCompensationId = 0;

    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "TxApertureElement, "
                                        "CenterFrequency, "
                                        "NumHalfCycles "
                                        "FROM TxWaveforms WHERE ID in "
                                        "(SELECT TxWaveformB from ImagingCases WHERE ID = ?)",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    txApertureElement  = sqlite3_column_int   (pStmt, 0);
    centerFrequency    = sqlite3_column_double(pStmt, 1);
    numHalfCycles      = sqlite3_column_int   (pStmt, 2);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

//------------------------------------------------------------------------------------------
void UpsReader::getColorWaveformInfo( uint32_t imagingCaseId,
                                      uint32_t &txApertureElement,
                                      float    &centerFrequency,
                                      uint32_t &numHalfCycles )
{
    sqlite3_stmt* pStmt;

    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "TxApertureElement, "
                                        "CenterFrequency, "
                                        "NumHalfCycles "
                                        "FROM TxWaveforms WHERE ID in "
                                        "(SELECT ColorTxIndex from TxWaveforms WHERE ID in "
                                        "(SELECT TxWaveformB from ImagingCases WHERE ID = ?))",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, imagingCaseId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    txApertureElement  = sqlite3_column_int   (pStmt, 0);
    centerFrequency    = sqlite3_column_double(pStmt, 1);
    numHalfCycles      = sqlite3_column_int   (pStmt, 2);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

uint32_t UpsReader::getEcgBlob(bool isInternal, uint32_t leadNum, uint32_t daecgId, uint32_t ecgBlob[])
{
    Sqlite3Stmt stmt;
    uint32_t blobSize = 0;

    if (isInternal)
    {
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT ECGBlobINT1 FROM DAECG WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, 0);
        /*
        switch (leadNum)
        {
            case 1:
                SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                              "SELECT ECGBlobINT1 FROM DAECG WHERE ID = ?",
                                              -1, &stmt, 0), SQLITE_OK, 0);
                break;

            case 2:
                SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                              "SELECT ECGBlobINT2 FROM DAECG WHERE ID = ?",
                                              -1, &stmt, 0), SQLITE_OK, 0);
                break;

            case 3:
                SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                              "SELECT ECGBlobINT3 FROM DAECG WHERE ID = ?",
                                              -1, &stmt, 0), SQLITE_OK, 0);
                break;

            default:
                LOGE( "invalid internal lead numnber (%d)", leadNum);
                mErrorCount++;
                return (0);
        }
         */
    }
    else
    {
        switch (leadNum)
        {
            case 1:
                SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                              "SELECT ECGBlobEXT1 FROM DAECG WHERE ID = ?",
                                              -1, &stmt, 0), SQLITE_OK, 0);
                break;

            case 2:
                SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                              "SELECT ECGBlobEXT2 FROM DAECG WHERE ID = ?",
                                              -1, &stmt, 0), SQLITE_OK, 0);
                break;

            case 3:
                SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                              "SELECT ECGBlobEXT3 FROM DAECG WHERE ID = ?",
                                              -1, &stmt, 0), SQLITE_OK, 0);
                break;

            default:
                LOGE( "invalid external lead numnber (%d)", leadNum);
                mErrorCount++;
                return (0);
        }
    }

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, daecgId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);

    blobSize = sqlite3_column_bytes(stmt, 0);
    if (blobSize > MAX_ECG_BLOB_SIZE*sizeof(uint32_t))
    {
        LOGE("%s, size of ECG blob (%d) exceeds max size (%d)", __func__, blobSize,  MAX_ECG_BLOB_SIZE*sizeof(uint32_t));
        mErrorCount++;
        return (0);
    }
    if (blobSize == 0)
    {
        LOGE("%s, ECG blob in UPS in null", __func__);
        mErrorCount++;
        return (0);
    }
    memcpy(ecgBlob, sqlite3_column_blob(stmt, 0), blobSize);

    return (blobSize>>2);
}

uint32_t UpsReader::getGyroBlob(uint32_t daEcgId, uint32_t gyroBlob[])
{
    Sqlite3Stmt stmt;
    uint32_t blobSize = 0;


    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                        "SELECT GyroBlob FROM DAECG WHERE ID = ?",
                        -1, &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, daEcgId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);

    blobSize = sqlite3_column_bytes(stmt, 0);
    if (blobSize > MAX_GYRO_BLOB_SIZE*sizeof(uint32_t))
    {
        LOGE("%s, size of GyroBlob (%d) exceeds max size (%d)", __func__, blobSize,  MAX_ECG_BLOB_SIZE*sizeof(uint32_t));
        mErrorCount++;
        return (0);
    }
    if (blobSize == 0)
    {
        LOGE("%s, GyroBlob in UPS in null", __func__);
        mErrorCount++;
        return (0);
    }
    memcpy(gyroBlob, sqlite3_column_blob(stmt, 0), blobSize);

    return (blobSize>>2);  // size in uint32_t words
}

bool UpsReader::getAfeClksPerDaEcgSample(uint32_t daEcgId,
                                  uint32_t &afeClksPerDaSample,
                                  uint32_t &afeClksPerEcgSample)
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                        "SELECT AfeClksPerDaSample, AfeClksPerEcgSample FROM DAECG WHERE ID = ?",
                        -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, daEcgId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    // afeClksPerDaSample = sqlite3_column_int(stmt, 0);
    afeClksPerDaSample = 2048;
    afeClksPerEcgSample = sqlite3_column_int(stmt, 1);

    return (true);
}

bool UpsReader::getEcgADCMax(uint32_t daEcgId, uint32_t &adcMax)
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT ADC_MAX FROM DAECG WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, daEcgId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    adcMax = sqlite3_column_int(stmt, 0);
    return (true);
}

uint32_t UpsReader::getDaCoeff(uint32_t daId, float daCoeff[]) {
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT Coeffs FROM DAFilter WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, daId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    int32_t blobSize = sqlite3_column_bytes(stmt, 0);
    if ( (blobSize <= 0) || (blobSize > 8192) || ((blobSize % 4) != 0))
    {
        LOGE("UpsReader::getDaCoeff(), invalid blob size : %d", blobSize);
        mErrorCount++;
        return (0);
    }

    uint32_t numCoeff = (uint32_t)blobSize/sizeof(double);

    //memcpy(daCoeff, sqlite3_column_blob(stmt, 0), blobSize);

    // Convert to float to reduce burden on DA processing.
    const double* vptr = (const double*)sqlite3_column_blob(stmt, 0);
    for (uint32_t ii=0; ii<numCoeff; ii++) {
        daCoeff[ii] = (float)(*(vptr+ii));
    }

    return (numCoeff);
}

bool UpsReader::getPsClkDivForEcg(uint32_t daEcgId, uint32_t &psClkDiv)
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT PsClkDiv FROM DAECG WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, daEcgId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    psClkDiv = sqlite3_column_int(stmt, 0);
    return(true);
}

void UpsReader::getFocus( uint32_t depthIndex, float &focus )
{
    sqlite3_stmt* pStmt;

    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT Focus FROM Depths WHERE ID = ?",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, depthIndex))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    focus  = sqlite3_column_double(pStmt, 0);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

void UpsReader::getGainAtgcParams( uint32_t depthIndex,
                                uint32_t &afePowerSelectIDC,
                                float    &maxGainC,
                                float    &minGainC )
{
    sqlite3_stmt* pStmt;

    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "AfePowerSelectIDC, "
                                        "MaxGainC, "
                                        "MinGainC "
                                        "FROM GainAtgc WHERE DepthIndex = ?",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, depthIndex))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    afePowerSelectIDC  = sqlite3_column_int   (pStmt, 0);
    maxGainC           = sqlite3_column_double(pStmt, 1);
    minGainC           = sqlite3_column_double(pStmt, 2);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

void UpsReader::getAfeInputResistance( uint32_t afeResistSelectId,
                                       uint32_t &bitset)
{
    sqlite3_stmt* pStmt;

    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "BitSetting "
                                        "FROM AfeInputResistSelect WHERE ID = ?",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, afeResistSelectId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    bitset = sqlite3_column_int   (pStmt, 0);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

void UpsReader::getAfeGainModeParams( uint32_t gainSelectId,
                                      float &gainVal)
{
    sqlite3_stmt* pStmt;

    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT FixedGain FROM AfeGainMode WHERE ID = ?",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_OK !=  sqlite3_bind_int(pStmt, 1, gainSelectId))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }

    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    gainVal = sqlite3_column_double(pStmt, 0);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}


uint32_t UpsReader::getApiTableSize()
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT Count(*) FROM API",
                                  -1, &stmt, 0), SQLITE_OK, 0);

    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    uint32_t size = sqlite3_column_int(stmt, 0);

    return (size);
}

bool UpsReader::getUtpDataString(uint32_t id, char str[])
{
    bool retVal = false;
    Sqlite3Stmt stmt;
    uint32_t crcVal;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
            "SELECT Data, CRC FROM API WHERE ID = ?",
            -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, id), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    const char *strPtr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    crcVal = sqlite3_column_int(stmt, 1);
    strcpy( str, strPtr);
    uint32_t computedCrc = crc32_1byte(strPtr, strlen(strPtr));
    if (crcVal != computedCrc)
    {
        LOGE("CRC check failed for %d, expected = %08x, got = %08x", id, crcVal, computedCrc);
        return (false);
    }
    return (true);
}

uint32_t UpsReader::getTxApertureElement(uint32_t utpId )
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT TxApertureElement FROM UTPListing WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, utpId), SQLITE_OK, 0);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, 0);
    uint32_t txApertureElement = sqlite3_column_int(stmt, 0);
    return (txApertureElement);
}

bool UpsReader::getUtpIds(uint32_t imagingCaseId, uint32_t imagingMode, int32_t utpIds[3], float focalDepth, uint32_t halfcycle, bool isPWTDI)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT NumUTPs, UTPID1, UTPID2, UTPID3 FROM InputVector WHERE ID in "
                                  "(SELECT InputVectorID FROM ImagingCases WHERE ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    uint32_t numUtps = sqlite3_column_int(stmt, 0);

    switch (numUtps)
    {
        case 3:
            utpIds[2] = sqlite3_column_int(stmt, 3);

        case 2:
            utpIds[1] = sqlite3_column_int(stmt, 2);

        case 1:
            utpIds[0] = sqlite3_column_int(stmt, 1);
            break;

        default:
            LOGE("numUtps read from UPS (%d) for imagingCase %d is invalid",
                    numUtps, imagingCaseId);
            return (false);
            break;
    }
    if (imagingMode == IMAGING_MODE_COLOR)
    {
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                "SELECT UTPIDList from ColorFocalList WHERE ID = ?",
                -1, &stmt, 0), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_int(stmt, 1, utpIds[1]), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
        int32_t colorUtpIdList[32];
        int32_t blobSize = sqlite3_column_bytes(stmt, 0);
        if ( (blobSize <= 0) || (blobSize > 32*sizeof(uint32_t)))
        {
            LOGE("Invalid color focal list, blobSize = %d", blobSize);
            mErrorCount++;
            return (false);
        }
        memcpy(colorUtpIdList, sqlite3_column_blob(stmt, 0), blobSize);
        int32_t focalIndex = getColorFocalIndex(imagingCaseId, focalDepth);
        if ((focalIndex != -1) && (focalIndex < 32))
        {
            utpIds[1] = colorUtpIdList[focalIndex];
        }
        else
        {
            return (false);
        }
    }

    if (imagingMode == IMAGING_MODE_PW)
    {
        Sqlite3Stmt stmt;
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT NumUTPs, UTPID1, UTPID2, UTPID3 FROM InputVector WHERE ID in "
                                      "(SELECT InputVectorIDPW FROM ImagingCases WHERE ID = ?)",
                                      -1, &stmt, 0), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
        uint32_t numUtps = sqlite3_column_int(stmt, 0);
        utpIds[0] = sqlite3_column_int(stmt, 1);

        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT UTPIDList from PWCycleFocalList WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_int(stmt, 1, utpIds[0]), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
        int32_t pwUtpIdList[100];
        int32_t blobSize = sqlite3_column_bytes(stmt, 0);
        if ( (blobSize <= 0) || (blobSize > 100*sizeof(uint32_t)))
        {
            LOGE("Invalid PW focal list, blobSize = %d", blobSize);
            mErrorCount++;
            return (false);
        }

        memcpy(pwUtpIdList, sqlite3_column_blob(stmt, 0), blobSize);
        utpIds[0] = getPWCycleFocalIndex(imagingCaseId, focalDepth, halfcycle, isPWTDI);
    }

    if (imagingMode == IMAGING_MODE_CW)
    {
        Sqlite3Stmt stmt;
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT NumUTPs, UTPID1, UTPID2, UTPID3 FROM InputVector WHERE ID in "
                                      "(SELECT InputVectorIDCW FROM ImagingCases WHERE ID = ?)",
                                      -1, &stmt, 0), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
        uint32_t numUtps = sqlite3_column_int(stmt, 0);
        utpIds[0] = sqlite3_column_int(stmt, 1);

        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT UTPIDList from CWCycleFocalList WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_int(stmt, 1, utpIds[0]), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
        int32_t cwUtpIdList[10];
        int32_t blobSize = sqlite3_column_bytes(stmt, 0);
        if ( (blobSize <= 0) || (blobSize > 10*sizeof(uint32_t)))
        {
            LOGE("Invalid CW focal list, blobSize = %d", blobSize);
            mErrorCount++;
            return (false);
        }
        memcpy(cwUtpIdList, sqlite3_column_blob(stmt, 0), blobSize);
        utpIds[0] = getCwUtpIndex(imagingCaseId, focalDepth);

    }

    switch (numUtps)
    {
        case 3:
            if( (utpIds[0] == -1) || (utpIds[1] == -1) || (utpIds[2] == -1) )
            {
                return false;
            }
            break;

        case 2:
            if( (utpIds[0] == -1) || (utpIds[1] == -1) )
            {
                return false;
            }
            break;

        case 1:
            if (utpIds[0] == -1)
            {
                return false;
            }
            break;

            // default case is handled earlier
    }
    return (true);
}

bool UpsReader::getUtpIdsPW(uint32_t imagingCaseId, uint32_t imagingMode, int32_t utpIds[3], float focalDepth, uint32_t halfcycle, bool isPWTDI, uint32_t organId, float gateRange)
{
    if (imagingMode == IMAGING_MODE_PW)
    {
        Sqlite3Stmt stmt;
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT NumUTPs, UTPID1, UTPID2, UTPID3 FROM InputVector WHERE ID in "
                                      "(SELECT InputVectorIDPW FROM PWGateSize WHERE Organ = ? AND GateSizeMm = ?)",
                                      -1, &stmt, 0), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_double(stmt, 2, gateRange), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
        uint32_t numUtps = sqlite3_column_int(stmt, 0);
        utpIds[0] = sqlite3_column_int(stmt, 1);

        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT UTPIDList from PWCycleFocalList WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_bind_int(stmt, 1, utpIds[0]), SQLITE_OK, false);
        SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
        int32_t pwUtpIdList[100];
        int32_t blobSize = sqlite3_column_bytes(stmt, 0);
        if ( (blobSize <= 0) || (blobSize > 100*sizeof(uint32_t)))
        {
            LOGE("Invalid PW focal list, blobSize = %d", blobSize);
            mErrorCount++;
            return (false);
        }

        memcpy(pwUtpIdList, sqlite3_column_blob(stmt, 0), blobSize);
        utpIds[0] = getPWCycleFocalIndexLinear(imagingCaseId, pwUtpIdList, focalDepth, halfcycle, isPWTDI);
    }

    return (true);
}

bool UpsReader::getBLineDensity(uint32_t imagingCaseId, float &lineDensity )
{
    Sqlite3Stmt stmt;
    float radians2deg = (float)M_PI/180.0f;
    uint32_t organId;
    uint32_t organGlobalId;
    organId = this->getImagingOrgan(imagingCaseId);
    organGlobalId = this->getOrganGlobalId(organId);

    if (organGlobalId == BLADDER_GLOBAL_ID)
    {
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT RxPitchBbladder FROM Depths WHERE ID in "
                                      "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                      -1, &stmt, 0), SQLITE_OK, false);
    }
    else
    {
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT RxPitchB FROM Depths WHERE ID in "
                                      "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                      -1, &stmt, 0), SQLITE_OK, false);
    }
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    auto rxPitch = (float)sqlite3_column_double(stmt, 0);
    float txPitch = 4 * rxPitch;
    lineDensity = radians2deg / txPitch;
    return (true);
}

bool UpsReader::getBLineDensityLinear(uint32_t imagingCaseId, float &lineDensity )
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT RxPitchB FROM Depths WHERE ID in "
                                  "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    auto rxPitch = (float)sqlite3_column_double(stmt, 0);
    float txPitch = 4 * rxPitch;
    lineDensity = 1 / txPitch;
    return (true);
}

bool UpsReader::getColorLineDensityLinear(uint32_t imagingCaseId, float &lineDensity ) {
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT RxPitchBC FROM Depths WHERE ID in "
                                  "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    auto rxPitch = (float) sqlite3_column_double(stmt, 0);
    float txPitch = 4 * rxPitch;
    lineDensity = 1 / txPitch;
    return (true);
}

bool UpsReader::getColorLineDensity(uint32_t imagingCaseId, float &lineDensity )
{
    Sqlite3Stmt stmt;
    float radians2deg = (float)M_PI/180.0f;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT RxPitchBC FROM Depths WHERE ID in "
                                  "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    auto rxPitch = (float)sqlite3_column_double(stmt, 0);
    float txPitch = 4 * rxPitch;
    lineDensity = radians2deg / txPitch;
    return (true);
}

bool UpsReader::getMLineDensity(uint32_t imagingCaseId, float &lineDensity )
{
    Sqlite3Stmt stmt;
    float radians2deg = (float)M_PI/180.0f;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT RxPitchBM FROM Depths WHERE ID in "
                                  "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    auto rxPitch = (float)sqlite3_column_double(stmt, 0);
    float txPitch = 4 * rxPitch;
    lineDensity = radians2deg / txPitch;
    return (true);
}

bool UpsReader::getMLineDensityLinear(uint32_t imagingCaseId, float &lineDensity )
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT RxPitchBM FROM Depths WHERE ID in "
                                  "(SELECT Depth FROM ImagingCases WHERE ID = ?)",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    auto rxPitch = (float)sqlite3_column_double(stmt, 0);
    float txPitch = 4 * rxPitch;
    lineDensity = 1 / txPitch;
    return (true);
}

bool UpsReader::getDaGainConstants( float &gainOffset, float &gainRange )
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT DAGainOffset, DAGainRange FROM DAECG",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    gainOffset = (float)sqlite3_column_double(stmt, 0);
    gainRange = (float)sqlite3_column_double(stmt, 1);

    return (true);
}

bool UpsReader::getEcgGainConstants( bool isInternal, float &gainOffset, float &gainRange )
{
    Sqlite3Stmt stmt;

    if (isInternal)
    {
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT GainOffsetINT, GainRangeINT FROM DAECG",
                                      -1, &stmt, 0), SQLITE_OK, false);
    }
    else
    {
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT GainOffsetEXT, GainRangeEXT FROM DAECG",
                                      -1, &stmt, 0), SQLITE_OK, false);
    }
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    gainOffset = (float)sqlite3_column_double(stmt, 0);
    gainRange = (float)sqlite3_column_double(stmt, 1);

    return (true);
}

int32_t UpsReader::getColorFocalIndex(uint32_t imagingCaseId, float focalDepthMm)
{
    Sqlite3Stmt stmt;
    int32_t focalIndex = -1;
    float startFocus;
    float stopFocus;
    float focalInterval;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT ColorStartFocusMm, ColorStopFocusMm, ColorIntervalFocusMm FROM Globals",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, -1);

    startFocus = (float)sqlite3_column_double(stmt, 0);
    stopFocus = (float)sqlite3_column_double(stmt, 1);
    focalInterval = (float)sqlite3_column_double(stmt, 2);

    if (focalDepthMm < startFocus)
    {
        focalDepthMm = startFocus;
    }

    if (focalDepthMm > stopFocus)
    {
        focalDepthMm = stopFocus;
    }

    if ((focalInterval <= 0.0f) || (focalInterval > (stopFocus - startFocus)))
    {
        LOGE("invalid color focal interval (%f)", focalInterval);
        return(-1);
    }
    focalIndex = (int32_t)round((focalDepthMm - startFocus) / focalInterval);
    return (focalIndex);
}

int32_t UpsReader::getPWCycleFocalIndex(uint32_t imagingCaseId, float focalDepthMm, uint32_t halfcycle, bool isTDI)
{
    int32_t focalIndex = 6;
    float focalList [] = {20, 60, 80, 100, 140, 180, 220};
    uint32_t utpList [] = {121,125,127,129,133,137,141};
    if (isTDI)
    {
        utpList [0] = 296;
        for (uint32_t i=1;i<=6;i++)
            utpList [i] = i+303;
    }

    while (focalIndex > 0)
    {
        if (focalDepthMm < focalList[focalIndex])
        {
            focalIndex--;
        }
        else
        {
            break;
        }
    }
    return (utpList[focalIndex]);
}

int32_t UpsReader::getPWCycleFocalIndexLinear(uint32_t imagingCaseId, int32_t utpList[100], float focalDepthMm, uint32_t halfcycle, bool isTDI)
{
    int32_t focalIndex = 8;
    float focalList [] = {10, 20, 30, 40, 50, 60, 70, 80, 90};

    while (focalIndex > 0)
    {
        if (focalDepthMm < focalList[focalIndex])
        {
            focalIndex--;
        }
        else
        {
            break;
        }
    }
    return (utpList[focalIndex]);
}

int32_t UpsReader::getCwUtpIndex(uint32_t imagingCaseId, float focalDepthMm)
{
    int32_t focalIndex = 3;
    float focalList [] = {40, 80, 140, 200};
    uint32_t utpList [] = {262, 264, 267, 270};
    while (focalIndex > 0)
    {
        if (focalDepthMm >=60 and focalDepthMm <=80)
        {
            focalIndex = 1;
            break;
        }
        else if (focalDepthMm < focalList[focalIndex])
        {
            focalIndex--;
        }
        else
        {
            break;
        }
    }
    return (utpList[focalIndex]);
}

bool UpsReader::getReverbTestParams( float& lowLimit,
				     float& highLimit,
				     uint32_t txWaveform[16],
				     uint32_t& hvps1,
				     uint32_t& hvps2,
				     uint32_t& txrxt,
				     uint32_t& afetr,
				     uint32_t& samples )
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT "
				  "ReverbLowLimit, "    // 0
				  "ReverbHighLimit, "   // 1
				  "ReverbTxWaveform, "  // 2
				  "ReverbHVPS1, "       // 3
				  "ReverbHVPS2, "       // 4
				  "ReverbTXRXT, "       // 5
				  "ReverbAFETR, "       // 6
				  "ReverbSamples "      // 7
				  "FROM ReverbTest",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    lowLimit = (float)sqlite3_column_double(stmt, 0);
    highLimit = (float)sqlite3_column_double(stmt, 1);
    hvps1 = (uint32_t)sqlite3_column_int(stmt, 3);
    hvps2 = (uint32_t)sqlite3_column_int(stmt, 4);
    txrxt = (uint32_t)sqlite3_column_int(stmt, 5);
    afetr = (uint32_t)sqlite3_column_int(stmt, 6);
    samples = (uint32_t)sqlite3_column_int(stmt, 7);

    int blobSize = sqlite3_column_bytes(stmt, 2);
    if (0 == blobSize)
    {
        mErrorCount++;
	    return (false);
    }
    memcpy(txWaveform, sqlite3_column_blob(stmt, 2), blobSize);
    return (true);
}

// PW related methods
//------------------------------------------------------------------------------------------
float UpsReader::getGateSizeMm(uint32_t gateIndex)
{
    Sqlite3Stmt stmt;
    float gateSizeMm;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT GateSizeMm FROM PWGateSize WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, gateIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    gateSizeMm = (float)sqlite3_column_double(stmt, 0);
    return (gateSizeMm);
}

uint32_t UpsReader::getGateSizeSamples(uint32_t gateIndex)
{
    Sqlite3Stmt stmt;
    uint32_t gateSamples;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT GateSamples FROM PWGateSize WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, gateIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    gateSamples = sqlite3_column_int(stmt, 0);
    return (gateSamples);
}

bool UpsReader::getMaxGainDbToLinearLUT(float &maxGain)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT MaxGainDb from GainDbToLinearLUT",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    maxGain = (float)sqlite3_column_double(stmt, 0);
    return (true);
}

void UpsReader::getPwPrfList(float prfList[], uint32_t& numValidPrfs, bool isTDI)
{
    sqlite3_stmt* pStmt;
    int rc;

    numValidPrfs = 0;
    if (isTDI) {
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT PrfHz FROM TDIPRFs",
                                -1, &pStmt, 0);
    }
    else {
        rc = sqlite3_prepare_v2(mDb_ptr,
                                "SELECT PrfHz FROM PWPRFs",
                                -1, &pStmt, 0);
    }
    if (SQLITE_OK != rc)
    {
        LOGE("%s: prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    unsigned prfCount = 0;
    for ( ; (sqlite3_step(pStmt) == SQLITE_ROW) && (prfCount < MAX_PRF_SETTINGS); prfCount++ )
    {
        prfList[prfCount] = sqlite3_column_double(pStmt, 0);
    }
    LOGD("%s read %d PRF settings", __func__, prfCount);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
    }
    numValidPrfs = prfCount;
    return;
}

uint32_t UpsReader::getNumPwSamplesPerFrame(uint32_t prfIndex, bool isTDI)
{
    Sqlite3Stmt stmt;
    uint32_t numPwSamples;

    if (isTDI)
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT NumPwSamplesPerFrame FROM TDIPRFs WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, -1);
    else
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT NumPwSamplesPerFrame FROM PWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    numPwSamples = sqlite3_column_int(stmt, 0);
    return (numPwSamples);
}

uint32_t UpsReader::getAfeClksPerPwFft(uint32_t prfIndex, bool isTDI)
{
    Sqlite3Stmt stmt;
    uint32_t AfeClksPerPwFft;

    if (isTDI)
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT AfeClksPerPwFft FROM TDIPRFs WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, -1);
    else
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT AfeClksPerPwFft FROM PWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    AfeClksPerPwFft = sqlite3_column_int(stmt, 0);
    return (AfeClksPerPwFft);
}

uint32_t UpsReader::getNumSamplesFft(uint32_t prfIndex, bool isTDI)
{
    Sqlite3Stmt stmt;
    uint32_t NumSamplesFft;

    if (isTDI)
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT NumSamplesFft FROM TDIPRFs WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, -1);
    else
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT NumSamplesFft FROM PWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    NumSamplesFft = sqlite3_column_int(stmt, 0);
    return (NumSamplesFft);
}

uint32_t UpsReader::getAfeClksPerPwSample(uint32_t prfIndex, bool isTDI)
{
    Sqlite3Stmt stmt;
    uint32_t numPwSamples;

    if (isTDI)
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT AfeClksPerPwSample FROM TDIPRFs WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, -1);
    else
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT AfeClksPerPwSample FROM PWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    numPwSamples = sqlite3_column_int(stmt, 0);
    return (numPwSamples);
}

void UpsReader::getPwWaveformInfo( uint32_t &txApertureElement,
                                   float    &centerFrequency, bool isTDI)
{
    sqlite3_stmt* pStmt;
    uint32_t txid;
    if (isTDI)
        txid=14;
    else
        txid=2;
    // get rest of requested stuff
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "TxApertureElement, "
                                        "CenterFrequency "
                                        "FROM TxWaveforms WHERE ID = ?",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_OK != sqlite3_bind_int(pStmt, 1, txid))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    txApertureElement  = sqlite3_column_int   (pStmt, 0);
    centerFrequency    = sqlite3_column_double(pStmt, 1);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

void UpsReader::getPwWaveformInfoLinear(uint32_t organId, float pwFocus,  float gateRange, uint32_t &txApertureElement, float    &centerFrequency, uint32_t &halfCycles)
{
    sqlite3_stmt* pStmt;
    if (pwFocus<10.0)
        pwFocus=10.0;

    if (pwFocus>90.0)
        pwFocus=90.0;
    switch (organId)
    {
        case 0:

            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "TxApertureElement, "
                                                "CenterFrequency, NumHalfCycles "
                                                "FROM TxWaveforms WHERE Organ = 0 and Focus = ? and GateRange = ?",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
        case 1:

            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "TxApertureElement, "
                                                "CenterFrequency, NumHalfCycles "
                                                "FROM TxWaveforms WHERE Organ = 1 and Focus = ? and GateRange = ?",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
        case 2:

            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "TxApertureElement, "
                                                "CenterFrequency, NumHalfCycles "
                                                "FROM TxWaveforms WHERE Organ = 2 and Focus = ? and GateRange = ?",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
    }

    /*
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "TxApertureElement, "
                                        "CenterFrequency "
                                        "FROM TxWaveforms WHERE Organ = 0 and Focus = ?",
                                        -1, &pStmt, 0))
    {
        LOGE("%s prepare failed", __func__);
        mErrorCount++;
        return;
    }
     */
    if (SQLITE_OK != sqlite3_bind_double(pStmt, 1, pwFocus))
    {
        LOGE("bind 1 failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_OK != sqlite3_bind_double(pStmt, 2, gateRange))
    {
        LOGE("bind 2 failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }

    txApertureElement  = sqlite3_column_int   (pStmt, 0);
    centerFrequency    = sqlite3_column_double(pStmt, 1);
    halfCycles         = sqlite3_column_int(pStmt, 2);
    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}


void UpsReader::getPwCenterFrequencyLinear(uint32_t organId, float pwFocus,  float gateRange, float &centerFrequency)
{
    sqlite3_stmt* pStmt;
    if (pwFocus<10.0)
        pwFocus=10.0;

    if (pwFocus>90.0)
        pwFocus=90.0;
    switch (organId)
    {
        case 0:

            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "CenterFrequency "
                                                "FROM TxWaveforms WHERE Organ = 0 and Focus = ? and GateRange = ?",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
        case 1:

            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "CenterFrequency "
                                                "FROM TxWaveforms WHERE Organ = 1 and Focus = ? and GateRange = ?",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
        case 2:

            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "CenterFrequency "
                                                "FROM TxWaveforms WHERE Organ = 2 and Focus = ? and GateRange = ?",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
    }

    if (SQLITE_OK != sqlite3_bind_double(pStmt, 1, pwFocus))
    {
        LOGE("bind 1 failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_OK != sqlite3_bind_double(pStmt, 2, gateRange))
    {
        LOGE("bind 2 failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("%s step failed", __func__);
        mErrorCount++;
    }
    centerFrequency    = sqlite3_column_double(pStmt, 0);
    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
    return;
}

uint32_t UpsReader::getPwWallFilterCoefficients(uint32_t wallFilterId, float wallFilterCoeff[], bool isTDI ) {
    Sqlite3Stmt stmt;
    if (isTDI)
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT WallFilterCoefficients,WallFilterTap FROM TDIWallFilter WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, false);
    else
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT WallFilterCoefficients,WallFilterTap FROM PWWallFilter WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, wallFilterId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    int32_t blobSize = sqlite3_column_bytes(stmt, 0);
    if ( (blobSize <= 0) || (blobSize > 8192) || ((blobSize % 4) != 0))
    {
        LOGE("UpsReader::getDaCoeff(), invalid blob size : %d", blobSize);
        mErrorCount++;
        return (0);
    }
    uint32_t numCoeff = sqlite3_column_int(stmt, 1);

    const double* vptr = (const double*)sqlite3_column_blob(stmt, 0);
    for (uint32_t ii=0; ii<numCoeff; ii++) {
        wallFilterCoeff[ii] = (float)(*(vptr+ii));
    }

    return (numCoeff);
}

bool UpsReader::getPwFftSizeRate(uint32_t &fftSize, uint32_t &fftOutputRateAfeClks)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT FftSize, FftOutputRateAfeClks FROM PWFft",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftSize = sqlite3_column_int(stmt, 0);
    fftOutputRateAfeClks = sqlite3_column_int(stmt, 1);

    return (true);
}

bool UpsReader::getPwFftAverageNum(uint32_t index, uint32_t &fftAverageNum, uint32_t &fftSmoothingNum)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT FftAverageNum, FftSmoothingNum FROM PWFft WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, index), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftAverageNum = sqlite3_column_int(stmt, 0);
    fftSmoothingNum = sqlite3_column_int(stmt, 1);
    return (true);
}

bool UpsReader::getPwFftParams(uint32_t prfIndex, uint32_t &fftSize, uint32_t &fftOutputRateAfeClks, uint32_t &fftAverageNum, uint32_t &fftWindowType, uint32_t &fftWindowSize, float fftWindowCoeff[], bool isTDI )
{
    fftWindowSize = getNumSamplesFft(prfIndex, isTDI);
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT FftSize, FftOutputRateAfeClks, FftAverageNum, FftWindowType, FftWindowCoefficients FROM PWFft",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftSize = sqlite3_column_int(stmt, 0);
    fftOutputRateAfeClks = sqlite3_column_int(stmt, 1);
    fftAverageNum = sqlite3_column_int(stmt, 2);
    fftWindowType = sqlite3_column_int(stmt, 3);

    const double* vptr = (const double*)sqlite3_column_blob(stmt, 4);
    for (uint32_t ii=0; ii<fftSize; ii++) {
        fftWindowCoeff[ii] = (float)(*(vptr+ii));
    }

    return (true);
}

bool UpsReader::getFftWindow(uint32_t prfIndex, uint32_t &fftWindowSize, float fftWindowCoeff[], bool isTDI )
{
    Sqlite3Stmt stmt;
    if (isTDI)
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                     "SELECT NumSamplesFft, FftWindowCoefficients FROM TDIPRFs WHERE ID = ?",
                                     -1, &stmt, 0), SQLITE_OK, false);
    else
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT NumSamplesFft, FftWindowCoefficients FROM PWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftWindowSize = sqlite3_column_int(stmt, 0);
    const double* vptr = (const double*)sqlite3_column_blob(stmt, 1);
    for (uint32_t ii=0; ii<fftWindowSize; ii++) {
        fftWindowCoeff[ii] = (float)(*(vptr+ii));
    }

    return (true);
}

//------------------------------------------------------------------------------------------
void UpsReader::getPwBpfLut(uint32_t imagingCaseId,
                            uint32_t &oscale,
                            uint32_t *coeffsI,
                            uint32_t *coeffsQ, bool isTDI)

{
    sqlite3_stmt* pStmt;
    uint32_t bpfIndex;
    if (isTDI)
        bpfIndex=69;
    else
        bpfIndex=59;
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "Scale, CoeffsI, CoeffsQ "
                                        "from BandPassFilters WHERE ID = ?",
                                        -1,
                                        &pStmt,
                                        0))

    {
        LOGE("prepare failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_OK != sqlite3_bind_int(pStmt, 1, bpfIndex))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("sqlite3_step failed for %s", __func__);
        mErrorCount++;
        return;
    }
    oscale = sqlite3_column_int(pStmt, 0);
    uint32_t lutSize = sqlite3_column_bytes(pStmt, 1);
    memcpy( coeffsI, sqlite3_column_blob(pStmt, 1), lutSize);

    lutSize = sqlite3_column_bytes(pStmt, 2);
    memcpy( coeffsQ, sqlite3_column_blob(pStmt, 2), lutSize);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
}

void UpsReader::getPwBpfLutLinear(uint32_t organId, float GateRange, uint32_t &oscale, uint32_t *coeffsI, uint32_t *coeffsQ)

{
    sqlite3_stmt* pStmt;

    switch (organId)
    {
        case 0:
            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "Scale, CoeffsI, CoeffsQ "
                                                "from BandPassFilters WHERE ID IN (SELECT BPFID from PWGateSize WHERE Organ=0 and GateSizeMm = ?)",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
        case 1:
            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "Scale, CoeffsI, CoeffsQ "
                                                "from BandPassFilters WHERE ID IN (SELECT BPFID from PWGateSize WHERE Organ=1 and GateSizeMm = ?)",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
        case 2:
            if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                                "SELECT "
                                                "Scale, CoeffsI, CoeffsQ "
                                                "from BandPassFilters WHERE ID IN (SELECT BPFID from PWGateSize WHERE Organ=2 and GateSizeMm = ?)",
                                                -1, &pStmt, 0))
            {
                LOGE("%s prepare failed", __func__);
                mErrorCount++;
                return;
            }
            break;
    }

    if (SQLITE_OK != sqlite3_bind_double(pStmt, 1, GateRange))
    {
        LOGE("bind failed for %s", __func__);
        mErrorCount++;
        return;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("sqlite3_step failed for %s", __func__);
        mErrorCount++;
        return;
    }
    oscale = sqlite3_column_int(pStmt, 0);
    uint32_t lutSize = sqlite3_column_bytes(pStmt, 1);
    memcpy( coeffsI, sqlite3_column_blob(pStmt, 1), lutSize);

    lutSize = sqlite3_column_bytes(pStmt, 2);
    memcpy( coeffsQ, sqlite3_column_blob(pStmt, 2), lutSize);

    if (SQLITE_OK != sqlite3_finalize(pStmt))
    {
        mErrorCount++;
        return;
    }
}
//------------------------------------------------------------------------------------------
bool UpsReader::getPwGateSamples(uint32_t gateIndex, uint32_t &gateSamples)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT GateSamples FROM PWGateSize WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, gateIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    gateSamples = sqlite3_column_int(stmt, 0);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getPwGateRange(uint32_t gateIndex, float &gateRange)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT GateSizeMm FROM PWGateSize WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, gateIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    gateRange = sqlite3_column_double(stmt, 0);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getPWSteeringAngle(uint32_t steeringAngleIndex, float &steeringAngleDegrees)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT AngleDegrees FROM PWSteeringAngle WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, steeringAngleIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    steeringAngleDegrees = sqlite3_column_double(stmt, 0);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getPwBaselineShiftFraction(uint32_t baselineShiftIndex, float &baselineShiftFraction)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT BaselineshiftFraction FROM PWBaselineShift WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, baselineShiftIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    baselineShiftFraction = sqlite3_column_double(stmt, 0);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getCwBaselineShiftFraction(uint32_t baselineShiftIndex, float &baselineShiftFraction)
{
    getPwBaselineShiftFraction(baselineShiftIndex, baselineShiftFraction);

    // x 1/2 to baselineShiftFraction
    baselineShiftFraction *= 0.5f;

    return (true);
}

//------------------------------------------------------------------------------------------
uint32_t UpsReader::getPwHilbertCoefficients(uint32_t filterId, float hilbertFilterCoeff[])
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT AudioHilbertCoefficients, AudioHilbertFilterTap "
                                  "FROM PWAudio WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, filterId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    int32_t blobSize = sqlite3_column_bytes(stmt, 0);
    if ( (blobSize <= 0) || (blobSize > 8192) || ((blobSize % 4) != 0))
    {
        LOGE("UpsReader::getPwHilbertCoefficients(), invalid blob size : %d", blobSize);
        mErrorCount++;
        return (0);
    }
    uint32_t numCoeff = sqlite3_column_int(stmt, 1);

    const double* vptr = (const double*)sqlite3_column_blob(stmt, 0);
    for (uint32_t ii=0; ii<numCoeff; ii++) {
        hilbertFilterCoeff[ii] = (float)(*(vptr+ii));
    }

    return (numCoeff);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getPwAudioHilbertLpfParams(uint32_t filterId, uint32_t &audioLpfTap,
                                           float &audioLpfCutoff)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT AudioHilbertLpfTap, AudioHilbertLpfCutoff "
                                  "FROM PWAudio WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, filterId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    audioLpfTap = sqlite3_column_int(stmt, 0);
    audioLpfCutoff = sqlite3_column_double(stmt, 1);

    return (true);
}

//------------------------------------------------------------------------------------------
uint32_t UpsReader::getPwAudioHilbertLpfCoeffs(uint32_t filterId, float filterCoeffs[])
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT AudioHilbertLpfCoefficients, AudioHilbertLpfTap "
                                  "FROM PWAudio WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, filterId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    int32_t blobSize = sqlite3_column_bytes(stmt, 0);
    if ( (blobSize <= 0) || (blobSize > 8192) || ((blobSize % 4) != 0))
    {
        LOGE("UpsReader::getPwAudioHilbertLpfCoeffs(), invalid blob size : %d", blobSize);
        mErrorCount++;
        return (0);
    }
    uint32_t numCoeff = sqlite3_column_int(stmt, 1);

    const double* vptr = (const double*)sqlite3_column_blob(stmt, 0);
    for (uint32_t ii=0; ii<numCoeff; ii++) {
        filterCoeffs[ii] = (float)(*(vptr+ii));
    }

    return (numCoeff);
}

uint32_t UpsReader::getDopplerCompressionCoeff(uint32_t compressionId, float compressionCoeff[]) {
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT CompressionValues FROM Compression WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, compressionId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    int32_t blobSize = sqlite3_column_bytes(stmt, 0);

    uint32_t numCoeff = 256;

    const float* vptr = (const float*)sqlite3_column_blob(stmt, 0);
    for (uint32_t ii=0; ii<numCoeff; ii++) {
        compressionCoeff[ii] = (float)(*(vptr+ii));
    }
    return (numCoeff);
}

// CW related methods
//------------------------------------------------------------------------------------------
uint32_t UpsReader::getNumCwSamplesPerFrame(uint32_t prfIndex)
{
    Sqlite3Stmt stmt;
    uint32_t numCwSamples;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT NumCwSamplesPerFrame FROM CWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    numCwSamples = sqlite3_column_int(stmt, 0);
    return (numCwSamples);
}

float UpsReader::getCwFocusOffset()
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT CWFocusOffsetMm FROM Globals",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    float focusoffset = (float)sqlite3_column_double(stmt, 0);
    return (focusoffset);
}

uint32_t UpsReader::getCwTransmitAperture(uint32_t utpId)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT TxApertureElement FROM UTPListing WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, utpId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    uint32_t txap = sqlite3_column_int(stmt, 0);
    return (txap);
}

bool UpsReader::getCwFftSizeRate(uint32_t &fftSize, uint32_t &fftOutputRateCwClks)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT FftSize, FftOutputRateCwClks FROM CWFft",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftSize = sqlite3_column_int(stmt, 0);
    fftOutputRateCwClks = sqlite3_column_int(stmt, 1);

    return (true);
}

uint32_t UpsReader::getCwClksPerCwSample(uint32_t prfIndex)
{
    Sqlite3Stmt stmt;
    uint32_t numCwSamples;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT CwClksPerCwSample FROM CWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, -1);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    numCwSamples = sqlite3_column_int(stmt, 0);
    return (numCwSamples);
}

bool UpsReader::getCwFftParams(uint32_t prfIndex, uint32_t &fftSize, uint32_t &fftOutputRateCwClks, uint32_t &fftAverageNum, uint32_t &fftWindowType, float fftWindowCoeff[])
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT FftSize, FftOutputRateCwClks, FftAverageNum, FftWindowType, FftWindowCoefficients FROM CWFft",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftSize = sqlite3_column_int(stmt, 0);
    fftOutputRateCwClks = sqlite3_column_int(stmt, 1);
    fftAverageNum = sqlite3_column_int(stmt, 2);
    fftWindowType = sqlite3_column_int(stmt, 3);

    const double* vptr = (const double*)sqlite3_column_blob(stmt, 4);
    for (uint32_t ii=0; ii<fftSize; ii++) {
        fftWindowCoeff[ii] = (float)(*(vptr+ii));
    }

    return (true);
}

bool UpsReader::getCwFftAverageNum(uint32_t index, uint32_t &fftAverageNum, uint32_t & fftSmoothingNum)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT FftAverageNum, FftSmoothingNum FROM CWFft WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, index), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftAverageNum = sqlite3_column_int(stmt, 0);
    fftSmoothingNum = sqlite3_column_int(stmt, 1);
    return (true);
}

bool UpsReader::getCwFftWindow(uint32_t prfIndex, uint32_t &fftWindowSize, float fftWindowCoeff[] )
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT NumSamplesFft, FftWindowCoefficients FROM CWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    fftWindowSize = sqlite3_column_int(stmt, 0);
    const double* vptr = (const double*)sqlite3_column_blob(stmt, 1);
    for (uint32_t ii=0; ii<fftWindowSize; ii++) {
        fftWindowCoeff[ii] = (float)(*(vptr+ii));
    }

    return (true);
}

uint32_t UpsReader::getCwDownsampleCoeffs(uint32_t prfIndex, float downsampleCoeff[])
{
    Sqlite3Stmt stmt;

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT DownsampleCwFilterTap, DownsampleCwCoefficients FROM CWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    uint32_t tap = sqlite3_column_int(stmt, 0);
    const double* vptr = (const double*)sqlite3_column_blob(stmt, 1);
    for (uint32_t ii=0; ii<tap; ii++) {
        downsampleCoeff[ii] = (float)(*(vptr+ii));
    }
    return (tap);
}

bool UpsReader::getCwDecFactorSamples(uint32_t prfIndex, uint32_t &decimationFactor,  uint32_t &samples)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT DecimationFactor, DecimatedSample FROM CWPRFs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, prfIndex), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    decimationFactor = sqlite3_column_int(stmt, 0);
    samples = sqlite3_column_int(stmt, 1);

    return (true);
}

void UpsReader::getCwPrfList(float prfList[], uint32_t& numValidPrfs)
{
    sqlite3_stmt* pStmt;
    int rc;

    numValidPrfs = 0;
    rc = sqlite3_prepare_v2(mDb_ptr,
                            "SELECT PrfHz FROM CWPRFs",
                            -1, &pStmt, 0);
    if (SQLITE_OK != rc)
    {
        LOGE("%s: prepare failed, return code is %d\n", __func__, rc);
        mErrorCount++;
        return;
    }

    unsigned prfCount = 0;
    for ( ; (sqlite3_step(pStmt) == SQLITE_ROW) && (prfCount < MAX_PRF_SETTINGS); prfCount++ )
    {
        prfList[prfCount] = sqlite3_column_double(pStmt, 0);
    }
    LOGD("%s read %d PRF settings", __func__, prfCount);
    rc = sqlite3_finalize(pStmt);
    if (SQLITE_OK != rc)
    {
        mErrorCount++;
    }
    numValidPrfs = prfCount;
    return;
}
//------------------------------------------------------------------------------------------
bool UpsReader::getCWPeakThresholdParams(uint32_t organId, float &refGainDbCW, float &peakThresholdCW)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT RefGainDbCW, PeakThresholdCW FROM Organs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    refGainDbCW = sqlite3_column_double(stmt, 0);
    peakThresholdCW = sqlite3_column_double(stmt, 1);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getPWPeakThresholdParams(uint32_t organId, float &refGainDbPW, float &peakThresholdPW)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT RefGainDbPW, PeakThresholdPW FROM Organs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    refGainDbPW = sqlite3_column_double(stmt, 0);
    peakThresholdPW = sqlite3_column_double(stmt, 1);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getCWGainParams(uint32_t organId, float &defaultGainCW, float &defaultGainRangeCW)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT DefaultOffsetCW, DefaultGainRangeCW FROM Organs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    defaultGainCW = sqlite3_column_double(stmt, 0);
    defaultGainRangeCW = sqlite3_column_double(stmt, 1);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getCWGainAudioParams(uint32_t organId, float &defaultGainAudioCW, float &defaultGainAudioRangeCW)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT DefaultOffsetAudioCW, DefaultGainAudioRangeCW FROM Organs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    defaultGainAudioCW = sqlite3_column_double(stmt, 0);
    defaultGainAudioRangeCW = sqlite3_column_double(stmt, 1);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getPWGainParams(uint32_t organId, float &defaultGainPW, float &defaultGainRangePW, bool isTDI)
{
    Sqlite3Stmt stmt;
    if (isTDI)
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT DefaultOffsetTDI, DefaultGainRangeTDI FROM Organs WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, false);
    else
        SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                      "SELECT DefaultOffsetPW, DefaultGainRangePW FROM Organs WHERE ID = ?",
                                      -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    defaultGainPW = sqlite3_column_double(stmt, 0);
    defaultGainRangePW = sqlite3_column_double(stmt, 1);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getPWGainAudioParams(uint32_t organId, float &defaultGainAudioPW, float &defaultGainAudioRangePW)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT DefaultOffsetAudioPW, DefaultGainAudioRangePW FROM Organs WHERE ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, organId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    defaultGainAudioPW = sqlite3_column_double(stmt, 0);
    defaultGainAudioRangePW = sqlite3_column_double(stmt, 1);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getTint(float &r, float &g, float &b)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT fractionR, fractionG, fractionB "
                                  "FROM TintMap",
                                  -1, &stmt, 0), SQLITE_OK, false);

    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);

    r = sqlite3_column_double(stmt, 0);
    g = sqlite3_column_double(stmt, 1);
    b = sqlite3_column_double(stmt, 2);

    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getSCParams(uint32_t imagingCaseId, uint32_t &numViews, float &steeringAngle, uint32_t &crossWidth)
{
    Sqlite3Stmt stmt;
    if (imagingCaseId>=1042 and imagingCaseId<=1047)
        imagingCaseId=imagingCaseId-21;
    const char *query =
            "SELECT NumViews, SteeringAngle, CrossWidth FROM SpatialCompounding WHERE ID IN (SELECT SCIndex from Organs WHERE ID IN (SELECT Organ FROM ImagingCases WHERE ID = ?))";

    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr, query, -1, &stmt, nullptr), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, imagingCaseId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    if (imagingCaseId>=2000)
        numViews=1;
    else
        numViews = sqlite3_column_int(stmt, 0);
    steeringAngle = sqlite3_column_double(stmt, 1);
    crossWidth = sqlite3_column_int(stmt, 2);

    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getExtraRX(uint32_t depthId, uint32_t& extraRX)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT ExtraRX from Depths where ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, depthId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    extraRX = sqlite3_column_int(stmt, 0);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getHvOffset(uint32_t depthId, uint32_t& hvOffset)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT HvOffset from Depths where ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, depthId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    hvOffset = sqlite3_column_int(stmt, 0);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getZeroDGC(uint32_t depthId, float& zeroDGC)
{
    Sqlite3Stmt stmt;
    SQLITE_TRY(sqlite3_prepare_v2(mDb_ptr,
                                  "SELECT ZeroDGCmm from Depths where ID = ?",
                                  -1, &stmt, 0), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_bind_int(stmt, 1, depthId), SQLITE_OK, false);
    SQLITE_TRY(sqlite3_step(stmt), SQLITE_ROW, false);
    zeroDGC = sqlite3_column_double(stmt, 0);
    return (true);
}

//------------------------------------------------------------------------------------------
bool UpsReader::getCPDParams(float& cpdCompressDynamicRangeDb, float& cpdCompressPivotPtIn, float& cpdCompressPivotPtOut)
{
    sqlite3_stmt* pStmt;
    if (SQLITE_OK != sqlite3_prepare_v2(mDb_ptr,
                                        "SELECT "
                                        "CPDCompressDynamicRangeDb, "
                                        "CPDCompressPivotPtIn, "
                                        "CPDCompressPivotPtOut "
                                        "from CPDCompression",
                                        -1,
                                        &pStmt,
                                        0))
    {
        LOGE("prepare failed for %s", __func__);
        mErrorCount++;
    }
    if (SQLITE_ROW != sqlite3_step(pStmt))
    {
        LOGE("sqlite3_step failed for %s", __func__);
        mErrorCount++;
    }
    cpdCompressDynamicRangeDb = sqlite3_column_double(pStmt, 0);
    cpdCompressPivotPtIn      = sqlite3_column_double(pStmt, 1);
    cpdCompressPivotPtOut     = sqlite3_column_double   (pStmt, 2);
    return (true);
}
