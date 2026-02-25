//
// Copyright 2017 EchoNous Inc.
//
#pragma once

#include "OpenCLShim.h"
#include <sys/types.h>
#include <string>
#include <ViewMatrix3x3.h>
#include <ThorError.h>
#include <ScanConverterHelper.h>

// SC Model Constants
// NOTE: Do not change the members or order of members without also changing
//       a matching variant in the kernel source.
typedef struct
{
    cl_int numSamples;
    cl_int numRays;
    cl_int screenWidth;
    cl_int screenHeight;
    cl_float mat_pp2p[9];
    cl_float vec_apex[3];
    cl_float startImgDepth;
    cl_float endImgDepth;
    cl_float startSampleMm;
    cl_float sampleSpacing;
    cl_float startRayRadian;
    cl_float raySpacing;
} ScModelConstants;

class ScanConverterCL
{
public:
    enum class OutputType {
        UCHAR8,
        FLOAT_UNSIGNED_SCALE_1,
        FLOAT_UNSIGNED_SCALE_255,
        FLOAT_SIGNED
    };

    // How to scale the physical space to fit in the image
    enum class ScaleMode {
        FIT_HEIGHT, // Fix height as filling the entire image, corners are typically cut off for most imaging angles
        FIT_WIDTH,  // Fix width as filling the entire image. In typical imaging params, this letterboxes the imaging cone
        FIT_BOTH,   // Ensure both height and width fit in the image, adding border to top/bottom or sides as necessary. In typical imaging params, the outcome is identical to FIT_WIDTH
        NON_UNIFORM_SCALE_TO_FIT,  // Scale X and Y independently to fill entire image.
    };

public: // Methods
    ScanConverterCL(OutputType outputType = OutputType::FLOAT_UNSIGNED_SCALE_1);
    ~ScanConverterCL();

    /** Wrapper for initialization */
    ThorStatus init();

    /** Set Conversion Parameters -- must be called before
     *  calling init. May be called after init to reset parameters. */
    ThorStatus setConversionParameters(int width,
                                       int height,
                                       float flipX,
                                       float flipY,
                                       ScaleMode mode,
                                       const ScanConverterParams& params);

    // Quick set of flip parameters only
    void setFlip(float flipX, float flipY);

    ThorStatus setConversionParameters(cl_int numSamples,
                                       cl_int numRays,
                                       cl_int screenWidth,
                                       cl_int screenHeight,
                                       cl_float startImgDepth,
                                       cl_float endImgDepth,
                                       cl_float startSampleMm,
                                       cl_float sampleSpacing,
                                       cl_float startRayRadian,
                                       cl_float raySpacing,
                                       cl_float flipX,
                                       cl_float flipY);

    /** Launch CL kernels*/
    ThorStatus runCLTex(u_char* inPtr, float* outPtr);
    ThorStatus runCLTex(u_char* inPtr, u_char* outPtr);

    /** Cleanup the resources*/
    void cleanupCL();

    /** Verify correctness*/
    void verify();

    const Matrix3& getPhysicalToPixelTransform() const;
    const Matrix3& getPixelToPhysicalTransform() const;

private: // Methods

    void setCLKernelSource(std::string& kernelSource);

    /** Setup platform and device*/
    ThorStatus initCLPlatform( unsigned int selected_device=CL_DEVICE_TYPE_DEFAULT);

    /** Setup CL buffers*/
    ThorStatus initCLBufferTex();

    /** Build CL program and create kernel*/
    ThorStatus initCLKernel();

    struct XYScaleShift {
        float xScale;
        float yScale;
        float xShift;
        float yShift;
    };

    XYScaleShift computeXYScaleShift(ScaleMode mode, const ScanConverterParams& params);

private: // Properties
    ScanConverterHelper     mSCHelper;
    Matrix3                 mPhysicalToPixel;
    Matrix3                 mPixelToPhysical;
    ScModelConstants        mSCMConstants;
    std::string             mCLKernelSource;
    OutputType              mOutputType;
    bool                    mInit;
    cl_device_id            mDevice;
    cl_context              mCtx;
    cl_command_queue        mCmdq;
    cl_program              mProgram;
    cl_kernel               mKernel;
    cl_mem                  mBuff[2];
    cl_mem                  mUniforms;
};

