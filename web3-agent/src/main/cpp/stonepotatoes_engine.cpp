#include <jni.h>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

// Include the actual VulkanBackend from StonePotatoes
#ifdef _WIN32
    #ifdef SP_VULKAN
        #include "VulkanBackend.h"
        #include "ComputeDevice.h"
        using namespace StonePotatoes::GPU;
    #endif
#endif

// Global VulkanBackend instance
#ifdef SP_VULKAN
static std::unique_ptr<VulkanBackend> g_backend;
#endif

extern "C" {

JNIEXPORT jboolean JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_isAvailable(JNIEnv* env, jclass clazz) {
#ifdef SP_VULKAN
    return VulkanBackend::IsAvailable();
#else
    return false;
#endif
}

JNIEXPORT jboolean JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_initialize(JNIEnv* env, jclass clazz, jint deviceIndex) {
#ifdef SP_VULKAN
    try {
        g_backend = std::make_unique<VulkanBackend>();
        return g_backend->Initialize(static_cast<uint32_t>(deviceIndex));
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

JNIEXPORT void JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_shutdown(JNIEnv* env, jclass clazz) {
#ifdef SP_VULKAN
    if (g_backend) {
        g_backend->Shutdown();
        g_backend.reset();
    }
#endif
}

JNIEXPORT jobject JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_getDeviceCaps(JNIEnv* env, jclass clazz) {
#ifdef SP_VULKAN
    if (!g_backend || !g_backend->IsInitialized()) {
        return nullptr;
    }

    const DeviceCaps& caps = g_backend->GetCaps();

    // Create DeviceCaps Java object
    jclass deviceCapsClass = env->FindClass("io/stonepotatoes/web3agent/engine/GameEngine$DeviceCaps");
    if (!deviceCapsClass) return nullptr;

    jmethodID constructor = env->GetMethodID(deviceCapsClass, "<init>", "(Ljava/lang/String;JIZ)V");
    if (!constructor) return nullptr;

    jstring name = env->NewStringUTF(caps.name.c_str());

    jobject result = env->NewObject(deviceCapsClass, constructor,
        name,
        static_cast<jlong>(caps.memoryBytes),
        static_cast<jint>(caps.computeUnits),
        static_cast<jboolean>(caps.supportsAtomics));

    return result;
#else
    return nullptr;
#endif
}

JNIEXPORT jlong JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_createBuffer(JNIEnv* env, jclass clazz, jlong bytes) {
#ifdef SP_VULKAN
    if (!g_backend || !g_backend->IsInitialized()) {
        return 0;
    }

    VulkanBackend::BufferHandle handle = g_backend->CreateBuffer(static_cast<uint64_t>(bytes));
    return static_cast<jlong>(handle.id);
#else
    return 0;
#endif
}

JNIEXPORT void JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_destroyBuffer(JNIEnv* env, jclass clazz, jlong bufferHandle) {
#ifdef SP_VULKAN
    if (!g_backend) return;

    VulkanBackend::BufferHandle handle;
    handle.id = static_cast<uint64_t>(bufferHandle);
    g_backend->DestroyBuffer(handle);
#endif
}

JNIEXPORT jboolean JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_writeBuffer(JNIEnv* env, jclass clazz,
                                                                 jlong bufferHandle, jbyteArray data, jlong offset) {
#ifdef SP_VULKAN
    if (!g_backend || !g_backend->IsInitialized()) {
        return false;
    }

    VulkanBackend::BufferHandle handle;
    handle.id = static_cast<uint64_t>(bufferHandle);

    jbyte* dataPtr = env->GetByteArrayElements(data, nullptr);
    jsize dataLen = env->GetArrayLength(data);

    bool result = g_backend->WriteBuffer(handle, dataPtr, static_cast<uint64_t>(dataLen),
                                         static_cast<uint64_t>(offset));

    env->ReleaseByteArrayElements(data, dataPtr, JNI_ABORT);
    return result;
#else
    return false;
#endif
}

JNIEXPORT jbyteArray JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_readBuffer(JNIEnv* env, jclass clazz,
                                                               jlong bufferHandle, jlong bytes, jlong offset) {
#ifdef SP_VULKAN
    if (!g_backend || !g_backend->IsInitialized()) {
        return nullptr;
    }

    VulkanBackend::BufferHandle handle;
    handle.id = static_cast<uint64_t>(bufferHandle);

    jbyteArray result = env->NewByteArray(static_cast<jsize>(bytes));
    if (!result) return nullptr;

    jbyte* dataPtr = env->GetByteArrayElements(result, nullptr);

    bool success = g_backend->ReadBuffer(handle, dataPtr, static_cast<uint64_t>(bytes),
                                        static_cast<uint64_t>(offset));

    env->ReleaseByteArrayElements(result, dataPtr, 0);

    if (!success) {
        env->DeleteLocalRef(result);
        return nullptr;
    }

    return result;
#else
    return nullptr;
#endif
}

JNIEXPORT jlong JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_createKernel(JNIEnv* env, jclass clazz,
                                                                 jbyteArray spirv, jint bufferCount, jint pushConstantBytes) {
#ifdef SP_VULKAN
    if (!g_backend || !g_backend->IsInitialized()) {
        return 0;
    }

    jbyte* spirvPtr = env->GetByteArrayElements(spirv, nullptr);
    jsize spirvLen = env->GetArrayLength(spirv);

    VulkanBackend::KernelHandle handle = g_backend->CreateKernel(
        reinterpret_cast<uint32_t*>(spirvPtr),
        static_cast<size_t>(spirvLen),
        static_cast<uint32_t>(bufferCount),
        static_cast<uint32_t>(pushConstantBytes)
    );

    env->ReleaseByteArrayElements(spirv, spirvPtr, JNI_ABORT);

    return static_cast<jlong>(handle.id);
#else
    return 0;
#endif
}

JNIEXPORT jobject JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_dispatch(JNIEnv* env, jclass clazz,
                                                             jlong kernelHandle, jlongArray bufferHandles,
                                                             jint groupsX, jint groupsY, jint groupsZ,
                                                             jbyteArray pushConstants) {
#ifdef SP_VULKAN
    if (!g_backend || !g_backend->IsInitialized()) {
        return nullptr;
    }

    VulkanBackend::KernelHandle kernel;
    kernel.id = static_cast<uint64_t>(kernelHandle);

    // Convert buffer handles
    std::vector<VulkanBackend::BufferHandle> buffers;
    jsize bufferCount = env->GetArrayLength(bufferHandles);
    jlong* bufferPtr = env->GetLongArrayElements(bufferHandles, nullptr);

    for (jsize i = 0; i < bufferCount; i++) {
        VulkanBackend::BufferHandle handle;
        handle.id = static_cast<uint64_t>(bufferPtr[i]);
        buffers.push_back(handle);
    }

    env->ReleaseLongArrayElements(bufferHandles, bufferPtr, JNI_ABORT);

    // Get push constants
    void* pushConstPtr = nullptr;
    jsize pushConstLen = 0;
    if (pushConstants) {
        pushConstPtr = env->GetByteArrayElements(pushConstants, nullptr);
        pushConstLen = env->GetArrayLength(pushConstants);
    }

    // Dispatch
    VulkanBackend::DispatchResult result = g_backend->Dispatch(
        kernel, buffers,
        static_cast<uint32_t>(groupsX),
        static_cast<uint32_t>(groupsY),
        static_cast<uint32_t>(groupsZ),
        pushConstPtr,
        static_cast<uint32_t>(pushConstLen)
    );

    if (pushConstants) {
        env->ReleaseByteArrayElements(pushConstants, reinterpret_cast<jbyte*>(pushConstPtr), JNI_ABORT);
    }

    // Create DispatchResult Java object
    jclass resultClass = env->FindClass("io/stonepotatoes/web3agent/engine/GameEngine$DispatchResult");
    if (!resultClass) return nullptr;

    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZDD)V");
    if (!constructor) return nullptr;

    jobject resultObj = env->NewObject(resultClass, constructor,
        static_cast<jboolean>(result.ok),
        static_cast<jdouble>(result.gpuMs),
        static_cast<jdouble>(result.wallMs));

    return resultObj;
#else
    return nullptr;
#endif
}

JNIEXPORT void JNICALL
Java_io_stonepotatoes_web3agent_engine_GameEngine_destroyKernel(JNIEnv* env, jclass clazz, jlong kernelHandle) {
#ifdef SP_VULKAN
    if (!g_backend) return;

    VulkanBackend::KernelHandle handle;
    handle.id = static_cast<uint64_t>(kernelHandle);
    g_backend->DestroyKernel(handle);
#endif
}

} // extern "C"
