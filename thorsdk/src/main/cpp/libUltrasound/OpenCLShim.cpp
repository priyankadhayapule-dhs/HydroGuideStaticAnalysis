//
// Copyright 2019 EchoNous Inc.
//

#define LOG_TAG "OpenCLShim"

#include <dlfcn.h>
#include <ThorUtils.h>
#include <OpenCLShim.h>

bool LoadOpenCL()
{
    // List of possible paths taken from https://stackoverflow.com/questions/31611790/using-opencl-in-the-new-android-studio
    const char *opencl_paths[] = {
            // Android
            "/vendor/lib64/libOpenCL.so",
            "/system/lib/libOpenCL.so", "/system/vendor/lib/libOpenCL.so",
            "/system/vendor/lib/egl/libGLES_mali.so",
            "/system/vendor/lib/libPVROCL.so",
            "/data/data/org.pocl.libs/files/lib/libpocl.so",
            // Linux
            "/usr/lib/libOpenCL.so",            "/usr/local/lib/libOpenCL.so",
            "/usr/local/lib/libpocl.so",
            "/usr/lib64/libOpenCL.so", "/usr/lib32/libOpenCL.so",
            "libOpenCL.so"
    };
    void *handle = nullptr;
    for (const char *path : opencl_paths)
    {
        LOGD("Trying to open path %s", path);
        handle = dlopen(path, RTLD_LAZY);
        if (nullptr != handle) {
            LOGD("Found OpenCL.so at path %s", path);
            break;
        }
    }
    if (nullptr == handle)
    {
        ALOGE("Failed to load libOpenCL by any known path.");
        return false;
    }

#define LOADFUNC(func) do{shim_ ##func = reinterpret_cast<PFN_ ##func>(dlsym(handle, #func)); if (nullptr == shim_ ##func) { ALOGE("Failed to load OpenCL function %s", #func); return false;}}while(0)
    LOADFUNC(clGetPlatformIDs);
    LOADFUNC(clGetPlatformInfo);
    LOADFUNC(clGetDeviceIDs);
    LOADFUNC(clGetDeviceInfo);
    LOADFUNC(clCreateSubDevices);
    LOADFUNC(clRetainDevice);
    LOADFUNC(clReleaseDevice);
    LOADFUNC(clCreateContext);
    LOADFUNC(clCreateContextFromType);
    LOADFUNC(clRetainContext);
    LOADFUNC(clReleaseContext);
    LOADFUNC(clGetContextInfo);
    LOADFUNC(clCreateCommandQueueWithProperties);
    LOADFUNC(clRetainCommandQueue);
    LOADFUNC(clReleaseCommandQueue);
    LOADFUNC(clGetCommandQueueInfo);
    LOADFUNC(clCreateBuffer);
    LOADFUNC(clCreateSubBuffer);
    LOADFUNC(clCreateImage);
    LOADFUNC(clCreatePipe);
    LOADFUNC(clRetainMemObject);
    LOADFUNC(clReleaseMemObject);
    LOADFUNC(clGetSupportedImageFormats);
    LOADFUNC(clGetMemObjectInfo);
    LOADFUNC(clGetImageInfo);
    LOADFUNC(clGetPipeInfo);
    LOADFUNC(clSetMemObjectDestructorCallback);
    LOADFUNC(clSVMAlloc);
    LOADFUNC(clSVMFree);
    LOADFUNC(clCreateSamplerWithProperties);
    LOADFUNC(clRetainSampler);
    LOADFUNC(clReleaseSampler);
    LOADFUNC(clGetSamplerInfo);
    LOADFUNC(clCreateProgramWithSource);
    LOADFUNC(clCreateProgramWithBinary);
    LOADFUNC(clCreateProgramWithBuiltInKernels);
    LOADFUNC(clRetainProgram);
    LOADFUNC(clReleaseProgram);
    LOADFUNC(clBuildProgram);
    LOADFUNC(clCompileProgram);
    LOADFUNC(clLinkProgram);
    LOADFUNC(clUnloadPlatformCompiler);
    LOADFUNC(clGetProgramInfo);
    LOADFUNC(clGetProgramBuildInfo);
    LOADFUNC(clCreateKernel);
    LOADFUNC(clCreateKernelsInProgram);
    LOADFUNC(clRetainKernel);
    LOADFUNC(clReleaseKernel);
    LOADFUNC(clSetKernelArg);
    LOADFUNC(clSetKernelArgSVMPointer);
    LOADFUNC(clSetKernelExecInfo);
    LOADFUNC(clGetKernelInfo);
    LOADFUNC(clGetKernelArgInfo);
    LOADFUNC(clGetKernelWorkGroupInfo);
    LOADFUNC(clWaitForEvents);
    LOADFUNC(clGetEventInfo);
    LOADFUNC(clCreateUserEvent);
    LOADFUNC(clRetainEvent);
    LOADFUNC(clReleaseEvent);
    LOADFUNC(clSetUserEventStatus);
    LOADFUNC(clSetEventCallback);
    LOADFUNC(clGetEventProfilingInfo);
    LOADFUNC(clFlush);
    LOADFUNC(clFinish);
    LOADFUNC(clEnqueueReadBuffer);
    LOADFUNC(clEnqueueReadBufferRect);
    LOADFUNC(clEnqueueWriteBuffer);
    LOADFUNC(clEnqueueWriteBufferRect);
    LOADFUNC(clEnqueueFillBuffer);
    LOADFUNC(clEnqueueCopyBuffer);
    LOADFUNC(clEnqueueCopyBufferRect);
    LOADFUNC(clEnqueueReadImage);
    LOADFUNC(clEnqueueWriteImage);
    LOADFUNC(clEnqueueFillImage);
    LOADFUNC(clEnqueueCopyImage);
    LOADFUNC(clEnqueueCopyImageToBuffer);
    LOADFUNC(clEnqueueCopyBufferToImage);
    LOADFUNC(clEnqueueMapBuffer);
    LOADFUNC(clEnqueueMapImage);
    LOADFUNC(clEnqueueUnmapMemObject);
    LOADFUNC(clEnqueueMigrateMemObjects);
    LOADFUNC(clEnqueueNDRangeKernel);
    LOADFUNC(clEnqueueNativeKernel);
    LOADFUNC(clEnqueueMarkerWithWaitList);
    LOADFUNC(clEnqueueBarrierWithWaitList);
    LOADFUNC(clEnqueueSVMFree);
    LOADFUNC(clEnqueueSVMMemcpy);
    LOADFUNC(clEnqueueSVMMemFill);
    LOADFUNC(clEnqueueSVMMap);
    LOADFUNC(clEnqueueSVMUnmap);
    LOADFUNC(clGetExtensionFunctionAddressForPlatform);

#undef LOADFUNC


    return true;
}

