//
// Copyright 2016 EchoNous, Inc.
//

#include <stdio.h>
#include <sys/param.h>
#include <ThorUtils.h>
#include <ThorConstants.h>
#include <BitfieldMacros.h>

#define ENABLE_DAECG 1
//-----------------------------------------------------------------------------
void printBuffer32(uint32_t* bufPtr, uint32_t bufSize, size_t refAddr)
{
    const uint32_t  numElements = 8;  // Elements per row
    uint32_t        index;
    unsigned        pos;

    for (index = 0; index < bufSize; index += numElements)
    {
        printf("0x%016X: ", (uint32_t)(refAddr + index*sizeof(uint32_t)));

        for (pos = 0; pos < MIN(numElements, (bufSize - index)); pos++)
        {
            printf("0x%08X  ", *(bufPtr + index + pos));
        }
        printf("\n");
    }
}

//-----------------------------------------------------------------------------
void printBuffer32(uint32_t* bufPtr, uint32_t bufSize)
{
    printBuffer32(bufPtr, bufSize, (size_t) bufPtr);
}

//-----------------------------------------------------------------------------
void logBuffer32(uint32_t* bufPtr, uint32_t bufSize, size_t refAddr)
{
    const uint32_t  numElements = 8;  // Elements per row
    const uint32_t  elementSize = 12; // See note below
    const uint32_t  addrSize = 20;    // 16 hex characters plus "0x" and ": "
    uint32_t        index;
    unsigned        pos;
    char            buffer[elementSize * numElements + addrSize + 1];

    for (index = 0; index < bufSize; index += numElements)
    {
        sprintf(buffer, "0x%016X: ", (uint32_t)(refAddr + index*sizeof(uint32_t)));

        for (pos = 0; pos < MIN(numElements, (bufSize - index)); pos++)
        {
            // elementSize is determined from the format string in sprintf:
            // "0x" => 2
            //         +
            // %08X => 8
            //         +
            // "  " => 2
            // ----------
            // Total = 12
            sprintf(buffer + addrSize + (pos * elementSize),
                    "0x%08X  ",
                    *(bufPtr + index + pos));
        }
        ALOGD("%s", buffer);
    }
}

//-----------------------------------------------------------------------------
void logBuffer32(uint32_t* bufPtr, uint32_t bufSize)
{
    logBuffer32(bufPtr, bufSize, (size_t) bufPtr);
}

//-----------------------------------------------------------------------------
uint32_t getDauDataTypes(uint32_t imagingMode, bool isDaOn, bool isEcgOn, bool isUsOn)
{
    uint32_t    dataTypes = 0;
    uint32_t    mask = 0;

    if (isUsOn) {
        switch (imagingMode)
        {
            case IMAGING_MODE_B:
                mask = BIT(DAU_DATA_TYPE_B);
                if (isDaOn)
                    mask = mask | BIT(DAU_DATA_TYPE_DA);
                if (isEcgOn)
                    mask = mask | BIT(DAU_DATA_TYPE_ECG);
                break;

            case IMAGING_MODE_COLOR:
                mask = BIT(DAU_DATA_TYPE_B) | BIT(DAU_DATA_TYPE_COLOR);
                if (isDaOn)
                    mask = mask | BIT(DAU_DATA_TYPE_DA);
                if (isEcgOn)
                    mask = mask | BIT(DAU_DATA_TYPE_ECG);
                break;

            case IMAGING_MODE_M:
                mask = BIT(DAU_DATA_TYPE_B) | BIT(DAU_DATA_TYPE_M);
                break;

            case IMAGING_MODE_PW:
                mask = BIT(DAU_DATA_TYPE_PW) | BIT(DAU_DATA_TYPE_PW_ADO);
                break;

            case IMAGING_MODE_CW:
                mask = BIT(DAU_DATA_TYPE_CW) | BIT(DAU_DATA_TYPE_CW_ADO);
                break;
        }
    }
    else
    {
        if (isDaOn)
            mask = mask | BIT(DAU_DATA_TYPE_DA);
        if (isEcgOn)
            mask = mask | BIT(DAU_DATA_TYPE_ECG);
    }
    BIT_SET(dataTypes, mask);
    return(dataTypes);
};

//-----------------------------------------------------------------------------
float getTimeSpan(uint32_t sweepSpeedIndex, bool isTCD)
{
    float timeSpan;

    switch (sweepSpeedIndex)
    {
        case 0:
            timeSpan = 6.817;
            break;
        case 2:
            timeSpan = 3.895;
            break;
        case 4:
            timeSpan = 1.818;
            break;
        case 5:
            timeSpan = 1.363;
            break;
        case 3:
            timeSpan = 2.727;
            break;
        case 1: // Default
        default:
            if (isTCD) {
                // default for 35 mm/sec
                timeSpan = 3.895;
            } else {
                // default for 25 mm/sec
                timeSpan = 5.454;
            }
            break;
    }

    ALOGD("getTimeSpan -> timeSpan: %f - isTCD: %s", timeSpan, isTCD ? "true" : "false");
    return timeSpan;
}

//-----------------------------------------------------------------------------
float getSweepSpeedMmSec(uint32_t sweepSpeedIndex, bool isTCD)
{
    float sweepSpeed = 25.0f;

    switch (sweepSpeedIndex)
    {
        case 0:
            sweepSpeed = 20.0f;
            break;
        case 2:
            sweepSpeed = 35.0f;
            break;
        case 3:
            sweepSpeed = 50.0f;
            break;
        case 4:
            sweepSpeed = 75.0f;
            break;
        case 5:
            sweepSpeed = 100.0f;
            break;
        case 1: // Default
        default:
            if (isTCD) {
                // default for 35 mm/sec
                sweepSpeed = 35.0f;
            } else {
                // default for 25 mm/sec
                sweepSpeed = 25.0f;
            }
            break;
    }

    ALOGD("getSweepSpeedMmSec -> sweepSpeed: %f - isTCD: %s", sweepSpeed, isTCD ? "true" : "false");
    return sweepSpeed;
}

//-----------------------------------------------------------------------------
uint32_t getMLinesToAverage(uint32_t sweepSpeedIndex)
{
    uint32_t linesToAvg;

    switch (sweepSpeedIndex)
    {
        case 0:
            linesToAvg = 2;
            break;
        case 1:
            linesToAvg = 2;
            break;
        case 2:
            linesToAvg = 2;
            break;
        case 4:
            linesToAvg = 1;
            break;
        case 5:
            linesToAvg = 1;
            break;
        case 3:   // default for 50 mm/sec
        default:
            linesToAvg = 1;
            break;
    }

    return linesToAvg;
}
