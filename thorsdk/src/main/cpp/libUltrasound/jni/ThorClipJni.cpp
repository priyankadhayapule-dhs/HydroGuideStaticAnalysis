#define LOG_TAG "ThorClipJni"
#include <jni.h>
#include <ThorUtils.h>
#include <ThorError.h>
#include <ThorConstants.h>
#include <ThorCapnpTypes.capnp.h>
#include <ScanConverterCl.h>
#include <capnp/serialize.h>
#include <capnp/message.h>

// For open()/close()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


namespace echonous {
    static jclass gConvertResultClass = nullptr;
    static jmethodID gConvertResultCtor = nullptr;

    static jclass gByteBufferClass = nullptr;
    static jmethodID gByteBufferAllocateDirect = nullptr;

}

namespace {

ThorStatus CreateScanConverter(const echonous::capnp::ThorClip::Reader &clip, int width, int height, bool scaleToFit, ScanConverterCL *sc) {
    ThorStatus status;
    if (sc == nullptr) {
        LOGE("Illegal argument, sc cannot be NULL (in %s)", __func__);
        return TER_MISC_PARAM;
    }
    auto params = clip.getCineParams().getBmodeParams();
    float endImgDepth = params.getStartSampleMm() +
                        (params.getSampleSpacingMm() * static_cast<float>(params.getNumSamples() - 1));
    float hScale = 1.0f;
    float aspect = static_cast<float>(width)/static_cast<float>(height);
    if (scaleToFit) {
        hScale = aspect*0.5f/sin(-params.getStartRayRadian());
    }

    status = sc->setConversionParameters(
            static_cast<cl_int>(params.getNumSamples()),
            static_cast<cl_int>(params.getNumRays()),
            width,
            height,
            0.0f,
            endImgDepth,
            params.getStartSampleMm(),
            params.getSampleSpacingMm(),
            params.getStartRayRadian(),
            params.getRaySpacing(),
            -1.0f * hScale,
            1.0f);
    if (IS_THOR_ERROR(status)) {
        LOGE("Error from ScanConverterCL::setConversionParameters(): 0x%08x (in %s)", status, __func__);
        return status;
    }
    status = sc->init();
    if (IS_THOR_ERROR(status)) {
        LOGE("Error from ScanConverterCL::init(): 0x%08x (in %s)", status, __func__);
        return status;
    }

    return THOR_OK;
}

}

extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ThorClip_nativeInit(JNIEnv *env, jobject thiz) {
    using namespace echonous;
    gConvertResultClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/echonous/ThorClip$ConvertResult")));
    gConvertResultCtor = env->GetMethodID(gConvertResultClass, "<init>", "(ILjava/nio/ByteBuffer;)V");

    gByteBufferClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("java/nio/ByteBuffer")));
    gByteBufferAllocateDirect = env->GetStaticMethodID(gByteBufferClass, "allocateDirect", "(I)Ljava/nio/ByteBuffer;");
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_echonous_ThorClip_nativeGetScanConvertedBModeFrames(JNIEnv *env, jobject thiz,
                                                             jstring jfile, jint width,
                                                             jint height) {
    using namespace echonous;
    ThorStatus status;

    // Open file as ThorClip::Reader
    const char *file = env->GetStringUTFChars(jfile, nullptr);
    try {
        auto fd = kj::AutoCloseFd(::open(file, O_RDONLY));
        if (fd == nullptr) {
            LOGE("Open file %s failed, errno = %d (in %s)", file, errno, __func__);
            return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(TER_MISC_OPEN_FILE_FAILED), static_cast<jobject>(nullptr));
        }
        env->ReleaseStringUTFChars(jfile, file);

        ::capnp::ReaderOptions options;
        options.traversalLimitInWords =
                1024 * 1024 * 128; // expand traversal limit to allow up to 1GiB files
        ::capnp::StreamFdMessageReader message(std::move(fd), options);
        auto clip = message.getRoot<echonous::capnp::ThorClip>();

        // Create ScanConverterCL
        ScanConverterCL sc;
        // TODO should this be a parameter from Java/Kotlin?
        bool scaleToFit = true;
        status = CreateScanConverter(clip, width, height, scaleToFit, &sc);
        if (IS_THOR_ERROR(status)) {
            return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(status), static_cast<jobject>(nullptr));
        }

        // Create output bytebuffer
        jint numFrames = static_cast<jint>(clip.getNumFrames());
        jobject bytebuffer = env->CallStaticObjectMethod(gByteBufferClass, gByteBufferAllocateDirect, static_cast<jint>(sizeof(float) * numFrames * width * height));
        if (bytebuffer == nullptr) {
            LOGE("Received NULL for ByteBuffer.allocateDirect (in %s)", __func__);
            return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(TER_MISC_PARAM), static_cast<jobject>(nullptr));
        }
        auto *framePtr = static_cast<float*>(env->GetDirectBufferAddress(bytebuffer));
        if (framePtr == nullptr) {
            LOGE("Received NULL for ByteBuffer.GetDirectBufferAddress (in %s)", __func__);
            return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(TER_MISC_PARAM), static_cast<jobject>(nullptr));
        }

        // Convert each frame
        auto bmode = clip.getBmode().getRawFrames();
        for (jint i = 0; i != numFrames; ++i) {
            auto frameData = bmode[i].getFrame();
            if (frameData.size() != MAX_B_FRAME_SIZE) {
                LOGE("Unexpected size for frame data (got %zu, expected %u) (in %s)", frameData.size(), MAX_B_FRAME_SIZE, __func__);
                return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(TER_MISC_INVALID_FILE), static_cast<jobject>(nullptr));
            }
            // ScanConverterCL takes a non const pointer, but never tries to mutate it
            auto  *input = const_cast<u_char*>(frameData.begin());
            float *output = &framePtr[i*width*height];
            status = sc.runCLTex(input, output);
            if (IS_THOR_ERROR(status)) {
                return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(status), static_cast<jobject>(nullptr));
            }
        }

        return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(THOR_OK), bytebuffer);
    } catch (std::exception &e) {
        LOGE("Exception while trying to read thor file in Capnp format: %s", e.what());
        return env->NewObject(gConvertResultClass, gConvertResultCtor, static_cast<jint>(TER_MISC_INVALID_FILE), static_cast<jobject>(nullptr));
    }
}