PFN_clGetPlatformIDs shim_clGetPlatformIDs;
PFN_clGetPlatformInfo shim_clGetPlatformInfo;
PFN_clGetDeviceIDs shim_clGetDeviceIDs;
PFN_clGetDeviceInfo shim_clGetDeviceInfo;
PFN_clCreateSubDevices shim_clCreateSubDevices;
PFN_clRetainDevice shim_clRetainDevice;
PFN_clReleaseDevice shim_clReleaseDevice;
PFN_clCreateContext shim_clCreateContext;
PFN_clCreateContextFromType shim_clCreateContextFromType;
PFN_clRetainContext shim_clRetainContext;
PFN_clReleaseContext shim_clReleaseContext;
PFN_clGetContextInfo shim_clGetContextInfo;
PFN_clCreateCommandQueueWithProperties shim_clCreateCommandQueueWithProperties;
PFN_clRetainCommandQueue shim_clRetainCommandQueue;
PFN_clReleaseCommandQueue shim_clReleaseCommandQueue;
PFN_clGetCommandQueueInfo shim_clGetCommandQueueInfo;
PFN_clCreateBuffer shim_clCreateBuffer;
PFN_clCreateSubBuffer shim_clCreateSubBuffer;
PFN_clCreateImage shim_clCreateImage;
PFN_clCreatePipe shim_clCreatePipe;
PFN_clRetainMemObject shim_clRetainMemObject;
PFN_clReleaseMemObject shim_clReleaseMemObject;
PFN_clGetSupportedImageFormats shim_clGetSupportedImageFormats;
PFN_clGetMemObjectInfo shim_clGetMemObjectInfo;
PFN_clGetImageInfo shim_clGetImageInfo;
PFN_clGetPipeInfo shim_clGetPipeInfo;
PFN_clSetMemObjectDestructorCallback shim_clSetMemObjectDestructorCallback;
PFN_clSVMAlloc shim_clSVMAlloc;
PFN_clSVMFree shim_clSVMFree;
PFN_clCreateSamplerWithProperties shim_clCreateSamplerWithProperties;
PFN_clRetainSampler shim_clRetainSampler;
PFN_clReleaseSampler shim_clReleaseSampler;
PFN_clGetSamplerInfo shim_clGetSamplerInfo;
PFN_clCreateProgramWithSource shim_clCreateProgramWithSource;
PFN_clCreateProgramWithBinary shim_clCreateProgramWithBinary;
PFN_clCreateProgramWithBuiltInKernels shim_clCreateProgramWithBuiltInKernels;
PFN_clRetainProgram shim_clRetainProgram;
PFN_clReleaseProgram shim_clReleaseProgram;
PFN_clBuildProgram shim_clBuildProgram;
PFN_clCompileProgram shim_clCompileProgram;
PFN_clLinkProgram shim_clLinkProgram;
PFN_clUnloadPlatformCompiler shim_clUnloadPlatformCompiler;
PFN_clGetProgramInfo shim_clGetProgramInfo;
PFN_clGetProgramBuildInfo shim_clGetProgramBuildInfo;
PFN_clCreateKernel shim_clCreateKernel;
PFN_clCreateKernelsInProgram shim_clCreateKernelsInProgram;
PFN_clRetainKernel shim_clRetainKernel;
PFN_clReleaseKernel shim_clReleaseKernel;
PFN_clSetKernelArg shim_clSetKernelArg;
PFN_clSetKernelArgSVMPointer shim_clSetKernelArgSVMPointer;
PFN_clSetKernelExecInfo shim_clSetKernelExecInfo;
PFN_clGetKernelInfo shim_clGetKernelInfo;
PFN_clGetKernelArgInfo shim_clGetKernelArgInfo;
PFN_clGetKernelWorkGroupInfo shim_clGetKernelWorkGroupInfo;
PFN_clWaitForEvents shim_clWaitForEvents;
PFN_clGetEventInfo shim_clGetEventInfo;
PFN_clCreateUserEvent shim_clCreateUserEvent;
PFN_clRetainEvent shim_clRetainEvent;
PFN_clReleaseEvent shim_clReleaseEvent;
PFN_clSetUserEventStatus shim_clSetUserEventStatus;
PFN_clSetEventCallback shim_clSetEventCallback;
PFN_clGetEventProfilingInfo shim_clGetEventProfilingInfo;
PFN_clFlush shim_clFlush;
PFN_clFinish shim_clFinish;
PFN_clEnqueueReadBuffer shim_clEnqueueReadBuffer;
PFN_clEnqueueReadBufferRect shim_clEnqueueReadBufferRect;
PFN_clEnqueueWriteBuffer shim_clEnqueueWriteBuffer;
PFN_clEnqueueWriteBufferRect shim_clEnqueueWriteBufferRect;
PFN_clEnqueueFillBuffer shim_clEnqueueFillBuffer;
PFN_clEnqueueCopyBuffer shim_clEnqueueCopyBuffer;
PFN_clEnqueueCopyBufferRect shim_clEnqueueCopyBufferRect;
PFN_clEnqueueReadImage shim_clEnqueueReadImage;
PFN_clEnqueueWriteImage shim_clEnqueueWriteImage;
PFN_clEnqueueFillImage shim_clEnqueueFillImage;
PFN_clEnqueueCopyImage shim_clEnqueueCopyImage;
PFN_clEnqueueCopyImageToBuffer shim_clEnqueueCopyImageToBuffer;
PFN_clEnqueueCopyBufferToImage shim_clEnqueueCopyBufferToImage;
PFN_clEnqueueMapBuffer shim_clEnqueueMapBuffer;
PFN_clEnqueueMapImage shim_clEnqueueMapImage;
PFN_clEnqueueUnmapMemObject shim_clEnqueueUnmapMemObject;
PFN_clEnqueueMigrateMemObjects shim_clEnqueueMigrateMemObjects;
PFN_clEnqueueNDRangeKernel shim_clEnqueueNDRangeKernel;
PFN_clEnqueueNativeKernel shim_clEnqueueNativeKernel;
PFN_clEnqueueMarkerWithWaitList shim_clEnqueueMarkerWithWaitList;
PFN_clEnqueueBarrierWithWaitList shim_clEnqueueBarrierWithWaitList;
PFN_clEnqueueSVMFree shim_clEnqueueSVMFree;
PFN_clEnqueueSVMMemcpy shim_clEnqueueSVMMemcpy;
PFN_clEnqueueSVMMemFill shim_clEnqueueSVMMemFill;
PFN_clEnqueueSVMMap shim_clEnqueueSVMMap;
PFN_clEnqueueSVMUnmap shim_clEnqueueSVMUnmap;
PFN_clGetExtensionFunctionAddressForPlatform shim_clGetExtensionFunctionAddressForPlatform;
