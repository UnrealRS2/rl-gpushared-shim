// rl_gpushared_shim.cpp
#include <atomic>
#include <jni.h>
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>

extern "C" {

// Constants must match Java
static constexpr int BUFFER_COUNT = 2;
static constexpr int MAX_SCENE_SIZE = 8 * 1024 * 1024;            // 8 MB
static constexpr int MAX_UI_SIZE = 3840 * 2160 * 4;              // RGBA 4K
static constexpr int INPUT_QUEUE_SIZE = 256;

// POD structs with fixed layout for predictable offsets
#pragma pack(push, 1)
    struct RLCameraStatus
    {
        int x, y, z;
        int pitch, yaw, scale;
    };

    struct RLFrameStatus
    {
        int width;
        int height;
        bool ready = false;
        bool consumed = true;
    };

// Shared region layout (contiguous)
struct FixedSharedMemoryRegionPOD
{
    RLCameraStatus camera;
    RLFrameStatus front_frame_status;
    RLFrameStatus back_frame_status;
};
#pragma pack(pop)

// Compute sizes & offsets
static const size_t CAMERA_SIZE = sizeof(RLCameraStatus);
static const size_t FRAME_STATUS_SIZE = sizeof(RLFrameStatus);
static const size_t FIXED_SHARED_REGION_SIZE = sizeof(FixedSharedMemoryRegionPOD);

// Globals for mapping
static HANDLE gMapHandle = nullptr;
static uint8_t* gBase = nullptr; // base pointer to mapped view

// Utility: throw java.io.IOException with message
static void throw_io_exception(JNIEnv* env, const char* msg)
{
    jclass exClass = env->FindClass("java/io/IOException");
    if (exClass != nullptr)
        env->ThrowNew(exClass, msg);
}

// Pointers into metadata
static inline RLCameraStatus* ptr_camera() { return reinterpret_cast<RLCameraStatus*>(gBase); }
// Pointers into metadata
static inline RLCameraStatus* ptr_front_buffer_info() { return reinterpret_cast<RLCameraStatus*>(gBase + CAMERA_SIZE); }
static inline RLCameraStatus* ptr_back_buffer_info() { return reinterpret_cast<RLCameraStatus*>(gBase + CAMERA_SIZE + FRAME_STATUS_SIZE); }

// JNI functions (as in the generated header)

// openSharedMemory(String) -> jlong (returns base pointer as jlong)
JNIEXPORT jlong JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_openSharedMemory
  (JNIEnv* env, jobject /*this*/, jstring name)
{
    if (name == nullptr)
    {
        throw_io_exception(env, "Shared memory name is null");
        return 0;
    }

    const char* cname = env->GetStringUTFChars(name, nullptr);
    if (!cname)
    {
        throw_io_exception(env, "Failed to get string chars");
        return 0;
    }

    // Try open first
    gMapHandle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, cname);
    bool created = false;
    if (!gMapHandle)
    {
        // Create mapping
        DWORD sizeLow  = static_cast<DWORD>(FIXED_SHARED_REGION_SIZE & 0xFFFFFFFFULL);
        DWORD sizeHigh = static_cast<DWORD>((FIXED_SHARED_REGION_SIZE >> 32) & 0xFFFFFFFFULL);

        gMapHandle = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            sizeHigh,
            sizeLow,
            cname
        );

        if (!gMapHandle)
        {
            env->ReleaseStringUTFChars(name, cname);
            throw_io_exception(env, "Failed to create shared memory mapping");
            return 0;
        }
        // If created, GetLastError() may be ERROR_ALREADY_EXISTS; check:
        if (GetLastError() != ERROR_ALREADY_EXISTS) created = true;
    }

    // Map view
    void* mapped = MapViewOfFile(gMapHandle, FILE_MAP_ALL_ACCESS, 0, 0, FIXED_SHARED_REGION_SIZE);
    if (!mapped)
    {
        CloseHandle(gMapHandle);
        gMapHandle = nullptr;
        env->ReleaseStringUTFChars(name, cname);
        throw_io_exception(env, "Failed to map shared memory view");
        return 0;
    }

    gBase = reinterpret_cast<uint8_t*>(mapped);

    // If created new region, zero-init metadata and buffers to deterministic state
    if (created)
        std::memset(gBase, 0, FIXED_SHARED_REGION_SIZE);
    env->ReleaseStringUTFChars(name, cname);

    // Return base pointer as jlong for convenience (Java may ignore it)
    return reinterpret_cast<jlong>(gBase);
}

// closeSharedMemory(J)V
JNIEXPORT void JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_closeSharedMemory
  (JNIEnv* /*env*/, jobject /*this*/, jlong /*handle*/)
{
    if (gBase)
    {
        UnmapViewOfFile(gBase);
        gBase = nullptr;
    }
    if (gMapHandle)
    {
        CloseHandle(gMapHandle);
        gMapHandle = nullptr;
    }
}

// mapCamera(J)Ljava/nio/ByteBuffer;
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapCamera
  (JNIEnv* env, jobject /*this*/, jlong /*handle*/)
{
    if (!gBase) { throw_io_exception(env, "Shared memory not open"); return nullptr; }
    void* ptr = reinterpret_cast<void*>(ptr_camera());
    return env->NewDirectByteBuffer(ptr, (jlong)CAMERA_SIZE);
}

// mapCamera(J)Ljava/nio/ByteBuffer;
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapFrontBufferInfo
  (JNIEnv* env, jobject /*this*/, jlong /*handle*/)
{
    if (!gBase) { throw_io_exception(env, "Shared memory not open"); return nullptr; }
    void* ptr = reinterpret_cast<void*>(ptr_front_buffer_info());
    return env->NewDirectByteBuffer(ptr, (jlong)FRAME_STATUS_SIZE);
}

// mapCamera(J)Ljava/nio/ByteBuffer;
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapBackBufferInfo
   (JNIEnv* env, jobject /*this*/, jlong /*handle*/)
{
    if (!gBase) { throw_io_exception(env, "Shared memory not open"); return nullptr; }
    void* ptr = reinterpret_cast<void*>(ptr_back_buffer_info());
    return env->NewDirectByteBuffer(ptr, (jlong)FRAME_STATUS_SIZE);
}

} // extern "C"
