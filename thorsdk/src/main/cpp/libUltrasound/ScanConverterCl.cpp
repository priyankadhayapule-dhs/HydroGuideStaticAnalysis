//
// Copyright 2017 EchoNous Inc.
//
#define LOG_TAG "ScanConverterCl"

#include <string.h>
#include <stdlib.h>
#include <ThorUtils.h>
#include <ElapsedTime.h>
#include <ScanConverterCl.h>

// only needs these lines when GL texture is shared with CL
//#include <EGL/egl.h>
//#include <GLES/gl.h>

const char *kernelSourcePart1 =
        "typedef struct\n" \
                "{\n" \
                "    int numSample;\n" \
                "    int numRay;\n" \
                "    int screenWidth;\n" \
                "    int screenHeight;\n" \
                "    float mat_pp2p[9];\n" \
                "    float vec_apex[3];\n" \
                "    float startImgDepth;\n" \
                "    float endImgDepth;\n" \
                "    float startSampleMm;\n" \
                "    float sampleSpacing;\n" \
                "    float startRayRadian;\n" \
                "    float raySpacing;\n" \
                "    \n" \
                "} ScModelConstants __attribute__ ((aligned (16)));\n" \
                "\n" \
                "#define PI 3.1415926f\n" \
                "\n" \
                "const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE |\n" \
                "                                CLK_FILTER_LINEAR;\n" \
                "\n" \
                "float4 cubicCoef(float x) {\n" \
                "    float x2 = x * x;\n" \
                "    float x3 = x2 * x;\n" \
                "    float4 w;\n" \
                "    w.x =   -x3   + 3.0*x2 - 3.0*x + 1.0;\n" \
                "    w.y =  3.0*x3 - 6.0*x2         + 4.0;\n" \
                "    w.z = -3.0*x3 + 3.0*x2 + 3.0*x + 1.0;\n" \
                "    w.w =  x3;\n" \
                "    return w / 6.0;\n" \
                "}\n" \
                "\n" \
                "__kernel void ScanConverter (\n" \
                "    __read_only image2d_t input,\n";

