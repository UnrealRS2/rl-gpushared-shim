#include "rl_gpushared_shim.hpp"
#include <jni.h>
#include <windows.h>
#include <stdexcept>

extern "C" {

// ===========================================================================
// JNI: Open Shared Memory
// ===========================================================================
JNIEXPORT jlong JNICALL
Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_openSharedMemory(
    JNIEnv* env, jobject, jstring name)
{
    const char* shmName = env->GetStringUTFChars(name, nullptr);
    if (!shmName)
    {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "Shared memory name is null");
        return 0;
    }

    // Try to open existing mapping first
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shmName);
    if (!hMapFile)
    {
        // Not existing → create a new one
        constexpr size_t shmSize = sizeof(SharedMemoryRegion);
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            static_cast<DWORD>(shmSize >> 32),
            static_cast<DWORD>(shmSize & 0xFFFFFFFF),
            shmName
        );

        if (!hMapFile)
        {
            env->ReleaseStringUTFChars(name, shmName);
            env->ThrowNew(env->FindClass("java/io/IOException"), "Failed to create shared memory mapping");
            return 0;
        }
    }

    void* pBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryRegion));
    if (!pBase)
    {
        CloseHandle(hMapFile);
        env->ReleaseStringUTFChars(name, shmName);
        env->ThrowNew(env->FindClass("java/io/IOException"), "Failed to map shared memory view");
        return 0;
    }

    SharedMemoryRegion* region = reinterpret_cast<SharedMemoryRegion*>(pBase);

    env->ReleaseStringUTFChars(name, shmName);
    return reinterpret_cast<jlong>(region);
}

// ===========================================================================
// JNI: Close Shared Memory
// ===========================================================================
JNIEXPORT void JNICALL
Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_closeSharedMemory(
    JNIEnv*, jobject, jlong handle)
{
    SharedMemoryRegion* region = reinterpret_cast<SharedMemoryRegion*>(handle);
    if (!region)
        return;

    UnmapViewOfFile(region);
}

} // extern "C"
