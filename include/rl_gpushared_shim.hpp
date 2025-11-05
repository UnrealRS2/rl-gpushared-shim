#pragma once
#include <atomic>
#include <cstdint>
#include <windows.h>

constexpr int MAX_SCENE_SIZE = 8 * 1024 * 1024; // 3D scene / 8 MB
constexpr int MAX_UI_SIZE = 3840 * 2160 * 4; // RGBA framebuffer / 4K
constexpr int BUFFER_COUNT = 2;
constexpr int INPUT_QUEUE_SIZE = 256;

struct CameraState
{
    float x, y, z;
    float yaw, pitch;
    float zoom;
};

struct FrameHeader
{
    std::atomic<uint32_t> ready{0};
    std::atomic<uint32_t> consumed{1};
    uint32_t frameId{0};
    uint32_t sceneDataLength{0};
    uint32_t uiDataLength{0};
};

enum class InputType : uint8_t { KeyDown, KeyUp, MouseDown, MouseUp, MouseMove };

struct InputEvent
{
    InputType type;
    uint8_t keyCode;
    uint8_t mouseButton;
    float mouseX;
    float mouseY;
};

struct InputQueue
{
    std::atomic<uint32_t> head{0};
    std::atomic<uint32_t> tail{0};
    InputEvent events[INPUT_QUEUE_SIZE];
};

struct Resolution
{
    std::atomic<uint32_t> width{0};
    std::atomic<uint32_t> height{0};
};

struct SharedMemoryRegion
{
    std::atomic<uint32_t> activeIndex{0};
    CameraState camera;

    FrameHeader frames[BUFFER_COUNT];
    uint8_t sceneData[BUFFER_COUNT][MAX_SCENE_SIZE];
    uint8_t uiData[BUFFER_COUNT][MAX_UI_SIZE];

    InputQueue inputQueue;

    Resolution runeliteResolution;
    Resolution externalResolution;
};