const char *kernelSourcePart2 =
                "    __constant ScModelConstants* pUniforms\n" \
                ")\n" \
                "{\n" \
                "    int screen_width = pUniforms->screenWidth;\n" \
                "    int screen_height = pUniforms->screenHeight;\n" \
                "    int input_width = pUniforms->numSample;\n" \
                "    int input_height = pUniforms->numRay;\n" \
                "\n" \
                "    float out_pix = 0.0f;\n" \
                "\n" \
                "    float startSampleMm = pUniforms->startSampleMm;\n" \
                "    int num_sample = pUniforms->numSample;\n" \
                "    float sampleSpacing = pUniforms->sampleSpacing;\n" \
                "    float startRayRadian = pUniforms->startRayRadian;\n" \
                "    int num_ray = pUniforms->numRay;\n" \
                "    float raySpacing = pUniforms->raySpacing;\n" \
                "\n" \
                "    float endSampleMm = startSampleMm + (num_sample-1) * sampleSpacing;\n" \
                "    float endRayRadian = startRayRadian + (num_ray - 1) * raySpacing;\n" \
                "\n" \
                "    int2 coord = (int2) (get_global_id(0), screen_height-get_global_id(1));\n" \
                "\n" \
                "    float pix_x, pix_y;\n" \
                "    float x_dis, y_dis, xy_dis, radd;\n" \
                "    float x_loc, r_loc;\n" \
                "    float x_loc_int, x_frac, r_loc_int, r_frac;\n" \
                "\n" \
                "    pix_x = pUniforms->mat_pp2p[0]*coord.x + pUniforms->mat_pp2p[3]*coord.y + pUniforms->mat_pp2p[6];\n" \
                "    pix_y = pUniforms->mat_pp2p[1]*coord.x + pUniforms->mat_pp2p[4]*coord.y + pUniforms->mat_pp2p[7];\n" \
                "    x_dis = pix_x - pUniforms->vec_apex[0];\n" \
                "    y_dis = pix_y - pUniforms->vec_apex[1];\n" \
                "\n" \
                "    xy_dis = hypot(x_dis, y_dis);\n" \
                "    radd = PI/2 - atan2(y_dis, x_dis);\n" \
                "\n" \
                "    x_loc = (xy_dis - startSampleMm)/sampleSpacing;\n" \
                "    r_loc = (radd - startRayRadian)/raySpacing;\n" \
                "\n" \
                "    // x, r int frac\n" \
                "    x_loc_int = floor(x_loc);\n" \
                "    x_frac = x_loc - x_loc_int;\n" \
                "    r_loc_int = floor(r_loc);\n" \
                "    r_frac = r_loc - r_loc_int;\n" \
                "\n" \
                "    float4 xcubic = cubicCoef(x_frac);\n" \
                "    float4 ycubic = cubicCoef(r_frac);\n" \
                "\n" \
                "    float4 c = (float4)(x_loc_int - 0.5, x_loc_int + 1.5, r_loc_int - 0.5, r_loc_int + 1.5);\n" \
                "    float4 s = (float4)(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);\n" \
                "    float4 offset = c + (float4)(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;\n" \
                "\n" \
                "    float sx = s.x / (s.x + s.y);\n" \
                "    float sy = s.z / (s.z + s.w);\n" \
                "\n" \
                "    float4 sample0 = read_imagef(input, sampler, (float2)(offset.x, offset.z));\n" \
                "    float4 sample1 = read_imagef(input, sampler, (float2)(offset.y, offset.z));\n" \
                "    float4 sample2 = read_imagef(input, sampler, (float2)(offset.x, offset.w));\n" \
                "    float4 sample3 = read_imagef(input, sampler, (float2)(offset.y, offset.w));\n" \
                "\n" \
                "    out_pix = mix(mix(sample3.x, sample2.x, sx), mix(sample1.x, sample0.x, sx), sy);\n" \
                "\n" \
                "    //xy_dis < startSampleMm || xy_dis > endSampleMm || y_dis < pUniforms->startImgDepth || y_dis > pUniforms->endImgDepth || radd < startRayRadian || radd > endRayRadian\n" \
                "    if (x_loc < 0.0 || x_loc >= (float)(num_sample - 1) || r_loc < 0.0 || r_loc >= (float)(num_ray - 1)) {\n" \
                "        out_pix = 0.0;\n" \
                "    }\n" \
                "\n" \
                "    uint oid = get_global_id(0) + (screen_height - 1.0 - get_global_id(1)) * screen_width;\n" \
                "\n" \
                "    // put out_pix into -1.0 ~ 1.0\n" \
                "    //out_pix = out_pix * 2.0 - 1.0;\n" \
                "\n";


ScanConverterCL::ScanConverterCL(ScanConverterCL::OutputType outputType) {
    mOutputType = outputType;
    mInit = false;
    // Set input and output buffers to explicitly null, because we later delete them if not null
    // in initCLBufferTex.
    mBuff[0] = nullptr;
    mBuff[1] = nullptr;
}

void ScanConverterCL::setCLKernelSource(std::string &kernelSource) {
    mCLKernelSource = kernelSource;
}


