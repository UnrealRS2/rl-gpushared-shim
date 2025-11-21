// rl_gpushared_shim.cpp
#include <atomic>
#include <jni.h>
#include <windows.h>
#include <cstdint>

extern "C" {
// POD structs with fixed layout for predictable offsets
#pragma pack(push, 1)
struct RLCameraStatus {
    int x;
    int y;
    int z;
    int pitch;
    int yaw;
    int scale;
};

struct RLBufferInfo {
    int width{};
    int height{};
    bool ready{};
    bool consumed = true;

    //dummy padding so pixels is 4-byte aligned
    uint8_t dummy{};
    uint8_t dummy2{};

    uint8_t pixels[3840 * 2160 * 4]{};
};

struct SResolution {
    int width;
    int height;
    bool consumed = true;
};

struct SMouseMove {
    int x;
    int y;
    bool consumed = true;
};

struct SMousePress {
    int button;
    bool consumed = true;
};

struct SMouseRelease {
    int button;
    bool consumed = true;
};

// Shared region layout (contiguous)
struct FixedSharedMemoryRegionPOD {
    RLCameraStatus camera{};
    RLBufferInfo front_frame_status;
    RLBufferInfo back_frame_status;
    SResolution resolution;
    SMouseMove mouse_move;
    SMousePress mouse_press;
    SMouseRelease mouse_release;
};
#pragma pack(pop)

// Globals for mapping
static HANDLE gMapHandle = nullptr;
static uint8_t *gBase = nullptr; // base pointer to mapped view

// Utility: throw java.io.IOException with message
static void throw_io_exception(JNIEnv *env, const char *msg) {
    jclass exClass = env->FindClass("java/io/IOException");
    if (exClass != nullptr)
        env->ThrowNew(exClass, msg);
}

static uint8_t *shmPtr = nullptr;

/**
 * ptr_camera MUST be first in Shared Memory as it sets base [shmPtr]
 */
static inline RLCameraStatus *ptr_camera() {
    shmPtr = gBase + sizeof(RLCameraStatus);
    return reinterpret_cast<RLCameraStatus *>(shmPtr - sizeof(RLCameraStatus));
}
static inline RLBufferInfo *ptr_front_buffer_info() {
    shmPtr += sizeof(RLBufferInfo);
    return reinterpret_cast<RLBufferInfo *>(shmPtr - sizeof(RLBufferInfo));
}
static inline RLBufferInfo *ptr_back_buffer_info() {
    shmPtr += sizeof(RLBufferInfo);
    return reinterpret_cast<RLBufferInfo *>(shmPtr - sizeof(RLBufferInfo));
}
static inline SResolution *ptr_resolution() {
    shmPtr += sizeof(SResolution);
    return reinterpret_cast<SResolution *>(shmPtr - sizeof(SResolution));
}
static inline SMouseMove *ptr_mouse_move() {
    shmPtr += sizeof(SMouseMove);
    return reinterpret_cast<SMouseMove *>(shmPtr - sizeof(SMouseMove));
}
static inline SMousePress *ptr_mouse_press() {
    shmPtr += sizeof(SMousePress);
    return reinterpret_cast<SMousePress *>(shmPtr - sizeof(SMousePress));
}
static inline SMouseRelease *ptr_mouse_release() {
    shmPtr += sizeof(SMouseRelease);
    return reinterpret_cast<SMouseRelease *>(shmPtr - sizeof(SMouseRelease));
}

// JNI implementations

JNIEXPORT jlong JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_openSharedMemory
(JNIEnv *env, jobject /*this*/, jstring name) {
    if (name == nullptr) {
        throw_io_exception(env, "Shared memory name is null");
        return 0;
    }

    const char *cname = env->GetStringUTFChars(name, nullptr);
    if (!cname) {
        throw_io_exception(env, "Failed to get string chars");
        return 0;
    }

    // Try open first
    gMapHandle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, cname);
    bool created = false;
    if (!gMapHandle) {
        // Create mapping
        DWORD sizeLow = sizeof(FixedSharedMemoryRegionPOD) & 0xFFFFFFFFULL;
        DWORD sizeHigh = sizeof(FixedSharedMemoryRegionPOD) >> 32 & 0xFFFFFFFFULL;

        gMapHandle = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            sizeHigh,
            sizeLow,
            cname
        );

        if (!gMapHandle) {
            env->ReleaseStringUTFChars(name, cname);
            throw_io_exception(env, "Failed to create shared memory mapping");
            return 0;
        }
        // If created, GetLastError() may be ERROR_ALREADY_EXISTS; check:
        if (GetLastError() != ERROR_ALREADY_EXISTS) created = true;
    }

    // Map view
    void *mapped = MapViewOfFile(gMapHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(FixedSharedMemoryRegionPOD));
    if (!mapped) {
        CloseHandle(gMapHandle);
        gMapHandle = nullptr;
        env->ReleaseStringUTFChars(name, cname);
        throw_io_exception(env, "Failed to map shared memory view");
        return 0;
    }

    gBase = reinterpret_cast<uint8_t *>(mapped);

    // If created new region, zero-init metadata and buffers to deterministic state
    if (created)
        std::memset(gBase, 0, sizeof(FixedSharedMemoryRegionPOD));
    env->ReleaseStringUTFChars(name, cname);

    // Return base pointer as jlong for convenience (Java may ignore it)
    return reinterpret_cast<jlong>(gBase);
}
JNIEXPORT void JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_closeSharedMemory
(JNIEnv * /*env*/, jobject /*this*/, jlong /*handle*/) {
    if (gBase) {
        UnmapViewOfFile(gBase);
        gBase = nullptr;
    }
    if (gMapHandle) {
        CloseHandle(gMapHandle);
        gMapHandle = nullptr;
    }
}
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapCamera
(JNIEnv *env, jobject /*this*/, jlong /*handle*/) {
    void *ptr = ptr_camera();
    return env->NewDirectByteBuffer(ptr, sizeof(RLCameraStatus));
}
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapFrontBufferInfo
(JNIEnv *env, jobject /*this*/, jlong /*handle*/) {
    void *ptr = ptr_front_buffer_info();
    return env->NewDirectByteBuffer(ptr, sizeof(RLBufferInfo));
}
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapBackBufferInfo
(JNIEnv *env, jobject /*this*/, jlong /*handle*/) {
    void *ptr = ptr_back_buffer_info();
    return env->NewDirectByteBuffer(ptr, sizeof(RLBufferInfo));
}
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapResolution
(JNIEnv *env, jobject /*this*/, jlong /*handle*/) {
    void *ptr = ptr_resolution();
    return env->NewDirectByteBuffer(ptr, sizeof(SResolution));
}
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapMouseMove
(JNIEnv *env, jobject /*this*/, jlong /*handle*/) {
    void *ptr = ptr_mouse_move();
    return env->NewDirectByteBuffer(ptr, sizeof(SMouseMove));
}
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapMousePress
(JNIEnv *env, jobject /*this*/, jlong /*handle*/) {
    void *ptr = ptr_mouse_press();
    return env->NewDirectByteBuffer(ptr, sizeof(SMousePress));
}
JNIEXPORT jobject JNICALL Java_net_runelite_client_plugins_gpushared_shim_SharedMemoryBridge_mapMouseRelease
(JNIEnv *env, jobject /*this*/, jlong /*handle*/) {
    void *ptr = ptr_mouse_release();
    return env->NewDirectByteBuffer(ptr, sizeof(SMouseRelease));
}
} // extern "C"