ThorStatus ScanConverterCL::initCLPlatform(unsigned int selected_device)
{
    static bool libcl_init = false;
    if (!libcl_init)
    {
        libcl_init = LoadOpenCL();
        if (!libcl_init)
            RETURN_ERROR(THOR_ERROR, "Failed to load opencl shim");
    }

    ThorStatus          retVal = THOR_ERROR;
    /** Pre-fault */
    cl_int              err = CL_FALSE;
    cl_uint             numDevices;
    cl_platform_id      platform;

    /** Get the platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if(CL_SUCCESS != err)
    {
        ALOGE("clGetPlatformIDs failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clGetPlatformIDs succeed: platformID: %p found", platform);
    }

    /** Check for Qualcomm platform*/
    char platformInfo[1024];
    err = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 1024, platformInfo, 0);
    if (CL_SUCCESS != err)
    {
        ALOGE("clGetPlatformInfo failed: error: %d\n", err);
        goto err_ret;
    }

    ALOGD("Platform: Info %s\n", platformInfo);

    /** Get the device*/
    err = clGetDeviceIDs(platform, selected_device, 1, &mDevice, &numDevices);
    if(CL_SUCCESS != err)
    {
        ALOGE("clGetDeviceIDs failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clGetDeviceIDs succeed: deviceID.");
    }


    ALOGD("Selected device is: ");
    switch (selected_device)
    {
    case CL_DEVICE_TYPE_CPU:
        ALOGD("CPU\n");
        break;
    case CL_DEVICE_TYPE_GPU:
        ALOGD("GPU\n");
        break;
    case CL_DEVICE_TYPE_DEFAULT:
        ALOGD("DEFAULT\n");
        break;
    }

    char deviceInfo[1024];
    err = clGetDeviceInfo(mDevice, CL_DEVICE_NAME, 1024, deviceInfo, 0);
    if(CL_SUCCESS != err)
    {
        ALOGE("clGetDeviceInfo failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("Device: %s found", deviceInfo);
    }

    /** Create the context */
    mCtx = clCreateContext(0, 1, &mDevice, NULL, NULL, &err);
    if(CL_SUCCESS != err)
    {
        ALOGE("clCreateContext failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clCreateContext succeed.");
    }

    mCmdq = clCreateCommandQueueWithProperties(mCtx, mDevice, 0, &err);

    if(CL_SUCCESS != err)
    {
        ALOGE("clCreateCommandQueue failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clCreateCommandQueue succeed.");
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

ThorStatus ScanConverterCL::initCLBufferTex()
{
    ThorStatus  retVal = THOR_ERROR;
    cl_int      err = CL_FALSE;

    /** Input buffer */
    cl_image_format clImageFormat;
    clImageFormat.image_channel_order = CL_R;
    clImageFormat.image_channel_data_type = CL_UNORM_INT8;

    cl_image_desc clImageDesc;
    clImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
    clImageDesc.image_width = mSCMConstants.numSamples;
    clImageDesc.image_height = mSCMConstants.numRays;
    clImageDesc.image_row_pitch = 0;
    clImageDesc.image_slice_pitch = 0;
    clImageDesc.num_mip_levels = 0;
    clImageDesc.num_samples = 0;
    clImageDesc.buffer = NULL;

    LOGD("About to create cl image, width = %d height = %d", mSCMConstants.numSamples, mSCMConstants.numRays);

    // using texture unit
    if (mBuff[0]) { clReleaseMemObject(mBuff[0]); }
    mBuff[0] = clCreateImage(mCtx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, &clImageFormat, &clImageDesc, NULL, &err );
    if(CL_SUCCESS != err)
    {
        ALOGE("clCreateImage [0] TEX failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clCreateImage [0] TEX succeed.");
    }

    ALOGD("Creating input buffer of size %dx%d", (int)mSCMConstants.screenWidth, (int)mSCMConstants.screenHeight);

    /** Output buffer */
    if (mBuff[1]) { clReleaseMemObject(mBuff[1]); }
    if (mOutputType == OutputType::UCHAR8)
        mBuff[1] = clCreateBuffer(mCtx, CL_MEM_WRITE_ONLY, sizeof(u_char)*mSCMConstants.screenWidth*mSCMConstants.screenHeight, NULL, &err);
    else
        mBuff[1] = clCreateBuffer(mCtx, CL_MEM_WRITE_ONLY, sizeof(float)*mSCMConstants.screenWidth*mSCMConstants.screenHeight, NULL, &err);


    if(CL_SUCCESS != err)
    {
        ALOGE("clCreateBuffer [1] failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clCreateBuffer [1] succeed.");
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

ThorStatus ScanConverterCL::initCLKernel()
{
    ThorStatus  retVal = THOR_ERROR;
    cl_int      err = CL_FALSE;

    const char* kernelSource = mCLKernelSource.c_str();

    /** Load kernel source and build the program */
    mProgram = clCreateProgramWithSource(mCtx, 1, (const char**)&kernelSource, NULL, &err);
    if(CL_SUCCESS != err)
    {
        ALOGE("clCreateProgramWithSource failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clCreateProgramWithSource succeed.");
    }

    err = clBuildProgram(mProgram, 0, NULL, NULL, NULL, NULL);
    if(CL_SUCCESS != err)
    {
        char buildLog[256];
        size_t logLength = 0;
        clGetProgramBuildInfo(mProgram,
                              mDevice,
                              CL_PROGRAM_BUILD_LOG,
                              sizeof(buildLog),
                              buildLog,
                              &logLength);

        ALOGE("clBuildProgram failed: error: %d, log = \"%.*s\"\n", err, static_cast<int>(logLength), buildLog);
        goto err_ret;
    }
    else
    {
        ALOGD("clBuildProgram succeed");
    }

    /** Create the kernel */
    mKernel = clCreateKernel(mProgram, "ScanConverter", &err);
    if(CL_SUCCESS != err)
    {
        ALOGE("clCreateKernel failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clCreateKernel succeed.");
    }

    /** Allocate Memory for constant parameters (arguments) **/
    mUniforms = clCreateBuffer(mCtx, CL_MEM_READ_ONLY, sizeof(ScModelConstants), NULL, &err );
    if (CL_SUCCESS != err)
    {
        ALOGE(" clCreateBuffer failed to allocate memory object for ScModelConstants.\n" );
        goto err_ret;
    } else
    {
        ALOGD("clCreateBuffer: memory allocation for Constants/Arguments success.");
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

static const char * OutputTypeName(ScanConverterCL::OutputType outputType) {
    switch (outputType) {
        case ScanConverterCL::OutputType::UCHAR8 : return "UCHAR8";
        case ScanConverterCL::OutputType::FLOAT_UNSIGNED_SCALE_1 : return "FLOAT_UNSIGNED_SCALE_1";
        case ScanConverterCL::OutputType::FLOAT_UNSIGNED_SCALE_255 : return "FLOAT_UNSIGNED_SCALE_255";
        case ScanConverterCL::OutputType::FLOAT_SIGNED : return "FLOAT_SIGNED";
    }
}

ThorStatus ScanConverterCL::runCLTex(u_char* inPtr, float* outPtr)
{
    ELAPSED_FUNC();

    ThorStatus  retVal = THOR_ERROR;

    if (!mInit) {
        ThorStatus status = init();
        if (IS_THOR_ERROR(status)) {
            LOGE("Returning error %08x in ScanConverterCL::runCLTex, error from init()", status);
            return status;
        }
    }

    if (mOutputType == OutputType::UCHAR8) {
        LOGE("Incorrect output type: set to UCHAR8, but provided a float buffer");
        return TER_MISC_PARAM;
    }

    cl_int err = CL_FALSE;
    cl_event event;
    cl_event event2;
    /** Global and local work size*/
    size_t gws[3] = {(size_t)mSCMConstants.screenWidth, (size_t)mSCMConstants.screenHeight, 0};

    cl_mem *uniformMem = &mUniforms;

    // Set up the constant argument
    ScModelConstants* pUniforms = (ScModelConstants*) clEnqueueMapBuffer(mCmdq, *uniformMem, CL_TRUE,
                                                                         CL_MAP_WRITE, 0, sizeof(ScModelConstants), 0, NULL, NULL, &err );

    // setup arguments here
    pUniforms->screenWidth = mSCMConstants.screenWidth;
    pUniforms->screenHeight = mSCMConstants.screenHeight;
    pUniforms->numSamples = mSCMConstants.numSamples;
    pUniforms->numRays = mSCMConstants.numRays;
    pUniforms->startImgDepth = mSCMConstants.startImgDepth;
    pUniforms->endImgDepth = mSCMConstants.endImgDepth;
    pUniforms->startSampleMm = mSCMConstants.startSampleMm;
    pUniforms->sampleSpacing = mSCMConstants.sampleSpacing;
    pUniforms->startRayRadian = mSCMConstants.startRayRadian;
    pUniforms->raySpacing = mSCMConstants.raySpacing;
    pUniforms->mat_pp2p[0] = mSCMConstants.mat_pp2p[0];
    pUniforms->mat_pp2p[1] = mSCMConstants.mat_pp2p[1];
    pUniforms->mat_pp2p[2] = mSCMConstants.mat_pp2p[2];
    pUniforms->mat_pp2p[3] = mSCMConstants.mat_pp2p[3];
    pUniforms->mat_pp2p[4] = mSCMConstants.mat_pp2p[4];
    pUniforms->mat_pp2p[5] = mSCMConstants.mat_pp2p[5];
    pUniforms->mat_pp2p[6] = mSCMConstants.mat_pp2p[6];
    pUniforms->mat_pp2p[7] = mSCMConstants.mat_pp2p[7];
    pUniforms->mat_pp2p[8] = mSCMConstants.mat_pp2p[8];
    pUniforms->vec_apex[0] = mSCMConstants.vec_apex[0];
    pUniforms->vec_apex[1] = mSCMConstants.vec_apex[1];
    pUniforms->vec_apex[2] = mSCMConstants.vec_apex[2];

    err = clEnqueueUnmapMemObject(mCmdq, *uniformMem, pUniforms, 0, NULL, NULL );

    // for this version assuming affine transformation
    /** Set the kernel arguments */
    err &= clSetKernelArg(mKernel, 0, sizeof(cl_mem), &mBuff[0]);
    err &= clSetKernelArg(mKernel, 1, sizeof(cl_mem), &mBuff[1]);
    err &= clSetKernelArg(mKernel, 2, sizeof(cl_mem), uniformMem);


    if(CL_SUCCESS != err)
    {
        ALOGE("clSetKernelArg failed: error: %d\n", err);
        goto err_ret;
    }

    /** Enqueue the buffer write command and kernel launch to the command queue */
    //clEnqueueWriteImage for update texture image
    {
        size_t origin[3] = { 0, 0, 0 };
        size_t region[3] = { (size_t)mSCMConstants.numSamples, (size_t)mSCMConstants.numRays, 1 } ;
        err = clEnqueueWriteImage(mCmdq, mBuff[0], CL_FALSE, origin, region, 0, 0, inPtr, 0, NULL, &event);
    }

    if(CL_SUCCESS != err)
    {
        ALOGE("clEnqueueWriteBuffer failed: error: %d\n", err);
        goto err_ret;
    }

    err = clEnqueueNDRangeKernel(mCmdq, mKernel, 2, 0, gws, NULL, 1, &event, &event2);    // wait until the input image transfer is complete

    if(CL_SUCCESS != err)
    {
        ALOGE("clEnqueueNDRangeKernel failed: error: %d\n", err);
        goto err_ret;
    }

    /** Enqueue the buffer read command*/
    err = clEnqueueReadBuffer(mCmdq, mBuff[1], CL_FALSE, 0, sizeof(float)*mSCMConstants.screenWidth*mSCMConstants.screenHeight, outPtr, 1, &event2, NULL);

    if(CL_SUCCESS != err)
    {
        ALOGE("clEnqueueReadBuffer failed: error: %d\n", err);
        goto err_ret;
    }

    /** Block until finish */
    clFlush(mCmdq);
    clFinish(mCmdq);
    clReleaseEvent(event);

    retVal = THOR_OK;

err_ret:
    return(retVal);
}


ThorStatus ScanConverterCL::runCLTex(u_char* inPtr, u_char* outPtr)
{
    ThorStatus  retVal = THOR_ERROR;

    if (!mInit) {
        ThorStatus status = init();
        if (IS_THOR_ERROR(status)) {
            LOGE("Returning error %08x in ScanConverterCL::runCLTex, error from init()", status);
            return status;
        }
    }

    if (mOutputType != OutputType::UCHAR8) {
        LOGE("Incorrect output type: set to %s, but provided a u_char buffer", OutputTypeName(mOutputType));
        return TER_MISC_PARAM;
    }

    cl_int err = CL_FALSE;
    cl_event event;
    cl_event event2;
    /** Global and local work size*/
    size_t gws[3] = {(size_t)mSCMConstants.screenWidth, (size_t)mSCMConstants.screenHeight, 0};

    cl_mem *uniformMem = &mUniforms;

    // Set up the constant argument
    ScModelConstants* pUniforms = (ScModelConstants*) clEnqueueMapBuffer(mCmdq, *uniformMem, CL_TRUE,
                                                                         CL_MAP_WRITE, 0, sizeof(ScModelConstants), 0, NULL, NULL, &err );

    // setup arguments here
    pUniforms->screenWidth = mSCMConstants.screenWidth;
    pUniforms->screenHeight = mSCMConstants.screenHeight;
    pUniforms->numSamples = mSCMConstants.numSamples;
    pUniforms->numRays = mSCMConstants.numRays;
    pUniforms->startImgDepth = mSCMConstants.startImgDepth;
    pUniforms->endImgDepth = mSCMConstants.endImgDepth;
    pUniforms->startSampleMm = mSCMConstants.startSampleMm;
    pUniforms->sampleSpacing = mSCMConstants.sampleSpacing;
    pUniforms->startRayRadian = mSCMConstants.startRayRadian;
    pUniforms->raySpacing = mSCMConstants.raySpacing;
    pUniforms->mat_pp2p[0] = mSCMConstants.mat_pp2p[0];
    pUniforms->mat_pp2p[1] = mSCMConstants.mat_pp2p[1];
    pUniforms->mat_pp2p[2] = mSCMConstants.mat_pp2p[2];
    pUniforms->mat_pp2p[3] = mSCMConstants.mat_pp2p[3];
    pUniforms->mat_pp2p[4] = mSCMConstants.mat_pp2p[4];
    pUniforms->mat_pp2p[5] = mSCMConstants.mat_pp2p[5];
    pUniforms->mat_pp2p[6] = mSCMConstants.mat_pp2p[6];
    pUniforms->mat_pp2p[7] = mSCMConstants.mat_pp2p[7];
    pUniforms->mat_pp2p[8] = mSCMConstants.mat_pp2p[8];
    pUniforms->vec_apex[0] = mSCMConstants.vec_apex[0];
    pUniforms->vec_apex[1] = mSCMConstants.vec_apex[1];
    pUniforms->vec_apex[2] = mSCMConstants.vec_apex[2];

    err = clEnqueueUnmapMemObject(mCmdq, *uniformMem, pUniforms, 0, NULL, NULL );

    // for this version assuming affine transformation
    /** Set the kernel arguments */
    err &= clSetKernelArg(mKernel, 0, sizeof(cl_mem), &mBuff[0]);
    err &= clSetKernelArg(mKernel, 1, sizeof(cl_mem), &mBuff[1]);
    err &= clSetKernelArg(mKernel, 2, sizeof(cl_mem), uniformMem);


    if(CL_SUCCESS != err)
    {
        ALOGE("clSetKernelArg failed: error: %d\n", err);
        goto err_ret;
    }
    else
    {
        ALOGD("clSetKernelArg succeed");
    }

    /** Enqueue the buffer write command and kernel launch to the command queue */
    //clEnqueueWriteImage for update texture image
    {
        size_t origin[3] = { 0, 0, 0 };
        size_t region[3] = { (size_t)mSCMConstants.numSamples, (size_t)mSCMConstants.numRays, 1 } ;
        err = clEnqueueWriteImage(mCmdq, mBuff[0], CL_FALSE, origin, region, 0, 0, inPtr, 0, NULL, &event);
    }

    if(CL_SUCCESS != err)
    {
        ALOGE("clEnqueueWriteBuffer failed: error: %d\n", err);
        goto err_ret;
    }

    {
        clock_t startTimer1, stopTimer1;
        startTimer1=clock();

        err = clEnqueueNDRangeKernel(mCmdq, mKernel, 2, 0, gws, NULL, 1, &event, &event2);

        stopTimer1 = clock();
        double elapse = 1000.0* (double)(stopTimer1 - startTimer1)/(double)CLOCKS_PER_SEC;
        ALOGD("OpenCL code on the GPU took %g ms\n\n", 1000.0* (double)(stopTimer1 - startTimer1)/(double)CLOCKS_PER_SEC) ;
    }

    if(CL_SUCCESS != err)
    {
        ALOGE("clEnqueueNDRangeKernel failed: error: %d\n", err);
        goto err_ret;
    }

    /** Enqueue the buffer read command*/
    err = clEnqueueReadBuffer(mCmdq, mBuff[1], CL_FALSE, 0, sizeof(u_char)*mSCMConstants.screenWidth*mSCMConstants.screenHeight, outPtr, 1, &event2, NULL);

    if(CL_SUCCESS != err)
    {
        ALOGE("clEnqueueReadBuffer failed: error: %d\n", err);
        goto err_ret;
    }

    /** Block until finish */
    clFlush(mCmdq);
    clFinish(mCmdq);
    clReleaseEvent(event);

    retVal = THOR_OK;

err_ret:
    return(retVal);
}


ThorStatus ScanConverterCL::init()
{
    ThorStatus  retVal = THOR_ERROR;
    std::string kernelStr = std::string(kernelSourcePart1);


    // initialize CL Platform
    if (IS_THOR_ERROR(initCLPlatform(CL_DEVICE_TYPE_GPU)))     // using GPU
    {
        ALOGE("initCLPlatform failed");
        goto err_ret;
    }

    // build Kernel Source
    if (mOutputType == OutputType::UCHAR8) {
        kernelStr += "    __global unsigned char *output,\n";
    } else {
        kernelStr += "    __global float *output,\n";
    }

    kernelStr += std::string(kernelSourcePart2);

    if (mOutputType == OutputType::UCHAR8) {
        kernelStr += "    out_pix = round(out_pix*255.0);\n";
        kernelStr += "    output[oid] = (unsigned char) out_pix;\n";

    } else if (mOutputType == OutputType::FLOAT_UNSIGNED_SCALE_1) {
        kernelStr += "    output[oid] = out_pix;\n";
    } else if (mOutputType == OutputType::FLOAT_UNSIGNED_SCALE_255) {
        kernelStr += "    output[oid] = out_pix * 255.0;\n";
    }
    else {
        // FLOAT SIGNED
        kernelStr += "    out_pix = out_pix * 2.0 - 1.0;\n";
        kernelStr += "    output[oid] = out_pix;\n";
    }

    kernelStr += "}";

    // setKernelSource with the String
    setCLKernelSource(kernelStr);

    // initialize buffer and kernal
    if (IS_THOR_ERROR(initCLBufferTex()))
    {
        ALOGE("initCLBufferTex failed");
        goto err_ret;
    }
    if (IS_THOR_ERROR(initCLKernel()))
    {
        ALOGE("initCLKernel failed");
        goto err_ret;
    }

    retVal = THOR_OK;
    mInit = true;

err_ret:
    return(retVal);
}

ThorStatus ScanConverterCL::setConversionParameters(cl_int numSamples, cl_int numRays, cl_int screenWidth,
                                                    cl_int screenHeight, cl_float startImgDepth, cl_float endImgDepth, cl_float startSampleMm,
                                                    cl_float sampleSpacing, cl_float startRayRadian, cl_float raySpacing,
                                                    cl_float flipX, cl_float flipY) {
    ScanConverterParams params;
    params.numSamples = numSamples;
    params.numRays = numRays;
    params.startSampleMm = startSampleMm;
    params.sampleSpacingMm = sampleSpacing;
    params.startRayRadian = startRayRadian;
    params.raySpacing = raySpacing;

    // legacy behavior
    auto mode = ScaleMode::FIT_HEIGHT;

    return setConversionParameters(screenWidth, screenHeight, flipX, flipY, mode, params);
}

ThorStatus ScanConverterCL::setConversionParameters(int width, int height, float flipX, float flipY, ScanConverterCL::ScaleMode mode, const ScanConverterParams& params) {

    // setting up conversion parameters
    // Potential TODO: can (and should) we rewrite the openCL code above to not need all this info?
    // The OpenCL code is going from pixel space -> physical space -> ray/sample space, should we
    // potentially rewrite to use a single transform from pixel -> raw/sample space?
    mSCMConstants.numSamples = params.numSamples;
    mSCMConstants.numRays = params.numRays;
    mSCMConstants.screenWidth = width;
    mSCMConstants.screenHeight = height;
    mSCMConstants.startImgDepth = params.startSampleMm;
    mSCMConstants.endImgDepth = params.startSampleMm + (params.numSamples-1) * params.sampleSpacingMm;
    mSCMConstants.startSampleMm = params.startSampleMm;
    mSCMConstants.sampleSpacing = params.sampleSpacingMm;
    mSCMConstants.startRayRadian = params.startRayRadian;
    mSCMConstants.raySpacing = params.raySpacing;

    LOGD("setConversionParameters, constants samples/rays = %d %d", mSCMConstants.numSamples, mSCMConstants.numRays);

    auto shiftScale = computeXYScaleShift(mode, params);

    mSCHelper.init((float)width, (float)height, params);
    mSCHelper.setScaleXYShift(shiftScale.xScale, shiftScale.yScale, shiftScale.xShift, shiftScale.yShift);
    mSCHelper.setFlip(flipX, flipY);
    mPhysicalToPixel = mSCHelper.getPhysicalToPixelTransform((float)width, (float)height);
    mPixelToPhysical = mPhysicalToPixel.inverse();

    // apex location
    float vec_apex_loc_mm[3];
    vec_apex_loc_mm[0] = 0.0f;
    vec_apex_loc_mm[1] = 0.0f;
    vec_apex_loc_mm[2] = 1.0f;

    // copy the calculated results to conversion model constants
    memcpy(mSCMConstants.mat_pp2p, mPixelToPhysical.ptr(), sizeof(mSCMConstants.mat_pp2p));
    memcpy(mSCMConstants.vec_apex, vec_apex_loc_mm, sizeof(vec_apex_loc_mm));

    // If needed, recreate input and output buffers
    if (mInit) {
        return initCLBufferTex();
    }
    LOGD("Not reinitializing buffers (mInit is false)");

    return THOR_OK;
}

void ScanConverterCL::setFlip(float flipX, float flipY) {
    mSCHelper.setFlip(flipX, flipY);
    mPhysicalToPixel = mSCHelper.getPhysicalToPixelTransform((float)mSCMConstants.screenWidth, (float)mSCMConstants.screenHeight);
    mPixelToPhysical = mPhysicalToPixel.inverse();
    memcpy(mSCMConstants.mat_pp2p, mPixelToPhysical.ptr(), sizeof(mSCMConstants.mat_pp2p));
}


void ScanConverterCL::verify()
{
  // placeholder for verification function
}

ScanConverterCL::XYScaleShift ScanConverterCL::computeXYScaleShift(ScanConverterCL::ScaleMode mode,
                                                                   const ScanConverterParams &params) {

    float imagingDepth = (params.numSamples-1) * params.sampleSpacingMm;
    float height = params.startSampleMm + imagingDepth; // height of cone in mm
    float width = std::abs(height * sin(params.startRayRadian) * 2.0f); // width of widest part of cone, in mm
    auto larger = std::max(height, width);

    // Imaging cone is always centered in image, so the shift values do not change per mode

    switch (mode) {
        case ScaleMode::FIT_HEIGHT:
            return {2.0f/height, 2.0f/height, 0.0f, -1.0f};
        case ScaleMode::FIT_WIDTH:
            return {2.0f/width, 2.0f/width, 0.0f, -1.0f};
        case ScaleMode::FIT_BOTH:
            return {2.0f/larger, 2.0f/larger, 0.0f, -1.0f};
        case ScaleMode::NON_UNIFORM_SCALE_TO_FIT:
            return {2.0f/width, 2.0f/height, 0.0f, -1.0f};
    }
}

const Matrix3& ScanConverterCL::getPhysicalToPixelTransform() const { return mPhysicalToPixel; }
const Matrix3& ScanConverterCL::getPixelToPhysicalTransform() const { return mPixelToPhysical; }


void ScanConverterCL::cleanupCL()
{
    if (mInit) {
        clReleaseMemObject(mBuff[1]);
        clReleaseMemObject(mBuff[0]);
        clReleaseMemObject(mUniforms);
        clReleaseKernel(mKernel);
        clReleaseProgram(mProgram);
        clReleaseCommandQueue(mCmdq);
        clReleaseContext(mCtx);
    }
}

ScanConverterCL::~ScanConverterCL()
{
    cleanupCL();
}
